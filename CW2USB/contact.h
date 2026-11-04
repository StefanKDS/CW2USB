#ifndef MORSE_PADDLE_CONTACT_H
#define MORSE_PADDLE_CONTACT_H

#include <Arduino.h>

// Independently debounced, active-LOW contact using INPUT_PULLUP.
class Contact {
public:
  Contact(uint8_t pin, unsigned long debounceMs);

  void begin();
  void update();

  bool isPressed() const;

private:
  const uint8_t pin_;
  const unsigned long debounceMs_;

  bool rawPressed_;
  bool stablePressed_;
  unsigned long lastChangeMs_;
};

#endif
