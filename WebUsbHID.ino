/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!
 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
/* This sketch demonstrates WebUSB as web serial with browser with WebUSB support (e.g Chrome).
 * After enumerated successfully, Browser will pop-up notification
 * with URL to landing page, click on it to test
 *  - Click "Connect" and select device, When connected the on-board LED will litted up.
 *  - Any charters received from either webusb/Serial will be echo back to webusb and Serial
 *  
 * Note: 
 * - The WebUSB landing page notification is currently disabled in Chrome 
 * on Windows due to Chromium issue 656702 (https://crbug.com/656702). You have to 
 * go to landing page (below) to test
 * 
 * - On Windows 7 and prior: You need to use Zadig tool to manually bind the 
 * WebUSB interface with the WinUSB driver for Chrome to access. From windows 8 and 10, this
 * is done automatically by firmware.
 */
#include "Adafruit_TinyUSB.h"
// Report ID
enum {
	RID_KEYBOARD = 1,
			  RID_MOUSE,
			  RID_CONSUMER_CONTROL, // Media, volume etc ..
}
;
// HID report descriptor using TinyUSB's template
uint8_t const desc_hid_report[] = {
	TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(RID_KEYBOARD) ),
			  TUD_HID_REPORT_DESC_MOUSE   ( HID_REPORT_ID(RID_MOUSE) ),
			  TUD_HID_REPORT_DESC_CONSUMER( HID_REPORT_ID(RID_CONSUMER_CONTROL) )
}
;
// USB HID object. For ESP32 these values cannot be changed after this declaration
// desc report, desc len, protocol, interval, use out endpoint
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, false);
// USB WebUSB object
Adafruit_USBD_WebUSB usb_web;
// Landing Page: scheme (0: http, 1: https), url
// Page source can be found at https://github.com/hathach/tinyusb-webusb-page/tree/main/webusb-serial
WEBUSB_URL_DEF(landingPage, 1 
/*https*/
, "example.tinyusb.org/webusb-serial/index.html");
int led_pin = LED_BUILTIN;
// the setup function runs once when you press reset or power the board
void setup() {
	#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
			  // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
	TinyUSB_Device_Init(0);
	#endif
			usb_hid.begin();
	pinMode(led_pin, OUTPUT);
	digitalWrite(led_pin, LOW);
	usb_web.setLandingPage(&landingPage);
	usb_web.setLineStateCallback(line_state_callback);
	//usb_web.setStringDescriptor("TinyUSB WebUSB");
	usb_web.begin();
	Serial.begin(115200);
	// wait until device mounted
	while( !TinyUSBDevice.mounted() ) delay(1);
	Serial.println("TinyUSB WebUSB Serial and HID example");
}
String rcv_data = "";
// function to echo to both Serial and WebUSB
void echo_all(uint8_t buf[], uint32_t count) {
	for (uint32_t i=0; i<count; i++) {
		if ( buf[i] == '\r' ) {
			if(rcv_data=="shoot") {
				mouse_click();
			} else if(rcv_data.startsWith("move")) {
				String x = rcv_data.substring(4,rcv_data.indexOf(':'));
				String y = rcv_data.substring(rcv_data.indexOf(':')+1);
				// if ( Serial ) {
				// 	Serial.write(x.c_str());
				// 	Serial.write(y.c_str());
				// 	Serial.flush();
				// }
				mouse_move(x.toInt(),y.toInt());
			}
			rcv_data = "";
		} else {
			rcv_data.concat(String((char)buf[i]));
		}
	}
	if (usb_web.connected()) {
		usb_web.write(buf, count);
		usb_web.flush();
	}
	if ( Serial ) {
		for (uint32_t i=0; i<count; i++) {
			Serial.write(buf[i]);
			if ( buf[i] == '\r' ) Serial.write('\n');
		}
		Serial.flush();
	}
}
void mouse_click() {
	if ( usb_hid.ready()) {
		usb_hid.mouseButtonPress(RID_MOUSE, 1);
		delay(10);
		usb_hid.mouseButtonRelease(RID_MOUSE);
	}
}
void mouse_move(uint8_t x,uint8_t y) {
	if ( usb_hid.ready()) {
		usb_hid.mouseMove(RID_MOUSE,x,y);
	}
}
void loop() {
	uint8_t buf[64];
	uint32_t count;
	// From Serial to both Serial & webUSB
	if (Serial.available()) {
		count = Serial.read(buf, 64);
		echo_all(buf, count);
	}
	// from WebUSB to both Serial & webUSB
	if (usb_web.available()) {
		count = usb_web.read(buf, 64);
		echo_all(buf, count);
	}
	// Remote wakeup
	if ( TinyUSBDevice.suspended() ) {
		// Wake up host if we are in suspend mode
		// and REMOTE_WAKEUP feature is enabled by host
		TinyUSBDevice.remoteWakeup();
	}
	delay(10);
}
void line_state_callback(bool connected) {
	digitalWrite(led_pin, connected);
	if ( connected ) {
		usb_web.println("WebUSB interface connected !!");
		usb_web.flush();
	}
}