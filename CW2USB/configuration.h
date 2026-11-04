#pragma once

#include <Arduino.h>

#if !defined(__AVR_ATmega32U4__)
#error "Select an ATmega32U4 Pro Micro board before compiling."
#endif

enum class OperatingMode : uint8_t {
  StraightKeyMania = 1,
  PaddleMania = 2,
  StraightKeyVBand = 3,
  PaddleVBand = 4
};

namespace Config {

// Each contact closes to GND. Internal pull-ups are enabled.
constexpr uint8_t PADDLE_1_PIN = 2;
constexpr uint8_t PADDLE_2_PIN = 3;

// false: use DEFAULT_MODE directly.
// true: select one of four modes with a startup paddle gesture.
constexpr bool ENABLE_STARTUP_MODE_SELECTION = true;
constexpr OperatingMode DEFAULT_MODE = OperatingMode::PaddleMania;

// Prompt detection: a transition after a quiet interval is accepted
// on the first observed edge. Bouncing transitions require settling.
// Set to 0 only for a diagnostic test with no debounce filtering.
constexpr uint32_t DEBOUNCE_INTERVAL_US = 5000UL;

// Short: below this duration. Long: at or above this duration.
constexpr uint32_t LONG_PRESS_MS = 250UL;

// Physical USB HID usage IDs, NOT ASCII and NOT Arduino key values.
// On a German host keyboard layout: 0x30 = '+', 0x2F = 'ue' umlaut.
// On a US host keyboard layout:     0x30 = ']', 0x2F = '['.
// To use physical A/B keys instead, use 0x04 and 0x05.
constexpr uint8_t MANIA_KEY_1_USAGE = 0x30;
constexpr uint8_t MANIA_KEY_2_USAGE = 0x2F;

// Classic SparkFun Pro Micro RX LED: pin 17, active LOW.
// The USB core also accesses this LED; it is not an exclusive indicator.
constexpr bool ENABLE_STATUS_LED = true;
constexpr uint8_t STATUS_LED_PIN = 17;

// Classic Pro Micro TX LED is connected to PD5.
// Disabling applies after setup() starts, not to the bootloader.
constexpr bool DISABLE_TX_LED = true;

}  // namespace Config
