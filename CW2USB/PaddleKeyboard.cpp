#include "PaddleKeyboard.h"

#include <HID.h>
#include <string.h>

namespace {
// Arduino Keyboard's current descriptor uses report ID 2.
// This implementation targets the standard Arduino Keyboard library
// and Arduino AVR core, not HID-Project or other keyboard libraries.
constexpr uint8_t KEYBOARD_REPORT_ID = 2;
constexpr uint8_t LEFT_CTRL_MASK = 0x01;
constexpr uint8_t RIGHT_CTRL_MASK = 0x10;
constexpr uint8_t SPACE_USAGE = 0x2C;

static_assert(sizeof(KeyReport) == 8, "Unexpected Keyboard report format.");
static_assert(Config::MANIA_KEY_1_USAGE > 0 &&
                  Config::MANIA_KEY_1_USAGE <= 0x73,
              "MANIA_KEY_1_USAGE must be a supported non-modifier HID usage.");
static_assert(Config::MANIA_KEY_2_USAGE > 0 &&
                  Config::MANIA_KEY_2_USAGE <= 0x73,
              "MANIA_KEY_2_USAGE must be a supported non-modifier HID usage.");
}

PaddleKeyboard::PaddleKeyboard()
    : mode_(Config::DEFAULT_MODE),
      lastSentReport_{},
      haveSentReport_(false) {
}

void PaddleKeyboard::begin() {
  // Referencing the standard Keyboard object links its constructor,
  // which registers the keyboard HID descriptor before setup().
  Keyboard.begin();
  haveSentReport_ = false;
}

void PaddleKeyboard::setMode(OperatingMode mode) {
  mode_ = mode;
}

bool PaddleKeyboard::update(bool contact1Pressed, bool contact2Pressed) {
  KeyReport report = {};

  switch (mode_) {
    case OperatingMode::StraightKeyMania:
      if (contact1Pressed) {
        report.keys[0] = SPACE_USAGE;
      }
      break;

    case OperatingMode::PaddleMania:
      if (contact1Pressed) {
        report.keys[0] = Config::MANIA_KEY_1_USAGE;
      }
      if (contact2Pressed &&
          (!contact1Pressed ||
           Config::MANIA_KEY_2_USAGE != Config::MANIA_KEY_1_USAGE)) {
        report.keys[1] = Config::MANIA_KEY_2_USAGE;
      }
      break;

    case OperatingMode::StraightKeyVBand:
      if (contact1Pressed) {
        report.modifiers = LEFT_CTRL_MASK;
      }
      break;

    case OperatingMode::PaddleVBand:
      if (contact1Pressed) {
        report.modifiers |= LEFT_CTRL_MASK;
      }
      if (contact2Pressed) {
        report.modifiers |= RIGHT_CTRL_MASK;
      }
      break;
  }

  return sendIfChanged(report);
}

bool PaddleKeyboard::releaseAll() {
  const KeyReport released = {};
  return sendIfChanged(released);
}

bool PaddleKeyboard::sendIfChanged(const KeyReport& report) {
  if (haveSentReport_ &&
      memcmp(&report, &lastSentReport_, sizeof(report)) == 0) {
    return true;
  }

  // One complete report represents BOTH contacts. The host may still
  // deliver separate key events; this is not a simultaneous-event API.
  const int sent = HID().SendReport(KEYBOARD_REPORT_ID, &report,
                                    sizeof(report));

  // Do not cache a failed transfer: the next loop retries the current
  // state. This is not a queue of every edge during a USB outage.
  if (sent != static_cast<int>(sizeof(report) + 1)) {
    return false;
  }

  lastSentReport_ = report;
  haveSentReport_ = true;
  return true;
}
