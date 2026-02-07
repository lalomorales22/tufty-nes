#include <cstdio>
#include <cstring>
#include <cstdarg>

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include <hardware/sync.h>
#include <hardware/flash.h>

#include "main.h"

#include <InfoNES.h>
#include <InfoNES_System.h>

#include "InfoNES_Mapper.h"

#include "graphics.h"

#include "audio.h"

#ifdef TUFTY2350
#include "rom_table.h"
#endif

#ifndef TUFTY2350
#include "ff.h"
#if USE_PS2_KBD
#include "ps2kbd_mrmltr.h"
#endif
#if USE_NESPAD
#include "nespad.h"
#endif
#endif

#include <bsp/board_api.h>

#pragma GCC optimize("Ofast")

#define HOME_DIR (char*)"\\NES"

#ifndef TUFTY2350
// Original flash-based ROM loading
#ifndef BUILD_IN_GAMES
#define FLASH_TARGET_OFFSET (1024 * 1024)
const char* rom_filename = (const char *)(XIP_BASE + FLASH_TARGET_OFFSET);
const uint8_t* rom = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET) + 4096;
size_t get_rom4prog_size() { return FLASH_TARGET_OFFSET - 4096; }

class Decoder {
};
#else
#include "lzwSource.h"
#include "lzw.h"
size_t get_rom4prog_size() { return sizeof(rom); }
#endif
uint32_t get_rom4prog() { return (uint32_t)rom; }
#endif // !TUFTY2350

#ifdef TUFTY2350
// Multi-ROM support - pointer set by ROM selector menu
static int current_rom_index = 0;
const uint8_t* rom = rom_table[0].data;
size_t get_rom4prog_size() { return rom_table[current_rom_index].size; }
uint32_t get_rom4prog() { return (uint32_t)rom; }
#endif

bool cursor_blink_state = false;
uint8_t CURSOR_X, CURSOR_Y = 0;
uint8_t manager_started = false;

enum menu_type_e {
    NONE,
    INT,
    TEXT,
    ARRAY,
    SAVE,
    LOAD,
    RESET,
    RETURN,
    ROM_SELECT,
    USB_DEVICE
};

struct semaphore vga_start_semaphore;
uint8_t SCREEN[NES_DISP_HEIGHT][NES_DISP_WIDTH]; // 61440 bytes
uint16_t linebuffer[256];

SETTINGS settings = {
    .version = 3,
    .show_fps = false,
    .flash_line = true,
    .flash_frame = true,
    .palette = RGB333,
    .snd_vol = 4,
    .player_1_input = COMBINED,
    .player_2_input = GAMEPAD1,
    .nes_palette = 0,
    .swap_ab = false,
};

#ifndef TUFTY2350
static FATFS fs, fs1;
FATFS* getFlashInDriveFATFSptr() { return &fs1; }
FATFS* getSDCardFATFSptr() { return &fs; }
#endif

i2s_config_t i2s_config;

char fps_text[3] = { "0" };
int start_time;
int frames;

const BYTE NesPalette[64] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f
};

