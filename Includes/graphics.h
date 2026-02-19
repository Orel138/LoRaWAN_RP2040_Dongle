#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

/* Colors (RGB565 format) */
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_ORANGE        0xFD20
#define COLOR_GRAY          0x8410
#define COLOR_DARKGRAY      0x4208
#define COLOR_LIGHTGRAY     0xC618

/* Font size */
typedef enum {
    FONT_SIZE_SMALL = 1,
    FONT_SIZE_MEDIUM = 2,
    FONT_SIZE_LARGE = 3
} FontSize_t;

/* Initialize graphics library */
void Graphics_Init(uint16_t width, uint16_t height);

/* Basic drawing functions */
void Graphics_DrawPixel(int16_t x, int16_t y, uint16_t color);
void Graphics_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void Graphics_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void Graphics_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void Graphics_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void Graphics_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void Graphics_DrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void Graphics_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void Graphics_DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void Graphics_FillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);

/* Text functions */
void Graphics_DrawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, FontSize_t size);
void Graphics_DrawString(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, FontSize_t size);
void Graphics_SetTextColor(uint16_t color, uint16_t bg);
void Graphics_SetTextSize(FontSize_t size);
void Graphics_SetCursor(int16_t x, int16_t y);
void Graphics_Print(const char *str);

/* Screen functions */
void Graphics_FillScreen(uint16_t color);
void Graphics_Clear(void);

/* Helper functions */
uint16_t Graphics_Color565(uint8_t r, uint8_t g, uint8_t b);

#endif /* GRAPHICS_H */