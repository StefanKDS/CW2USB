/*
  MorsePaddle - USB HID keyboard adapter for an ATmega32U4 Pro Micro.

  Functional reimplementation inspired by:
  barryshaffer/cw2usbkb, author KK7JXG.

  Wiring:
    Paddle contact 1 -> pin 2
    Paddle contact 2 -> pin 3
    Common contact  -> GND

  Default output:
    Pin 2 -> "+"
    Pin 3 -> "u with diaeresis" (U+00FC)

  The receiving device must use a German keyboard layout
  for these physical HID keys to produce the intended characters.

  Keyboard keys remain pressed until their contacts are released.
  Both contacts operate independently.

  Configuration is stored in Configuration.h.
*/

#include <Arduino.h>
#include <Keyboard.h>

#if !defined(__AVR_ATmega32U4__)
#error "This sketch requires a Pro Micro with an ATmega32U4."
#endif

#include <avr/io.h>

#include "Configuration.h"
#include "Contact.h"

Contact contact1(Config::PADDLE_1_PIN, Config::DEBOUNCE_MS);
Contact contact2(Config::PADDLE_2_PIN, Config::DEBOUNCE_MS);

Config::OperatingMode currentMode = Config::DEFAULT_MODE;

uint8_t activeKey1 = 0;
uint8_t activeKey2 = 0;

bool key1Sent = false;
bool key2Sent = false;

// Explicit declarations also keep the sketch independent of
// Arduino IDE automatic function-prototype generation.
void setStatusLed(bool on);
void updateContacts();
void waitForRelease();
void selectOperatingMode();
void assignKeys();
void confirmOperatingMode();
void updateKey(bool contactPressed, uint8_t key, bool &alreadySent);

void setup() {
  // Classic Pro Micro TX LED: PD5.
  // Make PD5 an input so USB activity cannot drive the LED LOW.
  // This affects sketch execution only, not the bootloader.
  if (Config::DISABLE_TX_LED) {
    DDRD &= ~_BV(DDD5);
    PORTD |= _BV(PORTD5);
  }

  if (Config::USE_STATUS_LED) {
    digitalWrite(Config::STATUS_LED_PIN, HIGH);
    pinMode(Config::STATUS_LED_PIN, OUTPUT);
  }

  contact1.begin();
  contact2.begin();

  Keyboard.begin();

  // Do not wait for a serial connection: this also runs on a phone.
  delay(1000);
  Keyboard.releaseAll();

  if (Config::SELECT_MODE_AT_STARTUP) {
    selectOperatingMode();
  }

  assignKeys();
  confirmOperatingMode();

  // A contact held during startup must not generate a key press.
  waitForRelease();
  setStatusLed(false);
}

void loop() {
  updateContacts();

  updateKey(contact1.isPressed(), activeKey1, key1Sent);
  updateKey(contact2.isPressed(), activeKey2, key2Sent);

  setStatusLed(key1Sent || key2Sent);
}

void setStatusLed(bool on) {
  if (Config::USE_STATUS_LED) {
    digitalWrite(Config::STATUS_LED_PIN, on ? LOW : HIGH);
  }
}

void updateContacts() {
  contact1.update();
  contact2.update();
}

void waitForRelease() {
  // Require both the electrical and debounced states to be released.
  while (true) {
    updateContacts();

    if (digitalRead(Config::PADDLE_1_PIN) == HIGH &&
        digitalRead(Config::PADDLE_2_PIN) == HIGH &&
        !contact1.isPressed() &&
        !contact2.isPressed()) {
      return;
    }

    delay(1);
  }
}

void selectOperatingMode() {
  waitForRelease();

  uint8_t selectedContact = 0;
  unsigned long pressStartMs = 0;

  while (true) {
    updateContacts();

    // Blink while waiting for a mode selection.
    setStatusLed(((millis() / 250UL) & 1UL) != 0);

    if (selectedContact == 0) {
      if (contact1.isPressed()) {
        selectedContact = 1;
        pressStartMs = millis();
      } else if (contact2.isPressed()) {
        selectedContact = 2;
        pressStartMs = millis();
      }
    } else {
      const bool stillPressed =
        (selectedContact == 1)
          ? contact1.isPressed()
          : contact2.isPressed();

      if (!stillPressed) {
        const bool longPress =
          (millis() - pressStartMs >= Config::LONG_PRESS_MS);

        if (selectedContact == 1) {
          currentMode = longPress
            ? Config::STRAIGHT_KEY_VBAND
            : Config::STRAIGHT_KEY_MANIA;
        } else {
          currentMode = longPress
            ? Config::PADDLE_VBAND
            : Config::PADDLE_MANIA;
        }

        break;
      }
    }

    delay(1);
  }

  setStatusLed(false);
  waitForRelease();
}

void assignKeys() {
  activeKey1 = 0;
  activeKey2 = 0;

  switch (currentMode) {
    case Config::STRAIGHT_KEY_MANIA:
      activeKey1 = ' ';
      break;

    case Config::PADDLE_MANIA:
      activeKey1 = Config::MANIA_KEY_1;
      activeKey2 = Config::MANIA_KEY_2;
      break;

    case Config::STRAIGHT_KEY_VBAND:
      activeKey1 = KEY_LEFT_CTRL;
      break;

    case Config::PADDLE_VBAND:
      activeKey1 = KEY_LEFT_CTRL;
      activeKey2 = KEY_RIGHT_CTRL;
      break;
  }
}

void confirmOperatingMode() {
  // Number of short LED flashes:
  // 1 = straight key, Morse Mania
  // 2 = paddle, Morse Mania
  // 3 = straight key, VBand
  // 4 = paddle, VBand
  if (!Config::USE_STATUS_LED) {
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

void updateKey(
  bool contactPressed,
  uint8_t key,
  bool &alreadySent
) {
  // A zero key means this contact is unused in the current mode.
  if (key == 0 || contactPressed == alreadySent) {
    return;
  }

  if (contactPressed) {
    Keyboard.press(key);
  } else {
    Keyboard.release(key);
  }

  alreadySent = contactPressed;
}

