#include "ssd1306.h"
#include "stm32f3xx_hal.h"

extern I2C_HandleTypeDef hi2c1;  // permette a ssd1306 di usare l'I2C

/* Display buffer */
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
SSD1306_t SSD1306;

/* Send command to OLED */
static void ssd1306_WriteCommand(uint8_t command) {
    uint8_t data[2];
    data[0] = 0x00; // Control byte for command
    data[1] = command;
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, data, 2, HAL_MAX_DELAY);
}

/* Send data to OLED */
static void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    uint8_t data[SSD1306_WIDTH + 1];
    data[0] = 0x40;
    for (size_t i = 0; i < buff_size; i++) {
        data[1 + i] = buffer[i];
    }
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, data, buff_size + 1, HAL_MAX_DELAY);
}

/* Initialize display */
void ssd1306_Init(void) {
    HAL_Delay(100);

    ssd1306_WriteCommand(0xAE); // display off
    ssd1306_WriteCommand(0x20); // Set Memory Addressing Mode
    ssd1306_WriteCommand(0x00); // Horizontal Addressing Mode
    ssd1306_WriteCommand(0xB0); // Set Page Start Address
    ssd1306_WriteCommand(0xC8); // COM Output Scan Direction
    ssd1306_WriteCommand(0x00); // Low column address
    ssd1306_WriteCommand(0x10); // High column address
    ssd1306_WriteCommand(0x40); // start line address
    ssd1306_WriteCommand(0x81); // set contrast control register
    ssd1306_WriteCommand(0xFF);
    ssd1306_WriteCommand(0xA1); // segment re-map 0 to 127
    ssd1306_WriteCommand(0xA6); // normal display
    ssd1306_WriteCommand(0xA8); // set multiplex ratio
    ssd1306_WriteCommand(0x3F);
    ssd1306_WriteCommand(0xA4); // display follows RAM content
    ssd1306_WriteCommand(0xD3); // display offset
    ssd1306_WriteCommand(0x00);
    ssd1306_WriteCommand(0xD5); // set display clock divide ratio
    ssd1306_WriteCommand(0xF0);
    ssd1306_WriteCommand(0xD9); // pre-charge period
    ssd1306_WriteCommand(0x22);
    ssd1306_WriteCommand(0xDA); // com pins hardware configuration
    ssd1306_WriteCommand(0x12);
    ssd1306_WriteCommand(0xDB); // vcomh
    ssd1306_WriteCommand(0x20);
    ssd1306_WriteCommand(0x8D); // charge pump
    ssd1306_WriteCommand(0x14);
    ssd1306_WriteCommand(0xAF); // display ON

    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    SSD1306.Initialized = 1;
}

/* Fill buffer with color */
void ssd1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color == Black) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

/* Update entire screen */
void ssd1306_UpdateScreen(void) {
    for (uint8_t i = 0; i < 8; i++) {
        ssd1306_WriteCommand(0xB0 + i);
        ssd1306_WriteCommand(0x00);
        ssd1306_WriteCommand(0x10);
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
    }
}

/* Draw pixel */
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    if (color == White) SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    else SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
}

/* Set cursor position */
void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

/* Draw a basic 6x8 font */
static const uint8_t font6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, // space
    {0x00,0x00,0x5F,0x00,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, // #
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, // $
    // ... (puoi ampliare se vuoi)
};

/* Write a single char (very minimal) */
void ssd1306_WriteChar(char ch, SSD1306_COLOR color) {
    if (ch < 32 || ch > 127) ch = '?';
    uint8_t index = ch - 32;
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t line = (index < sizeof(font6x8)/6) ? font6x8[index][i] : 0x00;
        for (uint8_t j = 0; j < 8; j++) {
            if (line & (1 << j)) ssd1306_DrawPixel(SSD1306.CurrentX + i, SSD1306.CurrentY + j, color);
            else ssd1306_DrawPixel(SSD1306.CurrentX + i, SSD1306.CurrentY + j, (SSD1306_COLOR)!color);
        }
    }
    SSD1306.CurrentX += 6;
}

/* Write string */
void ssd1306_WriteString(const char* str, SSD1306_COLOR color) {
    while (*str) {
        ssd1306_WriteChar(*str++, color);
    }
}

/* Write number (0-9999) */
void ssd1306_WriteNumber(uint16_t num, SSD1306_COLOR color) {
    char buf[8];
    sprintf(buf, "%u", num);
    ssd1306_WriteString(buf, color);
}