const int __not_in_flash_func(NesPalette888)[] = {
    RGB888(0x7c, 0x7c, 0x7c), RGB888(0x00, 0x00, 0xfc), RGB888(0x00, 0x00, 0xbc), RGB888(0x44, 0x28, 0xbc),
    RGB888(0x94, 0x00, 0x84), RGB888(0xa8, 0x00, 0x20), RGB888(0xa8, 0x10, 0x00), RGB888(0x88, 0x14, 0x00),
    RGB888(0x50, 0x30, 0x00), RGB888(0x00, 0x78, 0x00), RGB888(0x00, 0x68, 0x00), RGB888(0x00, 0x58, 0x00),
    RGB888(0x00, 0x40, 0x58), RGB888(0x00, 0x00, 0x00), RGB888(0x00, 0x00, 0x00), RGB888(0x00, 0x00, 0x00),
    RGB888(0xbc, 0xbc, 0xbc), RGB888(0x00, 0x78, 0xf8), RGB888(0x00, 0x58, 0xf8), RGB888(0x68, 0x44, 0xfc),
    RGB888(0xd8, 0x00, 0xcc), RGB888(0xe4, 0x00, 0x58), RGB888(0xf8, 0x38, 0x00), RGB888(0xe4, 0x5c, 0x10),
    RGB888(0xac, 0x7c, 0x00), RGB888(0x00, 0xb8, 0x00), RGB888(0x00, 0xa8, 0x00), RGB888(0x00, 0xa8, 0x44),
    RGB888(0x00, 0x88, 0x88), RGB888(0x00, 0x00, 0x00), RGB888(0x00, 0x00, 0x00), RGB888(0x00, 0x00, 0x00),
    RGB888(0xf8, 0xf8, 0xf8), RGB888(0x3c, 0xbc, 0xfc), RGB888(0x68, 0x88, 0xfc), RGB888(0x98, 0x78, 0xf8),
    RGB888(0xf8, 0x78, 0xf8), RGB888(0xf8, 0x58, 0x98), RGB888(0xf8, 0x78, 0x58), RGB888(0xfc, 0xa0, 0x44),
    RGB888(0xf8, 0xb8, 0x00), RGB888(0xb8, 0xf8, 0x18), RGB888(0x58, 0xd8, 0x54), RGB888(0x58, 0xf8, 0x98),
    RGB888(0x00, 0xe8, 0xd8), RGB888(0x78, 0x78, 0x78), RGB888(0x00, 0x00, 0x00), RGB888(0x00, 0x00, 0x00),
    RGB888(0xfc, 0xfc, 0xfc), RGB888(0xa4, 0xe4, 0xfc), RGB888(0xb8, 0xb8, 0xf8), RGB888(0xd8, 0xb8, 0xf8),
    RGB888(0xf8, 0xb8, 0xf8), RGB888(0xf8, 0xa4, 0xc0), RGB888(0xf0, 0xd0, 0xb0), RGB888(0xfc, 0xe0, 0xa8),
    RGB888(0xf8, 0xd8, 0x78), RGB888(0xd8, 0xf8, 0x78), RGB888(0xb8, 0xf8, 0xb8), RGB888(0xb8, 0xf8, 0xd8),
    RGB888(0x00, 0xfc, 0xfc), RGB888(0xf8, 0xd8, 0xf8), RGB888(0x00, 0x00, 0x00), RGB888(0x00, 0x00, 0x00),
    /* Matthew Conte's Palette */
    RGB888(0x80, 0x80, 0x80), RGB888(0x00, 0x00, 0xbb), RGB888(0x37, 0x00, 0xbf), RGB888(0x84, 0x00, 0xa6),
    RGB888(0xbb, 0x00, 0x6a), RGB888(0xb7, 0x00, 0x1e), RGB888(0xb3, 0x00, 0x00), RGB888(0x91, 0x26, 0x00),
    RGB888(0x7b, 0x2b, 0x00), RGB888(0x00, 0x3e, 0x00), RGB888(0x00, 0x48, 0x0d), RGB888(0x00, 0x3c, 0x22),
    RGB888(0x00, 0x2f, 0x66), RGB888(0x00, 0x00, 0x00), RGB888(0x05, 0x05, 0x05), RGB888(0x05, 0x05, 0x05),
    RGB888(0xc8, 0xc8, 0xc8), RGB888(0x00, 0x59, 0xff), RGB888(0x44, 0x3c, 0xff), RGB888(0xb7, 0x33, 0xcc),
    RGB888(0xff, 0x33, 0xaa), RGB888(0xff, 0x37, 0x5e), RGB888(0xff, 0x37, 0x1a), RGB888(0xd5, 0x4b, 0x00),
    RGB888(0xc4, 0x62, 0x00), RGB888(0x3c, 0x7b, 0x00), RGB888(0x1e, 0x84, 0x15), RGB888(0x00, 0x95, 0x66),
    RGB888(0x00, 0x84, 0xc4), RGB888(0x11, 0x11, 0x11), RGB888(0x09, 0x09, 0x09), RGB888(0x09, 0x09, 0x09),
    RGB888(0xff, 0xff, 0xff), RGB888(0x00, 0x95, 0xff), RGB888(0x6f, 0x84, 0xff), RGB888(0xd5, 0x6f, 0xff),
    RGB888(0xff, 0x77, 0xcc), RGB888(0xff, 0x6f, 0x99), RGB888(0xff, 0x7b, 0x59), RGB888(0xff, 0x91, 0x5f),
    RGB888(0xff, 0xa2, 0x33), RGB888(0xa6, 0xbf, 0x00), RGB888(0x51, 0xd9, 0x6a), RGB888(0x4d, 0xd5, 0xae),
    RGB888(0x00, 0xd9, 0xff), RGB888(0x66, 0x66, 0x66), RGB888(0x0d, 0x0d, 0x0d), RGB888(0x0d, 0x0d, 0x0d),
    RGB888(0xff, 0xff, 0xff), RGB888(0x84, 0xbf, 0xff), RGB888(0xbb, 0xbb, 0xff), RGB888(0xd0, 0xbb, 0xff),
    RGB888(0xff, 0xbf, 0xea), RGB888(0xff, 0xbf, 0xcc), RGB888(0xff, 0xc4, 0xb7), RGB888(0xff, 0xcc, 0xae),
    RGB888(0xff, 0xd9, 0xa2), RGB888(0xcc, 0xe1, 0x99), RGB888(0xae, 0xee, 0xb7), RGB888(0xaa, 0xf7, 0xee),
    RGB888(0xb3, 0xee, 0xff), RGB888(0xdd, 0xdd, 0xdd), RGB888(0x11, 0x11, 0x11), RGB888(0x11, 0x11, 0x11),
};

