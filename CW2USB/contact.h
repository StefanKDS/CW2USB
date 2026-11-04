#pragma once

#include <Arduino.h>

// Independent prompt-detection debouncer for one active-LOW contact.
// Sampling is separated from processing so both contacts can be read
// before sending a USB report.
class Contact {
public:
  Contact(uint8_t pin, uint32_t debounceIntervalUs);

  void begin();
  bool readRawPressed() const;
  void update(bool rawPressed, uint32_t nowUs);

  bool isPressed() const;
  bool isReleasedAndStable(uint32_t nowUs) const;

private:
  const uint8_t pin_;
  const uint32_t debounceIntervalUs_;

  bool lastRawPressed_;
  bool pressed_;
  uint32_t lastRawChangeUs_;
};
