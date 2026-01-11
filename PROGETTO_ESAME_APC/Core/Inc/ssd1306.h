#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "stm32f3xx_hal.h"
#include <string.h>
#include <stdlib.h>

/* SSD1306 I2C address */
#define SSD1306_I2C_ADDR 0x78

/* Display size */
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

/* Color definitions */
typedef enum {
    Black = 0x00,
    White = 0x01
} SSD1306_COLOR;

/* Main struct */
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
} SSD1306_t;

extern I2C_HandleTypeDef hi2c1;
extern SSD1306_t SSD1306;

/* API */
void ssd1306_Init(void);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_UpdateScreen(void);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
void ssd1306_WriteChar(char ch, SSD1306_COLOR color);
void ssd1306_WriteString(const char* str, SSD1306_COLOR color);
void ssd1306_WriteNumber(uint16_t num, SSD1306_COLOR color);

#endif