void updatePalette(PALETTES palette) {
    for (uint8_t i = 0; i < 64; i++) {
        if (palette == RGB333) {
            graphics_set_palette(i, NesPalette888[i + (64 * settings.nes_palette)]);
        }
        else {
            uint32_t c = NesPalette888[i + (64 * settings.nes_palette)];
            uint8_t r = (c >> (16 + 6)) & 0x3;
            uint8_t g = (c >> (8 + 6)) & 0x3;
            uint8_t b = (c >> (0 + 6)) & 0x3;
            r *= 42 * 2;
            g *= 42 * 2;
            b *= 42 * 2;
            graphics_set_palette(i, RGB888(r, g, b));
        }
    }
}

struct input_bits_t {
    bool a: true;
    bool b: true;
    bool select: true;
    bool start: true;
    bool right: true;
    bool left: true;
    bool up: true;
    bool down: true;
    input_bits_t operator |(const input_bits_t& o) {
        input_bits_t res = {
            a      | o.a,
            b      | o.b,
            select | o.select,
            start  | o.start,
            right  | o.right,
            left   | o.left,
            up     | o.up,
            down   | o.down
        };
        return res;
    }
};

input_bits_t keyboard_bits = { false, false, false, false, false, false, false, false };
input_bits_t gamepad1_bits = { false, false, false, false, false, false, false, false };
static input_bits_t gamepad2_bits = { false, false, false, false, false, false, false, false };

// ============================================================
// QwSTPad I2C Gamepad + Tufty built-in buttons
// ============================================================
#ifdef TUFTY2350

// QwSTPad TCA9555 registers
#define QWSTPAD_INPUT_PORT0  0x00
#define QWSTPAD_CONFIG_PORT0 0x06
#define QWSTPAD_POLARITY_PORT0 0x04
#define QWSTPAD_OUTPUT_PORT0 0x02

// QwSTPad button bit positions in 16-bit read
#define QWST_BTN_A     14
#define QWST_BTN_B     12
#define QWST_BTN_X     15
#define QWST_BTN_Y     13
#define QWST_BTN_UP     1
#define QWST_BTN_DOWN   4
#define QWST_BTN_LEFT   2
#define QWST_BTN_RIGHT  3
#define QWST_BTN_PLUS  11
#define QWST_BTN_MINUS  5

static bool qwstpad_connected = false;
static uint8_t qwstpad_addr = QWSTPAD_ADDR;

