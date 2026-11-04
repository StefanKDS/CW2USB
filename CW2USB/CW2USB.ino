/*
  MorsePaddle - USB HID paddle adapter for ATmega32U4 Pro Micro.

  Wiring:
    Contact 1 -> pin 2
    Contact 2 -> pin 3
    Common    -> GND

  Functional inspiration: barryshaffer/cw2usbkb.
  New implementation; no hardware test has been performed.
*/

#include <Arduino.h>
#include <avr/io.h>

#include "Configuration.h"
#include "Contact.h"
#include "PaddleKeyboard.h"

Contact contact1(Config::PADDLE_1_PIN, Config::DEBOUNCE_INTERVAL_US);
Contact contact2(Config::PADDLE_2_PIN, Config::DEBOUNCE_INTERVAL_US);
PaddleKeyboard paddleKeyboard;

OperatingMode currentMode = Config::DEFAULT_MODE;

void setStatusLed(bool on) {
  if (Config::ENABLE_STATUS_LED) {
    digitalWrite(Config::STATUS_LED_PIN, on ? LOW : HIGH);
  }
}

uint32_t sampleContacts() {
  // Read both inputs before any USB transfer. They are sampled
  // sequentially, not by a truly simultaneous hardware snapshot.
  const bool raw1 = contact1.readRawPressed();
  const bool raw2 = contact2.readRawPressed();
  const uint32_t nowUs = micros();

  contact1.update(raw1, nowUs);
  contact2.update(raw2, nowUs);
  return nowUs;
}

void waitForRelease() {
  // Startup only: require both raw signals to settle in the open state.
  while (true) {
    const uint32_t nowUs = sampleContacts();

    if (contact1.isReleasedAndStable(nowUs) &&
        contact2.isReleasedAndStable(nowUs)) {
      return;
    }

    delay(1);
  }
}

void selectStartupMode() {
  waitForRelease();

  uint8_t selectedContact = 0;
  uint32_t pressStartedMs = 0;

  while (true) {
    sampleContacts();
    setStatusLed(((millis() / 250UL) & 1UL) != 0);

    if (selectedContact == 0) {
      if (contact1.isPressed()) {
        selectedContact = 1;
        pressStartedMs = millis();
      } else if (contact2.isPressed()) {
        selectedContact = 2;
        pressStartedMs = millis();
      }
    } else {
      const bool stillPressed = (selectedContact == 1)
                                    ? contact1.isPressed()
                                    : contact2.isPressed();

      if (!stillPressed) {
        const bool longPress =
            static_cast<uint32_t>(millis() - pressStartedMs) >=
            Config::LONG_PRESS_MS;

        if (selectedContact == 1) {
          currentMode = longPress ? OperatingMode::StraightKeyVBand
                                  : OperatingMode::StraightKeyMania;
        } else {
          currentMode = longPress ? OperatingMode::PaddleVBand
                                  : OperatingMode::PaddleMania;
        }

        break;
      }
    }

    delay(1);
  }

  setStatusLed(false);
  waitForRelease();
}

void confirmMode() {
  if (!Config::ENABLE_STATUS_LED) {
    return;
  }

  delay(200);

  for (uint8_t i = 0; i < static_cast<uint8_t>(currentMode); ++i) {
    setStatusLed(true);
    delay(100);
    setStatusLed(false);
    delay(150);
  }
}

void setup() {
  if (Config::DISABLE_TX_LED) {
    // PD5 is an input, so the USB core's PORTD LED writes cannot
    // drive the TX LED as a LOW output. Bootloader behavior is separate.
    DDRD &= ~_BV(DDD5);
    PORTD |= _BV(PORTD5);
  }

  if (Config::ENABLE_STATUS_LED) {
    digitalWrite(Config::STATUS_LED_PIN, HIGH);
    pinMode(Config::STATUS_LED_PIN, OUTPUT);
  }

  contact1.begin();
  contact2.begin();
  paddleKeyboard.begin();

  delay(1000);
  paddleKeyboard.releaseAll();

  if (Config::ENABLE_STARTUP_MODE_SELECTION) {
    selectStartupMode();
  }

  paddleKeyboard.setMode(currentMode);
  confirmMode();

  // No startup selection or held-at-boot contact becomes a key press.
  waitForRelease();
  setStatusLed(false);
}

void loop() {
  sampleContacts();

  // One combined report, only when the effective keyboard state changes.
  paddleKeyboard.update(contact1.isPressed(), contact2.isPressed());

  const bool secondContactUsed =
      currentMode == OperatingMode::PaddleMania ||
      currentMode == OperatingMode::PaddleVBand;

  setStatusLed(contact1.isPressed() ||
               (secondContactUsed && contact2.isPressed()));

  // No delay(), serial logging, or LED blink sequence in normal operation.
  // USB transfers can still wait inside the Arduino core.
}
