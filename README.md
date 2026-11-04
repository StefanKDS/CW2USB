# MorsePaddle

USB HID paddle / straight-key adapter for the **classic ATmega32U4 Pro Micro**,
built with the Arduino IDE.

The adapter exposes contact closures as keyboard key-down and key-up states.
It supports Morse Mania and VBand mappings, optional startup mode selection,
prompt-detection debouncing, and one combined keyboard report for both contacts.

**This is an input adapter, not a local CW keyer.** It does not generate timed
dots/dashes, element spacing, or a local sidetone. The receiving application
performs those functions.

## Hardware

- Classic ATmega32U4 Pro Micro: 5 V / 16 MHz or 3.3 V / 8 MHz.
- Two-contact paddle, or a straight key connected to contact 1.
- USB data cable. Android additionally needs USB host/OTG support.

This version is not intended for RP2040, SAMD21, or other boards also marketed
under similar names.

## Wiring

| Signal | Pro Micro connection |
| --- | --- |
| Paddle contact 1 / straight key | Pin 2 |
| Paddle contact 2 | Pin 3 |
| Common | GND |

Both contacts close to GND. The firmware enables internal pull-up resistors.
For a straight key, leave pin 3 unconnected.

Use isolated mechanical contacts. Do not apply external voltage to these inputs
or connect them in parallel with a transmitter/keyer input without a suitable
isolated interface.

## Files

| File | Purpose |
| --- | --- |
| `MorsePaddle.ino` | Startup, mode selection, input sampling, LED handling |
| `Configuration.h` | Pins, default mode, debounce interval, key mappings |
| `Contact.h` / `Contact.cpp` | Independent prompt-detection contact debouncer |
| `PaddleKeyboard.h` / `PaddleKeyboard.cpp` | Combined HID reports and report cache |
| `README.md` | Setup and usage |

Class implementations are C++, so they use `.cpp`, not `.c`.

## Arduino IDE setup

1. Extract the ZIP and keep the source files together in the `MorsePaddle` folder.
2. Open `MorsePaddle.ino`.
3. Install the SparkFun AVR Boards package if required.
4. Select **SparkFun Pro Micro** and the CPU setting matching the physical board:
   - **ATmega32U4 (5V, 16 MHz)**, or
   - **ATmega32U4 (3.3V, 8 MHz)**.
5. Install/select the standard **Keyboard library by Arduino**.
6. Review `Configuration.h`, then upload.

No Bounce2 dependency is required: `Contact` implements the prompt-detection
approach itself.

The report sender intentionally targets the standard Arduino Keyboard library
and Arduino AVR core. It relies on their keyboard report ID `2` and eight-byte
`KeyReport` format. It is not a drop-in backend for HID-Project or other libraries.

## Default behavior

Startup selection is **disabled by default**:

```cpp
constexpr bool ENABLE_STARTUP_MODE_SELECTION = false;
constexpr OperatingMode DEFAULT_MODE = OperatingMode::PaddleMania;
```

The board immediately uses the fixed default mode after its startup sequence.
It does not interpret a startup paddle gesture unless selection is enabled.

With the default mapping:

| Contact | Keyboard state |
| --- | --- |
| Pin 2 closed | Physical key producing `+` on a German layout |
| Pin 3 closed | Physical key producing `ü` on a German layout |
| Contact opens | Corresponding key released |
| Both closed | Both keys held |

The host must use **German / QWERTZ for this external keyboard** for the default
physical key positions to produce `+` and `ü`. With a US layout they produce
`]` and `[`. These are physical HID usages, not Unicode text.

The host may repeat characters while a key is held in a text editor. The adapter
does not generate repeated press/release pulses.

## Startup mode selection

### 1. Enable it before uploading

In `Configuration.h`, change:

```cpp
constexpr bool ENABLE_STARTUP_MODE_SELECTION = true;
```

Upload the modified project. This is a compile-time option, not a setting changed
by the startup gesture itself.

### 2. Connect the board with both contacts released

There is a startup pause. Once both contacts are released and stable, the RX LED
blinks to indicate selection is active.

If a contact is held while connecting, release it first. A held-at-boot contact
does **not** directly select a mode.

### 3. Press and release exactly one contact

| Contact | Duration | Selected mode | Output mapping | Confirmation |
| --- | --- | --- | --- | --- |
| Pin 2 | Short: below 250 ms | StraightKeyMania | Pin 2 -> Space | 1 blink |
| Pin 3 | Short: below 250 ms | PaddleMania | Pin 2 -> `+`, pin 3 -> `ü`* | 2 blinks |
| Pin 2 | Long: at least 250 ms | StraightKeyVBand | Pin 2 -> Left Ctrl | 3 blinks |
| Pin 3 | Long: at least 250 ms | PaddleVBand | Pin 2 -> Left Ctrl, pin 3 -> Right Ctrl | 4 blinks |

\* With a German host keyboard layout and the default physical key usages.

Selection is finalized **when the chosen contact is released**, not merely when
the long-press threshold is reached. For a clearly long gesture, hold it for
about one second. Do not press both contacts during selection; if both are first
detected together, pin 2 takes priority.

### 4. Wait for the confirmation blinks