void qwstpad_init() {
    i2c_init(QWSTPAD_I2C, 100 * 1000);  // Start at 100kHz for reliability
    gpio_set_function(QWSTPAD_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(QWSTPAD_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(QWSTPAD_SDA_PIN);
    gpio_pull_up(QWSTPAD_SCL_PIN);

    // Scan I2C bus for TCA9555 at any valid address (0x20-0x27)
    uint8_t found_addr = 0;
    for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
        uint8_t rxdata;
        int ret = i2c_read_blocking_until(QWSTPAD_I2C, addr, &rxdata, 1, false,
                                           make_timeout_time_ms(10));
        if (ret >= 0) {
            found_addr = addr;
            break;
        }
    }

    if (found_addr) {
        qwstpad_addr = found_addr;
    }

    // Try to init TCA9555 - configure button pins as inputs
    uint8_t buf[3];
    buf[0] = QWSTPAD_CONFIG_PORT0;
    buf[1] = 0x3F;  // Port0 config: lower 6 bits = input
    buf[2] = 0xF9;  // Port1 config: bits 0,3-7 = input
    int ret = i2c_write_blocking_until(QWSTPAD_I2C, qwstpad_addr, buf, 3, false,
                                        make_timeout_time_ms(50));
    if (ret < 0) {
        qwstpad_connected = false;
        // Diagnostic: rapid blink = not found
        for (int i = 0; i < 20; i++) {
            gpio_put(PICO_DEFAULT_LED_PIN, true);
            sleep_ms(50);
            gpio_put(PICO_DEFAULT_LED_PIN, false);
            sleep_ms(50);
        }
        return;
    }
    qwstpad_connected = true;

    // Diagnostic: 3 slow blinks = connected
    for (int i = 0; i < 3; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(200);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(200);
    }

    // Now switch to 400kHz for normal operation
    i2c_set_baudrate(QWSTPAD_I2C, 400 * 1000);

    // Set polarity inversion (buttons are active low)
    buf[0] = QWSTPAD_POLARITY_PORT0;
    buf[1] = 0x3F;
    buf[2] = 0xF8;
    i2c_write_blocking_until(QWSTPAD_I2C, qwstpad_addr, buf, 3, false,
                              make_timeout_time_ms(50));

    // Set LED outputs to off
    buf[0] = QWSTPAD_OUTPUT_PORT0;
    buf[1] = 0xC0;
    buf[2] = 0x06;
    i2c_write_blocking_until(QWSTPAD_I2C, qwstpad_addr, buf, 3, false,
                              make_timeout_time_ms(50));
}

uint16_t qwstpad_read() {
    if (!qwstpad_connected) return 0;

    uint8_t reg = QWSTPAD_INPUT_PORT0;
    uint8_t data[2] = {0, 0};
    int ret = i2c_write_blocking_until(QWSTPAD_I2C, qwstpad_addr, &reg, 1, true,
                                        make_timeout_time_ms(5));
    if (ret < 0) return 0;
    ret = i2c_read_blocking_until(QWSTPAD_I2C, qwstpad_addr, data, 2, false,
                                   make_timeout_time_ms(5));
    if (ret < 0) return 0;

    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

void tufty_buttons_init() {
    // Init built-in button GPIOs with pull-ups
    const uint8_t buttons[] = { BTN_A, BTN_B, BTN_C, BTN_UP, BTN_DOWN, BTN_HOME };
    for (int i = 0; i < 6; i++) {
        gpio_init(buttons[i]);
        gpio_set_dir(buttons[i], GPIO_IN);
        gpio_pull_up(buttons[i]);
    }
}

void tufty_input_tick() {
    // Read QwSTPad via I2C
    uint16_t pad = qwstpad_read();
    bool qwst_a     = (pad >> QWST_BTN_A) & 1;
    bool qwst_b     = (pad >> QWST_BTN_B) & 1;
    bool qwst_x     = (pad >> QWST_BTN_X) & 1;
    bool qwst_y     = (pad >> QWST_BTN_Y) & 1;
    bool qwst_up    = (pad >> QWST_BTN_UP) & 1;
    bool qwst_down  = (pad >> QWST_BTN_DOWN) & 1;
    bool qwst_left  = (pad >> QWST_BTN_LEFT) & 1;
    bool qwst_right = (pad >> QWST_BTN_RIGHT) & 1;
    bool qwst_plus  = (pad >> QWST_BTN_PLUS) & 1;
    bool qwst_minus = (pad >> QWST_BTN_MINUS) & 1;

    // Read Tufty built-in buttons (active low)
    bool btn_a    = !gpio_get(BTN_A);
    bool btn_b    = !gpio_get(BTN_B);
    bool btn_c    = !gpio_get(BTN_C);
    bool btn_up   = !gpio_get(BTN_UP);
    bool btn_down = !gpio_get(BTN_DOWN);
    bool btn_home = !gpio_get(BTN_HOME);

    // Map to NES:
    // QwSTPad: A=NES_A, B=NES_B, X=Select, Y=Start, +/-=Start/Select
    // Tufty: A=NES_A, B=NES_B, C=NES_Start, UP/DOWN=directions, HOME=Select
    gamepad1_bits.a      = qwst_a    | btn_a;
    gamepad1_bits.b      = qwst_b    | btn_b;
    gamepad1_bits.select = qwst_minus | qwst_x | btn_home;
    gamepad1_bits.start  = qwst_plus  | qwst_y | btn_c;
    gamepad1_bits.up     = qwst_up   | btn_up;
    gamepad1_bits.down   = qwst_down | btn_down;
    gamepad1_bits.left   = qwst_left;
    gamepad1_bits.right  = qwst_right;
}

#endif // TUFTY2350

// ============================================================
// Original NES pad + keyboard input (non-Tufty)
// ============================================================
#ifndef TUFTY2350
#if USE_NESPAD
void nespad_tick() {
    nespad_read();
    gamepad1_bits.a = settings.swap_ab ? (nespad_state & DPAD_B) != 0 : (nespad_state & DPAD_A) != 0;
    gamepad1_bits.b = settings.swap_ab ? (nespad_state & DPAD_A) != 0 : (nespad_state & DPAD_B) != 0;
    gamepad1_bits.select = (nespad_state & DPAD_SELECT) != 0;
    gamepad1_bits.start = (nespad_state & DPAD_START) != 0;
    gamepad1_bits.up = (nespad_state & DPAD_UP) != 0;
    gamepad1_bits.down = (nespad_state & DPAD_DOWN) != 0;
    gamepad1_bits.left = (nespad_state & DPAD_LEFT) != 0;
    gamepad1_bits.right = (nespad_state & DPAD_RIGHT) != 0;
    gamepad2_bits.a = settings.swap_ab ? (nespad_state2 & DPAD_B) != 0 : (nespad_state2 & DPAD_A) != 0;
    gamepad2_bits.b = settings.swap_ab ? (nespad_state2 & DPAD_A) != 0 : (nespad_state2 & DPAD_B) != 0;
    gamepad2_bits.select = (nespad_state2 & DPAD_SELECT) != 0;
    gamepad2_bits.start = (nespad_state2 & DPAD_START) != 0;
    gamepad2_bits.up = (nespad_state2 & DPAD_UP) != 0;
    gamepad2_bits.down = (nespad_state2 & DPAD_DOWN) != 0;
    gamepad2_bits.left = (nespad_state2 & DPAD_LEFT) != 0;
    gamepad2_bits.right = (nespad_state2 & DPAD_RIGHT) != 0;
}
#endif

#if USE_PS2_KBD
static bool isInReport(hid_keyboard_report_t const* report, const unsigned char keycode) {
    for (unsigned char i: report->keycode) {
        if (i == keycode) return true;
    }
    return false;
}

extern "C" {
    volatile bool altPressed = false;
    volatile bool ctrlPressed = false;
    volatile uint8_t fxPressedV = 0;
}

void __not_in_flash_func(process_kbd_report)(hid_keyboard_report_t const* report,
                                             hid_keyboard_report_t const* prev_report) {
    keyboard_bits.start = isInReport(report, HID_KEY_ENTER) || isInReport(report, HID_KEY_KEYPAD_ENTER);
    keyboard_bits.select = isInReport(report, HID_KEY_BACKSPACE) || isInReport(report, HID_KEY_ESCAPE);
    keyboard_bits.a = isInReport(report, HID_KEY_Z) || isInReport(report, HID_KEY_O);
    keyboard_bits.b = isInReport(report, HID_KEY_X) || isInReport(report, HID_KEY_P);
    keyboard_bits.up = isInReport(report, HID_KEY_ARROW_UP) || isInReport(report, HID_KEY_W);
    keyboard_bits.down = isInReport(report, HID_KEY_ARROW_DOWN) || isInReport(report, HID_KEY_S);
    keyboard_bits.left = isInReport(report, HID_KEY_ARROW_LEFT) || isInReport(report, HID_KEY_A);
    keyboard_bits.right = isInReport(report, HID_KEY_ARROW_RIGHT) || isInReport(report, HID_KEY_D);
}

Ps2Kbd_Mrmltr ps2kbd(pio1, PS2KBD_GPIO_FIRST, process_kbd_report);
#endif
#endif // !TUFTY2350

// Provide these symbols even if not used
#ifdef TUFTY2350
extern "C" {
    volatile bool altPressed = false;
    volatile bool ctrlPressed = false;
    volatile uint8_t fxPressedV = 0;
}
#endif

inline bool checkNESMagic(const uint8_t* data) {
    bool ok = false;
    int nIdx;
    if (memcmp(data, "NES\x1a", 4) == 0) {
        uint8_t MapperNo = *(data + 6) >> 4;
        for (nIdx = 0; MapperTable[nIdx].nMapperNo != -1; ++nIdx) {
            if (MapperTable[nIdx].nMapperNo == MapperNo) {
                ok = true;
                break;
            }
        }
    }
    return ok;
}

static int rapidFireMask = 0;
static int rapidFireCounter = 0;
int save_slot = 0;

void InfoNES_PadState(DWORD* pdwPad1, DWORD* pdwPad2, DWORD* pdwSystem) {
    static constexpr int LEFT = 1 << 6;
    static constexpr int RIGHT = 1 << 7;
    static constexpr int UP = 1 << 4;
    static constexpr int DOWN = 1 << 5;
    static constexpr int SELECT = 1 << 2;
    static constexpr int START = 1 << 3;
    static constexpr int A = 1 << 0;
    static constexpr int B = 1 << 1;

    input_bits_t player1_state = keyboard_bits | gamepad1_bits;

    int gamepad_state = (player1_state.left ? LEFT : 0) |
                        (player1_state.right ? RIGHT : 0) |
                        (player1_state.up ? UP : 0) |
                        (player1_state.down ? DOWN : 0) |
                        (player1_state.start ? START : 0) |
                        (player1_state.select ? SELECT : 0) |
                        (player1_state.a ? A : 0) |
                        (player1_state.b ? B : 0) |
                        0;
    ++rapidFireCounter;
    auto&dst = *pdwPad1;
    int rv = gamepad_state;
    if (rapidFireCounter & 2) {
        rv &= ~rapidFireMask;
    }
    dst = rv;
    *pdwPad2 = 0;
    *pdwSystem = 0;
}

void InfoNES_MessageBox(const char* pszMsg, ...) {
    printf("[MSG]");
    va_list args;
    va_start(args, pszMsg);
    vprintf(pszMsg, args);
    va_end(args);
    printf("\n");
}

void InfoNES_Error(const char* pszMsg, ...) {
    printf("[Error]");
    va_list args;
    va_start(args, pszMsg);
    va_end(args);
    printf("\n");
}

bool parseROM(const uint8_t* nesFile) {
    memcpy(&NesHeader, nesFile, sizeof(NesHeader));
    if (!checkNESMagic(NesHeader.byID)) {
        return false;
    }
    nesFile += sizeof(NesHeader);
    memset(SRAM, 0, SRAM_SIZE);
    if (NesHeader.byInfo1 & 4) {
        memcpy(&SRAM[0x1000], nesFile, 512);
        nesFile += 512;
    }
    auto romSize = NesHeader.byRomSize * 0x4000;
    ROM = (BYTE *)nesFile;
    nesFile += romSize;
    if (NesHeader.byVRomSize > 0) {
        auto vromSize = NesHeader.byVRomSize * 0x2000;
        VROM = (BYTE *)nesFile;
        nesFile += vromSize;
    }
    return true;
}

void InfoNES_ReleaseRom() {
    ROM = nullptr;
    VROM = nullptr;
}

void InfoNES_SoundInit() {
#ifndef TUFTY2350
    i2s_config = i2s_get_default_config();
    i2s_config.sample_freq = 44100;
    i2s_config.dma_trans_count = (uint16_t)i2s_config.sample_freq / 60;
    i2s_volume(&i2s_config, 0);
    i2s_init(&i2s_config);
#endif
}

int InfoNES_SoundOpen(int samples_per_sync, int sample_rate) {
    return 0;
}

void InfoNES_SoundClose() {
}

#define buffermax ((44100 / 60)*2)
int __not_in_flash_func(InfoNES_GetSoundBufferSize)() { return buffermax; }


void InfoNES_SoundOutput(int samples, const BYTE* wave1, const BYTE* wave2, const BYTE* wave3, const BYTE* wave4,
                         const BYTE* wave5) {
#ifndef TUFTY2350
    static int16_t samples_out[2][buffermax * 2];
    static int i_active_buf = 0;
    static int inx = 0;
    for (int i = 0; i < samples; i++) {
        int w1 = *wave1++;
        int w2 = *wave2++;
        int w3 = *wave3++;
        int w4 = *wave4++;
        int w5 = *wave5++;
        int l = w1 * 6 + w2 * 3 + w3 * 5 + w4 * 3 * 17 + w5 * 2 * 32;
        int r = w1 * 3 + w2 * 6 + w3 * 5 + w4 * 3 * 17 + w5 * 2 * 32;
        l -= 4000;
        r -= 4000;
        samples_out[i_active_buf][inx * 2] = (int16_t)l * settings.snd_vol;
        samples_out[i_active_buf][inx * 2 + 1] = (int16_t)r * settings.snd_vol;
        if (inx++ >= i2s_config.dma_trans_count) {
            inx = 0;
            i2s_dma_write(&i2s_config, reinterpret_cast<const int16_t *>(samples_out[i_active_buf]));
            i_active_buf ^= 1;
        }
    }
#endif
    // On Tufty: silently discard audio
}

void __not_in_flash_func(InfoNES_PreDrawLine)(int line) {
    InfoNES_SetLineBuffer(linebuffer, NES_DISP_WIDTH);
}

void __not_in_flash_func(InfoNES_PostDrawLine)(int line) {
    for (int x = 0; x < NES_DISP_WIDTH; x++) SCREEN[line][x] = (uint8_t)linebuffer[x];
}

/* Renderer loop on Pico's second core */
void __scratch_x("render") render_core() {
    multicore_lockout_victim_init();
    graphics_init();

    auto* buffer = &SCREEN[0][0];
    graphics_set_buffer(buffer, NES_DISP_WIDTH, NES_DISP_HEIGHT);
    graphics_set_textbuffer(buffer);
    graphics_set_bgcolor(0x000000);
    graphics_set_offset(32, 0);

    updatePalette(settings.palette);
    graphics_set_flashmode(settings.flash_line, settings.flash_frame);
    sem_acquire_blocking(&vga_start_semaphore);

    // 60 FPS loop
#define frame_tick (16666)
    uint64_t tick = time_us_64();
    uint64_t last_renderer_tick = tick;
    uint64_t last_input_tick = tick;
    while (true) {
        if (tick >= last_renderer_tick + frame_tick) {
            refresh_lcd();
            last_renderer_tick = tick;
        }
        if (tick >= last_input_tick + frame_tick) {
#ifdef TUFTY2350
            tufty_input_tick();
#else
#if USE_PS2_KBD
            ps2kbd.tick();
#endif
#if USE_NESPAD
            nespad_tick();
#endif
#endif
            last_input_tick = tick;
        }
        tick = time_us_64();

        tuh_task();
        tight_loop_contents();
    }

    __unreachable();
}

#ifndef TUFTY2350
// File browser and SD card functions (not used on Tufty)
bool filebrowser_loadfile(const char pathname[256]) { return false; }
#endif

#ifdef TUFTY2350
int tufty_rom_select() {
    graphics_set_mode(TEXTMODE_DEFAULT);
    sleep_ms(50);

    // Clear the text buffer
    memset(&SCREEN[0][0], 0, sizeof(SCREEN));

    int sel = 0;
    bool redraw = true;

    while (true) {
        if (redraw) {
            // Clear screen
            memset(&SCREEN[0][0], 0, sizeof(SCREEN));

            // Title
            draw_text("NES ROM Selector", 18, 2, 15, 0);

            // Separator line
            char sep[TEXTMODE_COLS + 1];
            memset(sep, 0, sizeof(sep));
            memset(sep, '-', 34);
            draw_text(sep, 10, 4, 7, 0);

            // ROM list
            for (int i = 0; i < ROM_COUNT; i++) {
                char line[TEXTMODE_COLS + 1];
                memset(line, 0, sizeof(line));

                if (i == sel) {
                    snprintf(line, sizeof(line), "> %s", rom_table[i].name);
                    draw_text(line, 8, 6 + i, 14, 1);  // yellow on blue
                } else {
                    snprintf(line, sizeof(line), "  %s", rom_table[i].name);
                    draw_text(line, 8, 6 + i, 15, 0);  // white on black
                }
            }

            // Instructions
            draw_text("A=Select  UP/DOWN=Navigate", 13, 28, 7, 0);

            redraw = false;
        }

        sleep_ms(16);

        // Read inputs from gamepad1_bits (updated by core1 via tufty_input_tick)
        // Don't call qwstpad_read() here - core1 already reads I2C, dual access corrupts the bus
        bool btn_up   = gamepad1_bits.up;
        bool btn_down = gamepad1_bits.down;
        bool btn_a    = gamepad1_bits.a;

        if (btn_up) {
            if (sel > 0) sel--;
            else sel = ROM_COUNT - 1;
            redraw = true;
            sleep_ms(200);
        }
        if (btn_down) {
            if (sel < ROM_COUNT - 1) sel++;
            else sel = 0;
            redraw = true;
            sleep_ms(200);
        }
        if (btn_a) {
            sleep_ms(200);
            break;
        }
    }

    // Set the selected ROM
    current_rom_index = sel;
    rom = rom_table[sel].data;

    // Switch back to graphics mode
    graphics_set_mode(GRAPHICSMODE_DEFAULT);
    return sel;
}
#endif

int InfoNES_Video() {
    if (!parseROM(reinterpret_cast<const uint8_t *>(rom))) {
        return 0;
    }
    if (InfoNES_Reset() < 0) {
        return 1;
    }
    graphics_set_mode(GRAPHICSMODE_DEFAULT);
    return 0;
}

int InfoNES_Menu() {
#ifdef TUFTY2350
    // Show ROM selector menu, then proceed to InfoNES_Video() which calls parseROM()
    tufty_rom_select();
    return 0;
#else
    graphics_set_mode(TEXTMODE_DEFAULT);
    filebrowser(HOME_DIR, (char *)"nes");
    graphics_set_mode(GRAPHICSMODE_DEFAULT);
    return 1;
#endif
}

#ifndef TUFTY2350
void load_config() {}
void save_config() {}

typedef struct __attribute__((__packed__)) {
    bool is_directory;
    bool is_executable;
    size_t size;
    char filename[79];
} file_item_t;

void filebrowser(const char pathname[256], const char* executables) {}
bool is_start_locked() { return false; }
void unlock_start() {}
void lock_start() {}
#endif

int InfoNES_LoadFrame() {
    frames++;
    return 0;
}

int main() {
#if !PICO_RP2040
    volatile uint32_t *qmi_m0_timing=(uint32_t *)0x400d000c;
    vreg_disable_voltage_limit();
    vreg_set_voltage(VREG_VOLTAGE_1_60);
    sleep_ms(33);
    *qmi_m0_timing = 0x60007204;
    set_sys_clock_khz(CPU_FREQ * KHZ, 0);
    *qmi_m0_timing = 0x60007303;
#else
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(10);
    set_sys_clock_khz(CPU_FREQ * KHZ, true);
#endif

    // LED blink to show we're alive
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    for (int i = 0; i < 6; i++) {
        sleep_ms(33);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(33);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }

#ifdef TUFTY2350
    // Enable power rail - GPIO 41 controls display/I2C power
    gpio_init(41);
    gpio_set_dir(41, GPIO_OUT);
    gpio_put(41, 1);

    // Disable CYW43 wireless chip - GPIO 23 (WL_ON) floating could cause interference
    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, 0);

    // Wait for I2C peripherals to power up after enabling the power rail
    sleep_ms(100);
#endif

#ifdef TUFTY2350
    // Init Tufty buttons and I2C gamepad
    tufty_buttons_init();
    qwstpad_init();
#endif

    tuh_init(BOARD_TUH_RHPORT);

    memset(&SCREEN[0][0], 0, sizeof SCREEN);

#ifndef TUFTY2350
#if USE_PS2_KBD
    ps2kbd.init_gpio();
#endif
#if USE_NESPAD
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);
#endif
#endif

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);

#ifdef TUFTY2350
    // Wait for core1 graphics_init() to finish (LCD init has ~40ms of delays)
    sleep_ms(200);
    // Show ROM selector menu, then parse the selected ROM
    tufty_rom_select();
    if (!parseROM(reinterpret_cast<const uint8_t *>(rom))) {
        // ROM parse failed - blink LED rapidly as error indicator
        while (1) {
            gpio_put(PICO_DEFAULT_LED_PIN, true);
            sleep_ms(100);
            gpio_put(PICO_DEFAULT_LED_PIN, false);
            sleep_ms(100);
        }
    }
#else
    f_mount(&fs, "", 1);
    f_mkdir(HOME_DIR);
    load_config();
    if (is_start_locked() || !parseROM(reinterpret_cast<const uint8_t *>(rom))) {
        InfoNES_Menu();
    }
    lock_start();
#endif

    bool start_from_game = InfoNES_Main(true);
    while (1) {
        sleep_ms(500);
        start_from_game = InfoNES_Main(start_from_game);
    }
}
