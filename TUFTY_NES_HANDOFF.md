# Tufty 2350 NES Emulator - Project Handoff

## Goal
Flash a Pimoroni Tufty 2350 (RP2350B-based badge) with a NES emulator that runs on its built-in ST7789 parallel TFT LCD. No SD card slot - ROMs are embedded directly into the firmware. Multi-ROM selector menu at boot.

## Hardware
- **Pimoroni Tufty 2350** - RP2350B (QFN80) chip, 320x240 ST7789 parallel TFT display, 6 built-in buttons (A, B, C, UP, DOWN, HOME), **CYW43 wireless chip** (like Pico 2 W), 16MB flash, 8MB PSRAM
- **QwSTPad** - I2C gamepad controller (TCA9555 I/O expander at address 0x21, connected via I2C0 on GPIO 4 & 5). Provides LEFT/RIGHT/A/B/X/Y/+/- buttons
- **Gyro/Accel sensor** - attached via I2C
- **Temp/Pressure/Humidity sensor** - attached via I2C
- **Lux/Proximity sensor** - attached via I2C

## Project Location
- **NES emulator source:** `~/Desktop/tufty/pico-nes/`
- **ELF output:** `~/Desktop/tufty/pico-nes/bin/Release/tufty-nes-TFT-PARALLEL-PWM-305.elf`
- **This doc:** `~/Desktop/tufty/TUFTY_NES_HANDOFF.md`

## Key Source Files
| File | Purpose |
|---|---|
| `boards/tufty2350.h` | Board pin definitions (display, buttons, I2C, etc.) |
| `src/main.cpp` | Main entry, input handling, ROM selector menu, ROM loading, core1 render loop |
| `src/rom_table.h` | **Generated** - 9 NES ROMs as C byte arrays + lookup table |
| `tools/convert_roms.py` | Converts `ROMs/*.nes` files to `src/rom_table.h` |
| `ROMs/` | Directory containing .nes ROM files (9 ROMs, ~1.9MB total) |
| `drivers/st7789/st7789.c` | **Display driver** - graphics_init(), refresh_lcd(), ST7789 init sequence |
| `drivers/st7789/st7789.h` | Display constants, textmode_palette[16], TEXTMODE_COLS=53, TEXTMODE_ROWS=30 |
| `drivers/graphics/graphics.c` | draw_text(), draw_window() for text mode rendering |
| `drivers/graphics/graphics.h` | Graphics API, graphics_mode_t enum (TEXTMODE_DEFAULT, GRAPHICSMODE_DEFAULT) |
| `CMakeLists.txt` | Build config - sets TFT, TFT_PARALLEL, INVERSION, TUFTY2350 |

## Official Pimoroni Pin Definitions (VERIFIED)
| Function | GPIO | Notes |
|---|---|---|
| LCD_BACKLIGHT | 26 | Backlight LED |
| LCD_CS | 27 | Chip Select |
| LCD_RS (DC) | 28 | Data/Command |
| LCD_WR | 30 | Write strobe |
| LCD_RD | 31 | Read strobe (held high) |
| LCD_DB0-DB7 | 32-39 | 8-bit parallel data bus |
| WL_ON | 23 | CYW43 power enable |
| WL_D | 24 | CYW43 SPI data |
| WL_CS | 25 | CYW43 SPI chip select |
| WL_CLK | 29 | CYW43 SPI clock |
| POWER_EN | 41 | Power enable (for I2C/display power) |
| Buttons | 6,7,9,10,11,22 | DOWN,A,B,C,UP,HOME |
| Rear LEDs | 0,1,2,3 | White LEDs on back |
| I2C SDA/SCL | 4,5 | QWIIC/STEMMA QT |

## Build Environment
- **Pico SDK:** `/Users/minibrain/pico-sdk` (version 2.2.0)
- **ARM Toolchain:** `/Users/minibrain/arm-toolchain/bin/arm-none-eabi-gcc` (14.2.1)
- **picotool:** `/opt/homebrew/bin/picotool` (2.2.0, installed via brew)

### How to Build
```bash
cd ~/Desktop/tufty/pico-nes
rm -rf build && mkdir build && cd build
PICO_SDK_PATH=/Users/minibrain/pico-sdk cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/Users/minibrain/arm-toolchain/bin/arm-none-eabi-gcc \
  -DCMAKE_CXX_COMPILER=/Users/minibrain/arm-toolchain/bin/arm-none-eabi-g++ \
  -DCMAKE_ASM_COMPILER=/Users/minibrain/arm-toolchain/bin/arm-none-eabi-gcc \
  -DCMAKE_MAKE_PROGRAM=/usr/bin/make ..
/usr/bin/make -j4
```
**IMPORTANT:** Must specify all three compilers (C, CXX, ASM) explicitly or the ASM compiler picks up the Homebrew arm-none-eabi-gcc which is missing `nosys.specs`.