The RX LED flashes the selected mode number, then both contacts must be released
and stable before normal operation starts.

The selection gesture is **not sent to the host as a keyboard action**.
Do not begin sending during the startup/confirmation sequence.

### 5. Reconnect or reset to select again

The selected mode is kept in RAM only. It is not saved in EEPROM. When startup
selection is enabled, each reset or power-up requires a new selection.

There is no selection timeout: the adapter waits until a gesture is completed.

If `ENABLE_STATUS_LED` is `false`, selection still works, but without LED prompts.

## Fixed VBand mode without startup selection

For an adapter used only with a VBand paddle, set:

```cpp
constexpr bool ENABLE_STARTUP_MODE_SELECTION = false;
constexpr OperatingMode DEFAULT_MODE = OperatingMode::PaddleVBand;
```

VBand accepts Left Ctrl and Right Ctrl as the two paddle inputs. Configure an
appropriate paddle/keyer mode in VBand as well. Its browser tab must have focus.

For a straight key, choose `OperatingMode::StraightKeyVBand` instead and connect
the key between pin 2 and GND.

## Latency-oriented changes

### Prompt-detection debouncing

The previous stable-input approach always waited for a newly observed state to
settle before accepting it. This version accepts the first observed transition
immediately **if the preceding raw level was quiet for at least the configured
interval**.

Further bounce transitions are suppressed until the raw input has settled again.
Both closing and opening edges use the same independent per-contact logic.

Default:

```cpp
constexpr uint32_t DEBOUNCE_INTERVAL_US = 5000UL;
```

This is not a guarantee of zero latency. Very brief pulses or transitions during
bouncing may still be delayed, stretched, or filtered. Reducing the interval
reduces filtering and may admit false transitions.

For a short diagnostic test only, `0` disables filtering. Restore filtering if
contact bounce produces extra events. Microsecond timestamps improve the timer
resolution; they do not guarantee microsecond delivery to the application.

### One combined keyboard report

Both contacts are sampled and processed before a USB send. If the effective
keyboard state changes, one complete report describes both states, instead of
separate `Keyboard.press()` / `Keyboard.release()` transfers for each contact.

Unchanged reports are not resent. Failed transfers are not cached as successful,
so a later loop retries the current state. There is no queue preserving every
edge during a disconnected or stalled USB connection.

The operating system can still emit individual key events from a combined
report. This is not a guarantee of simultaneous browser events.

### No deliberate wait in normal operation

`loop()` contains no `delay()`, serial logging, or LED blink sequence.
Startup selection and confirmation may block, intentionally.

Arduino's USB send can still wait internally. This firmware is not a hard
real-time input recorder, and firmware changes cannot remove browser scheduling
or audio buffering latency.

## LEDs

- **Power LED:** unchanged.
- **RX LED / pin 17:** startup indication, mode confirmation, and activity.
  Arduino's USB core also writes this LED, so it is not an exclusive status
  indicator and unrelated USB activity can affect it.
- **TX LED / PD5:** disabled after `setup()` begins by configuring PD5 as an input.
  The bootloader may still blink it during reset/upload.

For board clones with different LED wiring, review the LED settings first.

```cpp
constexpr bool ENABLE_STATUS_LED = true;
constexpr bool DISABLE_TX_LED = true;
```

Setting `ENABLE_STATUS_LED` to `false` stops this sketch's RX LED writes; it does
not disable USB-core RX activity indication.

## Troubleshooting

### VBand still feels delayed, even with a normal keyboard

Firmware optimization cannot resolve latency elsewhere in the chain. VBand
documents USB, browser JavaScript, and audio buffering as possible contributors.
Compare in the Practice Channel, avoid Bluetooth/HDMI audio for the comparison,
and check the application's keyer mode and spacing settings.

### No input after connecting

- With startup selection enabled, complete a selection gesture and wait for the
  confirmation blinks.
- Release any contact held during startup.
- Check that the board is in the intended mode.
- Check the USB data cable, host/OTG support, and browser focus.

### Unexpected characters in a text editor

Check the external keyboard's host layout. The default Morse Mania mapping is
intended for German QWERTZ. VBand mode sends Ctrl modifiers, not visible text.

## Validation status

The C++ sources passed a desktop compiler check using Arduino/HID test stubs.
Logic checks covered prompt edges, bouncing, short-pulse settling, disabled
filtering, timer rollover, combined reports, unchanged-state suppression,
failed-transfer retry, and all four output mappings.

**No AVR-toolchain compilation or physical Pro Micro / Android / VBand test has
been performed for this version.** Stub tests do not validate the USB descriptor,
board package, electrical timing, or host-side behavior.

## References

- Original functional inspiration: https://github.com/barryshaffer/cw2usbkb
- Prompt-detection reference: https://github.com/thomasfredericks/Bounce2
- Arduino Keyboard implementation: https://github.com/arduino-libraries/Keyboard
- Arduino AVR HID implementation:
  https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/HID
- SparkFun Pro Micro board definitions:
  https://github.com/sparkfun/Arduino_Boards
- VBand usage and latency troubleshooting: https://hamradio.solutions/vband/

This is an independent hobby project, not an official VBand adapter.
