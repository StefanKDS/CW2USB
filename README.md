# Morse Paddle USB Keyboard

Use a **Pro Micro with an ATmega32U4** to connect a Morse paddle or straight key to a computer or Android phone as a USB HID keyboard.

The project is intended for sending practice with **Morse Mania** and also includes keyboard mappings for **VBand**.

Inspired by [barryshaffer/cw2usbkb](https://github.com/barryshaffer/cw2usbkb), originally developed for a different board.

## Features

- Two independent, debounced inputs switching to GND.
- USB keyboard output: a key remains pressed while its contact is closed.
- Independent paddle contacts, including simultaneous operation.
- Four operating modes, with optional selection at startup.
- RX LED used as a status indicator.
- TX LED disabled during sketch operation.
- No keyboard-generated startup messages.

## Hardware and wiring

Use a **classic Pro Micro with an ATmega32U4**, either:
- **5 V / 16 MHz**, or
- **3.3 V / 8 MHz**.

| Connection | Pro Micro pin |
| --- | --- |
| Paddle contact 1 / straight key | `2` |
| Paddle contact 2 | `3` |
| Common contact | `GND` |

The inputs use `INPUT_PULLUP`. Closing a contact connects its input to GND; no external pull-up resistors are required.

For Android, use a USB-host-capable phone, a suitable OTG/host adapter, and a USB data cable.

## Installation

1. Place these files together in a folder named `MorsePaddle`:

   ```text
   MorsePaddle/
   MorsePaddle.ino
   Configuration.h
   Contact.h
   Contact.cpp
   ```

2. Open `MorsePaddle.ino` in the Arduino IDE.
3. Install the **SparkFun AVR Boards** package if required.
4. Select **SparkFun Pro Micro** and the processor option matching your board's voltage and clock speed.
5. Install **Keyboard by Arduino** if it is not already available.
6. Select the port and upload the sketch.

Both Pro Micro variants use the same source code.

## Default operation

**Startup mode selection is disabled by default.** The sketch starts directly in **Morse Mania paddle mode**.

| Input | Output |
| --- | --- |
| Pin `2` connected to GND | `+` |
| Pin `3` connected to GND | `Ä` |
| Both contacts closed | Both keys held |

The receiving device must use a **German / QWERTZ layout for the external USB keyboard** to interpret these physical keys as `+` and `Ã¼`.

Configure the application's input mapping to match. This firmware sends keyboard events; it does not decode Morse code or generate automatic dots and dashes.

## Startup mode selection

### Enable selection first

In `Configuration.h`, change the **startup mode selection setting** from `false` to `true`, then upload the sketch again.

Leave it disabled if you always want to use the configured default mode.

### Select a mode

1. Connect the board or reset it.
2. **Release both paddle contacts.** Selection waits until both inputs are open.
3. Wait for the **RX LED to blink**.
4. Press **one** contact using the table below, then release it.

| Contact | Press duration | Selected mode | Keyboard output during operation |
| --- | --- | --- | --- |
| Pin `2` | Short: less than 250 ms | **1 Morse Mania, straight key** | Pin `2`: Space |
| Pin `3` | Short: less than 250 ms | **2 Morse Mania, paddle** | Pin `2`: `+`; pin `3`: `Ä` |
| Pin `2` | Long: 250 ms or more | **3 VBand, straight key** | Pin `2`: Left Ctrl |
| Pin `3` | Long: 250 ms or more | **4 VBand, paddle** | Pin `2`: Left Ctrl; pin `3`: Right Ctrl |

**For an easy long selection, hold the contact for about one second.** The choice is accepted when you release it.

In either straight-key mode, pin `3` is unused during normal operation.

### Confirmation and start

After selection, the RX LED flashes the selected mode number:

| Flashes | Mode |
| --- | --- |
| 1 | Morse Mania straight key |
| 2 | Morse Mania paddle |
| 3 | VBand straight key |
| 4 | VBand paddle |

Release both contacts and wait for confirmation to finish before starting practice.

- The selection press is **not sent to the connected device**.
- The selected mode is **not saved** across power cycles or resets.
- With startup selection enabled, the board waits for a selection at every startup; there is no automatic selection timeout.
- With startup selection disabled, the configured default mode is used.

## LED behavior

- **Blinking RX LED:** startup selection is waiting for input.
- **One to four short RX flashes:** operating-mode confirmation.
- **RX LED during normal use:** indicates an active keyboard output.
- **TX LED:** disabled by the sketch after `setup()` starts.

The USB core can also affect the RX LED, so it is not an exclusive paddle indicator. The TX LED may still blink during reset, bootloader operation, or upload.

The LED configuration assumes the classic SparkFun-compatible Pro Micro wiring. Disable the status LED setting in `Configuration.h` if your board differs.

## Configuration

Edit `Configuration.h` to adjust:

- Input pins.
- Default operating mode.
- Startup mode selection.
- Morse Mania key assignments.
- Contact debounce time; default: **5 ms**.
- Short/long selection threshold; default: **250 ms**.
- Status LED use.

The contact class is split into `Contact.h` and `Contact.cpp`. Arduino setup, mode selection, and keyboard handling remain in `MorsePaddle.ino`.

## Troubleshooting

### Mode selection does not appear

It is disabled by default. Enable it in `Configuration.h` and upload again. Also release both contacts after connecting the board.

### Selection appears stuck

Release both contacts first. Then press and release only one contact. A mode is not accepted until the selected contact is released.

### The output is `]` and `[` instead of `+` and `Ä`

Set the **external physical keyboard layout** on the receiving device to **German / QWERTZ**.

### The straight key does not respond

Connect it between pin `2` and GND. Pin `3` is unused in straight-key modes.

### No input reaches the phone

Check USB host/OTG support, the adapter, and the data cable. Test briefly in a text field using the default Morse Mania paddle mode before testing the app.

### A key repeats while held

The firmware intentionally holds the HID key down until the contact opens. A text editor or operating system may generate repeated characters; this does not mean the sketch is repeatedly calling a key press.
