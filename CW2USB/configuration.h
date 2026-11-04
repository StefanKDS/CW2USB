#ifndef MORSE_PADDLE_CONFIGURATION_H
#define MORSE_PADDLE_CONFIGURATION_H

#include <Arduino.h>

namespace Config {

// Paddle contacts connect these pins to GND.
constexpr uint8_t PADDLE_1_PIN = 2;
constexpr uint8_t PADDLE_2_PIN = 3;

// false: use DEFAULT_MODE immediately.
// true: select the operating mode using the paddle at startup.
constexpr bool SELECT_MODE_AT_STARTUP = true;

// Classic Pro Micro RX LED: pin 17, active LOW.
// Disable this option if your board uses a different LED circuit.
constexpr bool USE_STATUS_LED = true;
constexpr uint8_t STATUS_LED_PIN = 17;

// Disable the classic Pro Micro TX LED after setup() starts.
// The bootloader may still flash the LED during reset or upload.
constexpr bool DISABLE_TX_LED = true;

// Each contact is debounced independently.
constexpr unsigned long DEBOUNCE_MS = 5;

// Threshold used for startup mode selection.
constexpr unsigned long LONG_PRESS_MS = 250;

// Arduino Keyboard raw HID usage codes require an offset of 136.
// These physical keys produce "+" and "u with diaeresis" when the
// receiving device uses a German keyboard layout.
constexpr uint8_t MANIA_KEY_1 = 136 + 0x30;
constexpr uint8_t MANIA_KEY_2 = 136 + 0x2F;

// To use the a/b mapping from the original project's source code,
// replace the two definitions above with:
//
// constexpr uint8_t MANIA_KEY_1 = 'a';
// constexpr uint8_t MANIA_KEY_2 = 'b';

enum OperatingMode : uint8_t {
  STRAIGHT_KEY_MANIA = 1,
  PADDLE_MANIA      = 2,
  STRAIGHT_KEY_VBAND = 3,
  PADDLE_VBAND      = 4
};

constexpr OperatingMode DEFAULT_MODE = PADDLE_MANIA;

}  // namespace Config

#endif
