/*
 * Board definition for Pimoroni Tufty 2350
 * RP2350B + 16MB flash + 8MB PSRAM
 * 2.8" 320x240 IPS LCD (ST7789, 8-bit parallel)
 */

#ifndef _BOARDS_TUFTY2350_H
#define _BOARDS_TUFTY2350_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)

#define PICO_RP2350A 0  // RP2350B (not A)

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- I2C (QWIIC/STEMMA QT) ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 4
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 5
#endif

// --- FLASH ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (16 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

#include "boards/pico2.h"

#define CPU_FREQ 252

// --- LCD (8-bit parallel ST7789) ---
#define TFT_CS_PIN     27
#define TFT_DC_PIN     28   // RS/Data-Command
#define TFT_WR_PIN     30   // Write strobe
#define TFT_RD_PIN     31   // Read strobe (active low, idle high)
#define TFT_LED_PIN    26   // Backlight
#define TFT_DB_BASE    32   // DB0=GPIO32 .. DB7=GPIO39

// Parallel mode flag
#define TFT_PARALLEL   1

// --- Buttons ---
#define BTN_A          7
#define BTN_B          9
#define BTN_C          10
#define BTN_UP         11
#define BTN_DOWN       6
#define BTN_HOME       22

// --- I2C for QwSTPad ---
#define QWSTPAD_I2C       i2c0
#define QWSTPAD_SDA_PIN   4
#define QWSTPAD_SCL_PIN   5
#define QWSTPAD_ADDR      0x21

// --- PSRAM ---
#define BW_PSRAM_CS    8

// --- Rear LEDs ---
#define BW_LED_0       0
#define BW_LED_1       1
#define BW_LED_2       2
#define BW_LED_3       3

// No SD card on Tufty
// No NES shift-register gamepad
// No PS2 keyboard
// No I2S/audio hardware

// Override pico2.h's PICO_DEFAULT_LED_PIN (25 = WL_CS on Tufty!)
#undef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN BW_LED_0
#define NES_GPIO_CLK   0xFF
#define NES_GPIO_LAT   0xFF
#define NES_GPIO_DATA  0xFF
#define NES_GPIO_DATA2 0xFF
#define PS2KBD_GPIO_FIRST 0xFF

// Not used but needed by build
#define SDCARD_PIN_SPI0_CS   0xFF
#define SDCARD_PIN_SPI0_SCK  0xFF
#define SDCARD_PIN_SPI0_MOSI 0xFF
#define SDCARD_PIN_SPI0_MISO 0xFF

#define SMS_SINGLE_FILE 1

// Sound - PWM on unused pin (muted, no speaker)
#define AUDIO_PWM_PIN  0xFF
#define AUDIO_DATA_PIN  0xFF
#define AUDIO_CLOCK_PIN 0xFF
#define AUDIO_LCK_PIN   0xFF

#endif // _BOARDS_TUFTY2350_H
