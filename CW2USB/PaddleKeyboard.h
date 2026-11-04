#pragma once

#include <Keyboard.h>
#include "Configuration.h"

// Sends complete two-contact states using Arduino Keyboard's descriptor.
// Do not mix this class with Keyboard.press()/release()/releaseAll():
// the standard Keyboard object maintains a separate report cache.
class PaddleKeyboard {
public:
  PaddleKeyboard();

  void begin();
  void setMode(OperatingMode mode);
  bool update(bool contact1Pressed, bool contact2Pressed);
  bool releaseAll();

private:
  bool sendIfChanged(const KeyReport& report);

  OperatingMode mode_;
  KeyReport lastSentReport_;
  bool haveSentReport_;
};
