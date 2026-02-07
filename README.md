# Tufty NES

A NES (Nintendo Entertainment System) emulator for the **Pimoroni Tufty 2350** badge. ROMs are embedded directly into the firmware -- no SD card needed. Features a multi-ROM selector menu at boot and support for the SparkFun QwSTPad I2C gamepad for full directional control.

> Forked from [xrip/pico-nes](https://github.com/xrip/pico-nes) and adapted for the Tufty 2350 hardware.

## Hardware

| Component | Details |
|---|---|
| **Pimoroni Tufty 2350** | RP2350B (QFN80), 320x240 ST7789 parallel TFT, 16MB flash, 8MB PSRAM |
| **SparkFun QwSTPad** | I2C gamepad (TCA9555 I/O expander), connects via QWIIC/STEMMA QT |

### Pin Map

| Function | GPIO | Notes |
|---|---|---|
| LCD Backlight | 26 | |
| LCD CS | 27 | Chip Select |
| LCD DC | 28 | Data/Command |
| LCD WR | 30 | Write strobe |
| LCD RD | 31 | Read strobe (held high) |
| LCD DB0-DB7 | 32-39 | 8-bit parallel data bus (bit-bang, not PIO) |
| POWER_EN | 41 | Must be HIGH to power display + I2C |
| WL_ON | 23 | Set LOW to disable CYW43 wireless |
| I2C SDA/SCL | 4, 5 | QWIIC connector for QwSTPad |
| Buttons | 7, 9, 10, 11, 6, 22 | A, B, C, UP, DOWN, HOME |
| Rear LEDs | 0, 1, 2, 3 | Used for diagnostics |

### Button Mapping

| Tufty Button | NES | QwSTPad | NES |
|---|---|---|---|
| A (GPIO 7) | A | A | A |
| B (GPIO 9) | B | B | B |
| C (GPIO 10) | Start | Y | Start |
| UP (GPIO 11) | Up | UP | Up |
| DOWN (GPIO 6) | Down | DOWN | Down |
| HOME (GPIO 22) | Select | MINUS / X | Select |
| -- | -- | LEFT | Left |
| -- | -- | RIGHT | Right |
| -- | -- | PLUS | Start |

The Tufty's built-in buttons cover A, B, Start, Select, Up, and Down. The QwSTPad adds **Left and Right** directional inputs, which are required for most games.

## Prerequisites

- **Pico SDK 2.2.0** -- <https://github.com/raspberrypi/pico-sdk>
- **ARM GNU Toolchain 14.2** -- `arm-none-eabi-gcc`
- **CMake 3.13+**
- **picotool 2.2.0** -- `brew install picotool` (macOS) or build from source
- **Python 3** -- for the ROM conversion tool

## Quick Start

```bash
# 1. Clone the repo
git clone https://github.com/lalomorales22/tufty-nes.git
cd tufty-nes

# 2. Drop your .nes ROM files into the ROMs/ directory
#    (ROMs over 1MB will be skipped automatically)

# 3. Generate the ROM table header
python3 tools/convert_roms.py

# 4. Build
mkdir build && cd build
PICO_SDK_PATH=/path/to/pico-sdk cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc \
  -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ \
  -DCMAKE_ASM_COMPILER=arm-none-eabi-gcc \
  ..
make -j4

# 5. Flash (put Tufty in bootloader mode: hold A while plugging USB)
picotool load -f -x ../bin/Release/tufty-nes-TFT-PARALLEL-PWM-305.elf
```

Or use the convenience script:

```bash
./flash_tufty.sh
```

## Building

### Full Build from Scratch

```bash
cd tufty-nes
rm -rf build && mkdir build && cd build
PICO_SDK_PATH=/path/to/pico-sdk cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/path/to/arm-none-eabi-gcc \
  -DCMAKE_CXX_COMPILER=/path/to/arm-none-eabi-g++ \
  -DCMAKE_ASM_COMPILER=/path/to/arm-none-eabi-gcc \
  -DCMAKE_MAKE_PROGRAM=/usr/bin/make \
  ..
make -j4
```

**Important:** You must specify all three compilers (C, CXX, ASM) explicitly, or the ASM compiler may pick up the wrong toolchain (e.g., Homebrew's `arm-none-eabi-gcc` which is missing `nosys.specs`).

### Adding / Removing ROMs

1. Add or remove `.nes` files in the `ROMs/` directory (max 1MB per ROM)
2. Regenerate the header: `python3 tools/convert_roms.py`
3. Rebuild and flash

## Flashing

**Always use `picotool`, not the UF2 mass storage copy method** -- the macOS Finder copy is unreliable for RP2350.

1. Put the Tufty in bootloader mode: **hold A (or BOOT) while plugging in USB**
2. Flash: `picotool load -f -x bin/Release/tufty-nes-TFT-PARALLEL-PWM-305.elf`
3. The device reboots automatically after flashing (`-x` flag)

## How It Works

### Architecture

- **Core 0:** Runs the NES CPU emulator (`InfoNES_Cycle`), ROM selector menu, and game logic
- **Core 1:** Runs the display refresh loop (`refresh_lcd`) at 60fps, reads inputs (buttons + I2C gamepad) every frame
- **Display:** Bit-bang GPIO writes to the ST7789 parallel interface (GPIOs 32-39). PIO doesn't work reliably on RP2350B for GPIOs 32+, so direct GPIO writes are used instead. Performance is 40-55 FPS at 252MHz.
- **I2C Gamepad:** QwSTPad (TCA9555) is read on core1 only, to avoid dual-core I2C bus conflicts. The ROM selector menu reads from a shared `gamepad1_bits` struct.

### Multi-ROM System

ROMs are embedded directly into the firmware binary at compile time:

1. `tools/convert_roms.py` scans `ROMs/*.nes` and generates `src/rom_table.h`
2. Each ROM becomes a `const unsigned char[]` array in flash
3. A `rom_table[]` struct provides name/data/size for the selector menu
4. At boot, the ROM selector menu lets you pick a game with UP/DOWN + A

### Flash Usage (9 ROMs)

```
FLASH:     2,101,360 B / 16 MB  (12.53%)
RAM:         233,752 B / 512 KB (44.58%)
SCRATCH_X:     1,736 B / 4 KB   (42.38%)
SCRATCH_Y:     3,168 B / 4 KB   (77.34%)
```

There's plenty of flash headroom for more ROMs.

## Included ROMs

| ROM | Size |
|---|---|
| 1942.nes | 40 KB |
| bartman.nes | 384 KB |
| excitebike.nes | 24 KB |
| paperboy.nes | 64 KB |
| rcproam.nes | 64 KB |
| simpsons.nes | 384 KB |
| tmnt.nes | 256 KB |
| Tsprbowl.nes | 384 KB |
| tyson.nes | 256 KB |

**Note:** ROM files are not included in this repository. You must supply your own `.nes` files.

## Project Structure

```
tufty-nes/
  boards/
    tufty2350.h          # Board pin definitions
  drivers/
    st7789/              # Display driver (bit-bang parallel)
    graphics/            # Text mode + graphics rendering
    audio/               # Audio (PWM, muted on Tufty)
    nespad/              # NES shift-register pad (unused on Tufty)
    ps2kbd/              # PS/2 keyboard (unused on Tufty)
  infones/               # InfoNES emulator core + mappers
  src/
    main.cpp             # Entry point, input, ROM selector, I2C gamepad
    rom_table.h          # Generated ROM data (not in git)
  tools/
    convert_roms.py      # ROM-to-C-header converter
  ROMs/                  # Your .nes files go here (not in git)
  flash_tufty.sh         # One-command build + flash script
  CMakeLists.txt         # Build configuration
```

## Known Issues

- **Display is upside down** -- MADCTL hardware flip causes color corruption on this panel. Software flip causes SCRATCH_Y overflow. Investigating alternatives.
- **No audio** -- The Tufty 2350 has no speaker/audio output. Sound is silently discarded.

## LED Diagnostics

After boot, the rear LED (GPIO 0) signals QwSTPad I2C status:

- **3 slow blinks** (200ms) = QwSTPad detected and initialized
- **20 rapid blinks** (50ms) = QwSTPad not found on I2C bus -- check cable

## Credits

- [xrip/pico-nes](https://github.com/xrip/pico-nes) -- Original Pico NES emulator
- [InfoNES](https://github.com/jay-kumogata/InfoNES) -- NES emulator core by Jay Kumogata
- [Pimoroni](https://shop.pimoroni.com/products/tufty-2350) -- Tufty 2350 hardware
- [SparkFun QwSTPad](https://www.sparkfun.com/sparkfun-qwstpad.html) -- I2C gamepad

## License

GPL-3.0 -- see [LICENSE](LICENSE).
