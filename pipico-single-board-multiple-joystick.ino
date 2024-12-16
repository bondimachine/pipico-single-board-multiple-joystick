#include "Adafruit_TinyUSB.h"
#if CFG_TUD_HID < 2
  #error "Requires two HID instances support. See https://github.com/adafruit/Adafruit_TinyUSB_Arduino/commit/b75604f794acdf88daad310dd75d3a0724129056"
#endif 

#define LEFT_LEFT 28
#define LEFT_TOP 27
#define LEFT_RIGHT 26
#define LEFT_BOTTOM 22

#define LEFT_A 19
#define LEFT_B 18
#define LEFT_X 21
#define LEFT_Y 20
#define LEFT_L 16
#define LEFT_R 17
#define LEFT_SELECT 13
#define LEFT_START 12


#define RIGHT_LEFT 0
#define RIGHT_TOP 1
#define RIGHT_RIGHT 2
#define RIGHT_BOTTOM 3

#define RIGHT_A 8 
#define RIGHT_B 5 
#define RIGHT_X 9 
#define RIGHT_Y 7 
#define RIGHT_L 4 
#define RIGHT_R 6
#define RIGHT_SELECT 11
#define RIGHT_START 10


#define TUD_HID_REPORT_DESC_SHORT_GAMEPAD(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    /* 8 bit X, Y (min -127, max 127 ) */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_LOGICAL_MIN    ( 0x81                                   ) ,\
    HID_LOGICAL_MAX    ( 0x7f                                   ) ,\
    HID_REPORT_COUNT   ( 2                                      ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 8 bit Button Map */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN      ( 1                                      ) ,\
    HID_USAGE_MAX      ( 8                                      ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( 8                                      ) ,\
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

// NB NB!!! Select "Adafruit TinyUSB" for USB stack

// HID report descriptor using TinyUSB's template
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_SHORT_GAMEPAD()
};

// USB HID object. For ESP32 these values cannot be changed after this declaration
// desc report, desc len, protocol, interval, use out endpoint
Adafruit_USBD_HID usb_hid[] {
  Adafruit_USBD_HID(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, false), 
  Adafruit_USBD_HID(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, false)
};

/// HID Gamepad Protocol Report.
typedef struct TU_ATTR_PACKED {
  int8_t  x;         ///< Delta x  movement of left analog-stick
  int8_t  y;         ///< Delta y  movement of left analog-stick
  uint8_t buttons;  ///< Buttons mask for currently pressed buttons
} hid_short_gamepad_report_t;

hid_short_gamepad_report_t   gp[2]; // Two gamepad descriptors

uint8_t axes_pins[] = {
  LEFT_LEFT,
  LEFT_TOP,
  LEFT_RIGHT,
  LEFT_BOTTOM,

  RIGHT_LEFT,
  RIGHT_TOP,
  RIGHT_RIGHT,
  RIGHT_BOTTOM,
};

uint8_t left_button_pins[] = {
  LEFT_A,
  LEFT_B,
  LEFT_X,
  LEFT_Y,
  LEFT_L,
  LEFT_R,
  LEFT_SELECT,
  LEFT_START,
};

uint8_t right_button_pins[] = {
  RIGHT_A,
  RIGHT_B,
  RIGHT_X,
  RIGHT_Y,
  RIGHT_L,
  RIGHT_R,
  RIGHT_SELECT,
  RIGHT_START
};

inline int8_t axis_from_buttons(uint8_t lower, uint8_t upper) {
  return lower == LOW ? -128 : upper == LOW ? 127 : 0;
}

void setup() {
  
  // Manual begin() is required on core without built-in support e.g. mbed rp2040
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }

  Serial.begin(115200);
  usb_hid[0].begin();
  usb_hid[1].begin();

  // If already enumerated, additional class driverr begin() e.g msc, hid, midi won't take effect until re-enumeration
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }
  
  Serial.println("Adafruit TinyUSB HID multi-gamepad example");

  for (int i = 0; i<8; i++) {
    pinMode(axes_pins[i], INPUT_PULLUP);
  }

  for (int i = 0; i<8; i++) {
    pinMode(left_button_pins[i], INPUT_PULLUP);
  }

  for (int i = 0; i<8; i++) {
    pinMode(right_button_pins[i], INPUT_PULLUP);
  }

}

void loop() {
  #ifdef TINYUSB_NEED_POLLING_TASK
  // Manual call tud_task since it isn't called by Core's background
  TinyUSBDevice.task();
  #endif

  // not enumerated()/mounted() yet: nothing to do
  if (!TinyUSBDevice.mounted()) {
    return;
  }

// poll gpio once each 10 ms
  static uint32_t ms = 0;
  if (millis() - ms < 2) {
    return;
  }

  ms = millis();

  uint8_t axes[8];
  for (int i = 0; i<8; i++) {
    axes[i] = digitalRead(axes_pins[i]);
  }

  gp[0].buttons = 0;
  for (int i = 0; i<8; i++) {
    gp[0].buttons |= (digitalRead(left_button_pins[i]) == LOW) << i;
  }

  gp[1].buttons = 0;
  for (int i = 0; i<8; i++) {
    gp[1].buttons |= (digitalRead(right_button_pins[i]) == LOW) << i;
  }

  gp[0].x = axis_from_buttons(axes[2], axes[0]);
  gp[0].y = axis_from_buttons(axes[3], axes[1]);

  gp[1].x = axis_from_buttons(axes[6], axes[4]);
  gp[1].y = axis_from_buttons(axes[5], axes[7]);

  Serial.print("left x: ");
  Serial.print(gp[0].x);
  Serial.print(" y: ");
  Serial.print(gp[0].y);
  Serial.print(" buttons: 0x");
  Serial.print(gp[0].buttons, HEX);

  Serial.print(" - right x: ");
  Serial.print(gp[1].x);
  Serial.print(" y: ");
  Serial.print(gp[1].y);
  Serial.print(" buttons: 0x");
  Serial.print(gp[1].buttons, HEX);

  Serial.println("");

  if (usb_hid[0].ready()) {
    usb_hid[0].sendReport(0, &gp[0], sizeof(gp[0]));
  }
  
  if (usb_hid[1].ready()) {
    usb_hid[1].sendReport(0, &gp[1], sizeof(gp[1]));
  }

}