### How to Regenerate ROM Table
```bash
cd ~/Desktop/tufty/pico-nes
python3 tools/convert_roms.py
# Then rebuild
```
This scans `ROMs/*.nes`, skips files >1MB, generates `src/rom_table.h`.

## How to Flash (USE PICOTOOL, NOT cp)
**Do NOT use the cp/UF2 mass storage method** - it's unreliable on macOS.
```bash
# Put device in bootloader mode (hold BOOT + press RESET, or hold A while plugging USB)
# Then:
picotool load -f -x bin/Release/tufty-nes-TFT-PARALLEL-PWM-305.elf
# -f = force, -x = execute (auto-reboot after flash)
```

## Current Status: MULTI-ROM MENU WORKS, TWO OPEN ISSUES

### What Works
- **ROM selector menu at boot** - shows 9 ROMs, navigate with UP/DOWN, select with A
- **Games load and run** with correct colors from the menu
- **Tufty built-in buttons** work in both menu and games (A, B, C, UP, DOWN, HOME)
- **Multi-ROM support** - 9 ROMs embedded, 2.1MB flash usage (12.5% of 16MB)
- **Text mode rendering** for the menu (TEXTMODE_DEFAULT + draw_text())
- **`InfoNES_Menu()` returns 0** so `InfoNES_Video()` calls `parseROM()` on game switch
- **Core1 render loop** handles both text mode (menu) and graphics mode (game)
- **`sleep_ms(200)` before `tufty_rom_select()`** is REQUIRED - without it, the menu tries to use the LCD before core1's `graphics_init()` finishes, causing a blank screen
- **I2C calls use timeouts** (`i2c_write_blocking_until` / `i2c_read_blocking_until` with 5-50ms timeouts) to prevent bus lockups from freezing the system

### What Does NOT Work

#### 1. QwSTPad I2C Controller Not Responding
- The QwSTPad (TCA9555 at I2C0 address 0x21) buttons are not registering during gameplay
- `qwstpad_init()` runs early in `main()` before `multicore_launch_core1()`
- `qwstpad_read()` is called from `tufty_input_tick()` on core1 every frame
- I2C timeout calls were added (5ms for reads, 50ms for init) - didn't fix it
- **Previously had a bug:** the ROM selector menu called `qwstpad_read()` directly from core0 while core1 was also reading I2C - dual-core I2C access corrupted the bus. **This was fixed** - menu now reads `gamepad1_bits` (populated by core1) instead.
- **Things to investigate:**
  - Is `qwstpad_connected` actually being set to true? Add debug (LED blink) after `qwstpad_init()`
  - Is the QwSTPad physically connected and powered? Check the QWIIC cable
  - Does the QwSTPad need GPIO 41 (POWER_EN) to be high before I2C init? Currently GPIO 41 is set high before `qwstpad_init()`, so this should be fine
  - Try adding `sleep_ms(100)` after GPIO 41 power enable, before `qwstpad_init()`, in case the I2C peripherals need time to power up
  - Try a standalone I2C scan to verify the device is responding at address 0x21

#### 2. Display is Upside Down (180 degrees rotated)
- The ST7789 display renders correctly but the image is upside down
- **MADCTL hardware flip DOES NOT WORK on this panel:**
  - Original MADCTL: `MX | MV` (0x60) = works but upside down
  - Tried `MY | MV` (0xA0) = 180° flip but **ALL COLORS TURN GREEN** - unusable
  - Tried `MY | MX | MV` (0xE0) = not tested yet
  - The green color issue is specific to changing MADCTL on this Tufty panel
- **Software flip in refresh_lcd() DOES NOT WORK:**
  - Tried reversing pixel iteration order in `refresh_lcd()` (read bitmap backwards for graphics mode, reverse Y/X/bit order for text mode)
  - Result: **screen goes dark** (small white flash then nothing)
  - The `refresh_lcd()` function is placed in SCRATCH_Y (`__scratch_y("refresh_lcd")`) and is marked `__inline` - the reversed iteration may confuse the compiler/linker for this memory section
  - SCRATCH_Y is at 77.34% capacity (3168 of 4096 bytes)
- **Things to investigate:**
  - Try `MY | MX | MV` (0xE0) MADCTL - this hasn't been tested yet
  - Try `MY | MV | BGR` (0xA8) - maybe BGR bit fixes the green
  - Try software flip WITHOUT `__inline` / `__scratch_y` attributes (move to regular flash)
  - Try flipping just the buffer contents in `InfoNES_PostDrawLine()` instead of in `refresh_lcd()` - write pixels in reverse order into SCREEN buffer
  - Check Pimoroni's own ST7789 driver to see what MADCTL they use for this panel

## Architecture Notes

