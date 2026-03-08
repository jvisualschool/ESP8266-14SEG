# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino sketch for ESP8266 NodeMCU v2 controlling an 8-digit 14-segment LED display (two HT16K33 modules) and a 0.96" OLED (SSD1306) as a dashboard.

## Build & Upload

Use Arduino IDE (not a Makefile/CLI build system):
1. Open `ESP8266-14Seg.ino` in Arduino IDE
2. Board: `NodeMCU 1.0 (ESP-12E Module)`
3. Upload speed: 115200
4. Monitor serial output at 115200 baud

Required libraries (Arduino Library Manager):
- Adafruit GFX Library
- Adafruit SSD1306
- Adafruit BusIO

The HT16K33 14-segment driver is implemented inline (no external library needed).

## Hardware Architecture

**Dual independent I2C buses** — required to prevent bus lock-up on ESP8266 when mixing devices:

| Bus | Type | Pins | Device |
|-----|------|------|--------|
| Bus A | Hardware I2C (`Wire`) | D5(SDA), D6(SCL) | OLED SSD1306 @ 0x3C |
| Bus B | Software bit-bang | D1(SCL), D2(SDA) | 14-Seg A (right) @ 0x70 |
| Bus C | Software bit-bang | D7(SCL), D0(SDA) | 14-Seg B (left) @ 0x70 |

Both 14-seg modules share the same I2C address (0x70) because they are on separate physical buses.

## Key Code Concepts

**`ROTATE_DISPLAYS true`**: Enables 180-degree rotation mode for both displays. When enabled:
- OLED uses `setRotation(2)`
- 14-seg characters are transformed via `rotateCharacter180()` which swaps segment bits for physical upside-down mounting
- Character ordering is reversed: module A gets indices 0–3 reversed, module B gets indices 4–7 reversed

**`HT16K33_SoftBus` class**: Custom software I2C implementation. `begin()` returns `false` if device is not detected. The globals `aReady`/`bReady`/`oReady` gate all display output.

**`displayText8(const char*)`**: Writes an 8-character string across both modules. Characters beyond the string length are space-padded.

**Font data**: `alphafonttable[]` stored in PROGMEM, indexed by ASCII value. 14-bit segment encoding per character.
