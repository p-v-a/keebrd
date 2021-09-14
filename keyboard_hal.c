/**
  @File Name
    main.c

  @Summary
    This source file provides keyboard handling routines.

  @Description
    This source file provides keyboard handling routines.
*/
#include "keyboard_hal.h"
#include "hw/keyboard/keyboard.h"
#include "hw/usb/usb.h"

#include "hw/system.h"
#include "hw/keyboard/keycode.h"

static KEYBOARD keyboard;
static KEYBOARD_INPUT_REPORT inputReport;
static volatile KEYBOARD_OUTPUT_REPORT outputReport;

//Application variables that need wide scope
KEYBOARD_INPUT_REPORT oldInputReport;
signed int keyboardIdleRate;
signed int LocalSOFCount;
static signed int OldSOFCount;

//Class specific descriptor - HID Keyboard
const struct{uint8_t report[HID_RPT01_SIZE];}hid_rpt01={
{   0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs), was 03
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs) , was 03
    0x95, KEYBOARD_REPORT_KEYS,                    //   REPORT_COUNT (KEYBOARD_REPORT_KEYS)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0}                          // End Collection
};

void KeyboardInit(void)
{
    keyboard.lastINTransmission = 0;

    //enable the HID endpoint
    USBEnableEndpoint(HID_EP, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

    //Arm OUT endpoint so we can receive caps lock, num lock, etc. info from host
    keyboard.lastOUTTransmission = HIDRxPacket(HID_EP,(uint8_t*)&outputReport, sizeof(outputReport) );
}

void KeyboardTasks(void)
{
    signed int TimeDeltaMilliseconds;
    unsigned char i;
    bool needToSendNewReportPacket;

    /* If the USB device isn't configured yet, we can't really do anything
     * else since we don't have a host to talk to.  So jump back to the
     * top of the while loop. */
    if( USBGetDeviceState() < CONFIGURED_STATE )
    {
        return;
    }

    /* If we are currently suspended, then we need to see if we need to
     * issue a remote wakeup.  In either case, we shouldn't process any
     * keyboard commands since we aren't currently communicating to the host
     * thus just continue back to the start of the while loop. */
    if( USBIsDeviceSuspended()== true )
    {
        return;
    }

    //Copy the (possibly) interrupt context SOFCounter value into a local variable.
    //Using a while() loop to do this since the SOFCounter isn't necessarily atomically
    //updated and we need to read it a minimum of twice to ensure we captured the correct value.
    while(LocalSOFCount != SOFCounter)
    {
        LocalSOFCount = SOFCounter;
    }

    //Compute the elapsed time since the last input report was sent (we need
    //this info for properly obeying the HID idle rate set by the host).
    TimeDeltaMilliseconds = LocalSOFCount - OldSOFCount;
    //Check for negative value due to count wraparound back to zero.
    if(TimeDeltaMilliseconds < 0)
    {
        TimeDeltaMilliseconds = (32767 - OldSOFCount) + LocalSOFCount;
    }

    //Check if the TimeDelay is quite large.  If the idle rate is == 0 (which represents "infinity"),
    //then the TimeDeltaMilliseconds could also become infinity (which would cause overflow)
    //if there is no recent button presses or other changes occurring on the keyboard.
    //Therefore, saturate the TimeDeltaMilliseconds if it gets too large, by virtue
    //of updating the OldSOFCount, even if we haven't actually sent a packet recently.
    if(TimeDeltaMilliseconds > 5000)
    {
        OldSOFCount = LocalSOFCount - 5000;
    }
    
    keyboard_task (&KeyboardKeyAction);

    /* Check if the IN endpoint is busy, and if it isn't check if we want to send
     * keystroke data to the host. */
    if(HIDTxHandleBusy(keyboard.lastINTransmission) == false)
    {
        //Check to see if the new packet contents are somehow different from the most
        //recently sent packet contents.
        needToSendNewReportPacket = false;
        for(i = 0; i < sizeof(inputReport); i++)
        {
            if(*((uint8_t*)&oldInputReport + i) != *((uint8_t*)&inputReport + i))
            {
                needToSendNewReportPacket = true;
                break;
            }
        }

        //Check if the host has set the idle rate to something other than 0 (which is effectively "infinite").
        //If the idle rate is non-infinite, check to see if enough time has elapsed since
        //the last packet was sent, and it is time to send a new repeated packet or not.
        if(keyboardIdleRate != 0)
        {
            //Check if the idle rate time limit is met.  If so, need to send another HID input report packet to the host
            if(TimeDeltaMilliseconds >= keyboardIdleRate)
            {
                needToSendNewReportPacket = true;
            }
        }

        //Now send the new input report packet, if it is appropriate to do so (ex: new data is
        //present or the idle rate limit was met).
        if(needToSendNewReportPacket == true)
        {
            //Save the old input report packet contents.  We do this so we can detect changes in report packet content
            //useful for determining when something has changed and needs to get re-sent to the host when using
            //infinite idle rate setting.
            oldInputReport = inputReport;

            /* Send the 8 byte packet over USB to the host. */
            keyboard.lastINTransmission = HIDTxPacket(HID_EP, (uint8_t*)&inputReport, sizeof(inputReport));
            OldSOFCount = LocalSOFCount;    //Save the current time, so we know when to send the next packet (which depends in part on the idle rate setting)
        }
    }

    /* Check if any data was sent from the PC to the keyboard device.  Report
     * descriptor allows host to send 1 byte of data.  Bits 0-4 are LED states,
     * bits 5-7 are unused pad bits.  The host can potentially send this OUT
     * report data through the HID OUT endpoint (EP1 OUT), or, alternatively,
     * the host may try to send LED state information by sending a SET_REPORT
     * control transfer on EP0.  See the USBHIDCBSetReportHandler() function. */
    if(HIDRxHandleBusy(keyboard.lastOUTTransmission) == false)
    {
        KeyboardProcessOutputReport();

        keyboard.lastOUTTransmission = HIDRxPacket(HID_EP,(uint8_t*)&outputReport,sizeof(outputReport));
    }
    
    return;
}

void KeyboardUpdateLED(void)
{
    uint8_t leds_status = 0;
    
    if(outputReport.leds.scrollLock != 0)
    {
        leds_status |= (1 << USB_LED_SCROLL_LOCK);
    }
    
    if(outputReport.leds.numLock != 0)
    {
        leds_status |= (1 << USB_LED_NUM_LOCK);
    }
    
    if(outputReport.leds.capsLock != 0)
    {
        leds_status |= (1 << USB_LED_CAPS_LOCK);
    }
    
    keyboard_set_leds(leds_status);
}

void KeyboardProcessOutputReport(void)
{
    KeyboardUpdateLED();
}

static void USBHIDCBSetReportComplete(void)
{
    /* 1 byte of LED state data should now be in the CtrlTrfData buffer.  Copy
     * it to the OUTPUT report buffer for processing */
    outputReport.value = CtrlTrfData[0];

    /* Process the OUTPUT report. */
    KeyboardProcessOutputReport();
}

void USBHIDCBSetReportHandler(void)
{
    /* Prepare to receive the keyboard LED state data through a SET_REPORT
     * control transfer on endpoint 0.  The host should only send 1 byte,
     * since this is all that the report descriptor allows it to send. */
    USBEP0Receive((uint8_t*)&CtrlTrfData, USB_EP0_BUFF_SIZE, USBHIDCBSetReportComplete);
}

//Callback function called by the USB stack, whenever the host sends a new SET_IDLE
//command.
void USBHIDCBSetIdleRateHandler(uint8_t reportID, uint8_t newIdleRate)
{
    //Make sure the report ID matches the keyboard input report id number.
    //If however the firmware doesn't implement/use report ID numbers,
    //then it should be == 0.
    if(reportID == 0)
    {
        keyboardIdleRate = newIdleRate;
    }
}

void KeyboardKeyAction (keyevent_t e) {
    if (e.pressed) {
        RegisterKeyCode (e.keycode);
    }
    else {
        UnregisterKeyCode (e.keycode);
    }
}

RegisterKeyCode (uint8_t code) {
    if (code == KC_NO) {
        return ;
    }
    else if (IS_KEY(code)) {
        AddKey (code);
    }
    else if (IS_MOD(code)) {
        AddMods (MOD_BIT(code));
    }
}

UnregisterKeyCode (uint8_t code) {
    if (code == KC_NO) {
        return ;
    }
    else if (IS_KEY(code)) {
        DelKey (code);
    }
    else if (IS_MOD(code)) {
        DelMods (MOD_BIT(code));
    }
}

void AddMods(uint8_t mods) { inputReport.modifiers.value |= mods; }
void DelMods(uint8_t mods) { inputReport.modifiers.value &= ~mods; }
void SetMods(uint8_t mods) { inputReport.modifiers.value = mods; }
void ClearMods(void) { inputReport.modifiers.value = 0; }

void AddKey(uint8_t code) {
    uint8_t i;
    int8_t empty = -1;
    
    for (i = 0; i < KEYBOARD_REPORT_KEYS; i++) {
        if (inputReport.keys[i] == code) {
            break;
        }
        
        if (empty == -1 && inputReport.keys[i] == KC_NO) {
            empty = i;
        }
    }
    
    if (i == KEYBOARD_REPORT_KEYS) {
        if (empty != -1) {
            inputReport.keys[empty] = code;
        }
    }
}

void DelKey(uint8_t code) {
    uint8_t i;
    for (i = 0; i < KEYBOARD_REPORT_KEYS; i++) {
        if (inputReport.keys[i] == code) {
            inputReport.keys[i] = KC_NO;
        }
    }
}

void ClearKeys(void) {
    uint8_t i;
    for (i = 0; i < KEYBOARD_REPORT_KEYS; i++) {
        inputReport.keys[i] = KC_NO;
    }
}