### Multi-ROM System
- `tools/convert_roms.py` scans `ROMs/*.nes`, skips >1MB, generates `src/rom_table.h`
- `rom_table.h` contains: individual `const unsigned char romname_data[]` arrays + `struct rom_entry { name, data, size }` table + `ROM_COUNT` define
- `main.cpp` includes `rom_table.h` (guarded by `#ifdef TUFTY2350`)
- Global `rom` pointer and `current_rom_index` are updated by `tufty_rom_select()`

### ROM Selector Menu (`tufty_rom_select()` in main.cpp)
- Calls `graphics_set_mode(TEXTMODE_DEFAULT)` to enable text rendering on core1
- Uses `draw_text()` to write ROM names to the SCREEN/text buffer
- Highlights current selection: yellow on blue (color 14 on bg 1), others white on black
- Reads input from `gamepad1_bits` struct (populated by core1's `tufty_input_tick()`) - **NOT** direct I2C/GPIO reads, to avoid dual-core I2C conflicts
- On A press: sets `rom` pointer and `current_rom_index`, switches to `GRAPHICSMODE_DEFAULT`, returns
- Called from `main()` before `parseROM()`, and from `InfoNES_Menu()` for game switching

### Display Rendering (core1)
- `render_core()` on core1: calls `graphics_init()`, sets up buffers, enters 60fps loop
- `refresh_lcd()` in SCRATCH_Y: renders either TEXTMODE_DEFAULT or GRAPHICSMODE_DEFAULT
- Text mode: 53 cols x 30 rows, 6x8 font, color byte = (bg<<4 | fg), palette index 0-15
- Graphics mode: reads from `SCREEN[240][256]` buffer, looks up `palette[]` for RGB565 colors
- Display uses **bit-bang GPIO** (NOT PIO) for parallel data writes to GPIOs 32-39
- Bit-bang functions must be `__attribute__((noinline))` to fit in SCRATCH_Y

### Critical Timing
- `sleep_ms(200)` before `tufty_rom_select()` in `main()` is **essential** - core1's `graphics_init()` takes ~40ms+ (LCD init sequence has delays), and calling `graphics_set_mode()` from core0 before that finishes causes a blank screen crash
- The semaphore between core0/core1 does NOT synchronize init completion - it only gates when core1 starts its render loop

### Key Fixes Previously Applied
1. **GPIO 41 (POWER_EN) set HIGH** - powers the display and I2C bus
2. **GPIO 23 (WL_ON) set LOW** - disables CYW43 wireless interference
3. **Bit-bang display driver** - PIO doesn't work on RP2350B for GPIO 32-39 data bus
4. **#undef PICO_DEFAULT_LED_PIN** before redefine (pico2.h sets it to 25=WL_CS)

### InfoNES Flow
- `main()` → `tufty_rom_select()` → `parseROM()` → `InfoNES_Main(true)`
- `InfoNES_Main(skip_fb=true)` → skips `InfoNES_Menu()` → calls `InfoNES_Video()` → `parseROM()` + `InfoNES_Reset()` → `InfoNES_Cycle()` (game loop)
- When game exits: `InfoNES_Main(skip_fb=false)` → calls `InfoNES_Menu()` which returns 0 → `InfoNES_Video()` re-parses ROM
- **`InfoNES_Menu()` MUST return 0** (not 1) so that `InfoNES_Video()` gets called to run `parseROM()` on the newly selected ROM

## Button Mapping
| Tufty Button | NES | QwSTPad | NES |
|---|---|---|---|
| A (GPIO 7) | A | A | A |
| B (GPIO 9) | B | B | B |
| C (GPIO 10) | Start | Y | Start |
| UP (GPIO 11) | Up | UP | Up |
| DOWN (GPIO 6) | Down | DOWN | Down |
| HOME (GPIO 22) | Select | MINUS/X | Select |
| - | - | LEFT | Left |
| - | - | RIGHT | Right |
| - | - | PLUS | Start |

## ROMs Included (9 total, ~1.9MB)
| ROM | Size | Notes |
|---|---|---|
| 1942.nes | 40KB | |
| Tsprbowl.nes | 384KB | Tecmo Super Bowl |
| bartman.nes | 384KB | |
| excitebike.nes | 24KB | |
| paperboy.nes | 64KB | |
| rcproam.nes | 64KB | RC Pro-Am |
| simpsons.nes | 384KB | |
| tmnt.nes | 256KB | Teenage Mutant Ninja Turtles |
| tyson.nes | 256KB | Mike Tyson's Punch-Out!! |
| ~~62.nes~~ | ~~1.5MB~~ | Skipped (>1MB limit) |

## Flash Usage
```
FLASH:     2,101,360 B / 16 MB  (12.53%)
RAM:         233,752 B / 512 KB (44.58%)
SCRATCH_X:     1,736 B / 4 KB   (42.38%)
SCRATCH_Y:     3,168 B / 4 KB   (77.34%)
```
