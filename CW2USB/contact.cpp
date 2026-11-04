#include "Contact.h"

Contact::Contact(uint8_t pin, unsigned long debounceMs)
  : pin_(pin),
    debounceMs_(debounceMs),
    rawPressed_(false),
    stablePressed_(false),
    lastChangeMs_(0) {
}

void Contact::begin() {
  pinMode(pin_, INPUT_PULLUP);

  rawPressed_ = (digitalRead(pin_) == LOW);
  stablePressed_ = rawPressed_;
  lastChangeMs_ = millis();
}

void Contact::update() {
  const unsigned long now = millis();
  const bool newRawState = (digitalRead(pin_) == LOW);

  if (newRawState != rawPressed_) {
    rawPressed_ = newRawState;
    lastChangeMs_ = now;
  }

  if (now - lastChangeMs_ >= debounceMs_) {
    stablePressed_ = rawPressed_;
  }
}

bool Contact::isPressed() const {
  return stablePressed_;
}
