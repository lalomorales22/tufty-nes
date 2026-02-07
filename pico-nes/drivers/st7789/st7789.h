#pragma once

#ifdef TFT_PARALLEL

// Parallel mode pin definitions (set in board header)
#ifndef TFT_CS_PIN
#define TFT_CS_PIN 27
#endif
#ifndef TFT_DC_PIN
#define TFT_DC_PIN 28
#endif
#ifndef TFT_WR_PIN
#define TFT_WR_PIN 30
#endif
#ifndef TFT_RD_PIN
#define TFT_RD_PIN 31
#endif
#ifndef TFT_LED_PIN
#define TFT_LED_PIN 26
#endif
#ifndef TFT_DB_BASE
#define TFT_DB_BASE 32
#endif

#else

// SPI mode pin definitions
#ifndef TFT_RST_PIN
#define TFT_RST_PIN 8
#endif
#ifndef TFT_CS_PIN
#define TFT_CS_PIN 6
#endif
#ifndef TFT_LED_PIN
#define TFT_LED_PIN 9
#endif
#ifndef TFT_CLK_PIN
#define TFT_CLK_PIN 13
#endif
#ifndef TFT_DATA_PIN
#define TFT_DATA_PIN 12
#endif
#ifndef TFT_DC_PIN
#define TFT_DC_PIN 10
#endif

#endif // TFT_PARALLEL

#define TEXTMODE_COLS 53
#define TEXTMODE_ROWS 30

#define RGB888(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
static const uint16_t textmode_palette[16] = {
    RGB888(0x00,0x00, 0x00),
    RGB888(0x00,0x00, 0xC4),
    RGB888(0x00,0xC4, 0x00),
    RGB888(0x00,0xC4, 0xC4),
    RGB888(0xC4,0x00, 0x00),
    RGB888(0xC4,0x00, 0xC4),
    RGB888(0xC4,0x7E, 0x00),
    RGB888(0xC4,0xC4, 0xC4),
    RGB888(0x4E,0x4E, 0x4E),
    RGB888(0x4E,0x4E, 0xDC),
    RGB888(0x4E,0xDC, 0x4E),
    RGB888(0x4E,0xF3, 0xF3),
    RGB888(0xDC,0x4E, 0x4E),
    RGB888(0xF3,0x4E, 0xF3),
    RGB888(0xF3,0xF3, 0x4E),
    RGB888(0xFF,0xFF, 0xFF),
};

inline static void graphics_set_bgcolor(uint32_t color888) {
    // dummy
}
inline static void graphics_set_flashmode(bool flash_line, bool flash_frame) {
    // dummy
}
void refresh_lcd();
