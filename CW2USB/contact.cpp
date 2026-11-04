#include "Contact.h"

Contact::Contact(uint8_t pin, uint32_t debounceIntervalUs)
    : pin_(pin),
      debounceIntervalUs_(debounceIntervalUs),
      lastRawPressed_(false),
      pressed_(false),
      lastRawChangeUs_(0) {
}

void Contact::begin() {
  pinMode(pin_, INPUT_PULLUP);
  lastRawPressed_ = readRawPressed();
  pressed_ = lastRawPressed_;
  lastRawChangeUs_ = micros();
}

bool Contact::readRawPressed() const {
  return digitalRead(pin_) == LOW;
}

void Contact::update(bool rawPressed, uint32_t nowUs) {
  // Check BEFORE resetting the raw-change timestamp. If the previous
  // raw level was quiet long enough, accept its next edge immediately.
  const uint32_t quietTimeUs = nowUs - lastRawChangeUs_;

  if (rawPressed != pressed_ && quietTimeUs >= debounceIntervalUs_) {
    pressed_ = rawPressed;
  }

  // Repeated bounce edges postpone acceptance of further transitions
  // until the raw signal has settled again.
  if (rawPressed != lastRawPressed_) {
    lastRawPressed_ = rawPressed;
    lastRawChangeUs_ = nowUs;
  }
}

bool Contact::isPressed() const {
  return pressed_;
}

bool Contact::isReleasedAndStable(uint32_t nowUs) const {
  return !pressed_ &&
         !lastRawPressed_ &&
         static_cast<uint32_t>(nowUs - lastRawChangeUs_) >=
             debounceIntervalUs_;
}
