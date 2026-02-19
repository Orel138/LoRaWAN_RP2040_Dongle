#include "graphics.h"
#include "pico/st7789.h"
#include <string.h>
#include <stdlib.h>

/* Display dimensions */
static uint16_t _width = 240;
static uint16_t _height = 320;

/* Text settings */
static uint16_t _textcolor = COLOR_WHITE;
static uint16_t _textbgcolor = COLOR_BLACK;
static FontSize_t _textsize = FONT_SIZE_SMALL;
static int16_t _cursor_x = 0;
static int16_t _cursor_y = 0;

/* 5x7 font bitmap (simplified) */
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    // Add more characters as needed (A-Z, a-z, etc.)
};

/* Initialize graphics */
void Graphics_Init(uint16_t width, uint16_t height)
{
    _width = width;
    _height = height;
}

/* Draw single pixel */
void Graphics_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
    if(x < 0 || x >= _width || y < 0 || y >= _height)
        return;
    
    st7789_set_cursor(x, y);
    st7789_put(color);
}

/* Draw line using Bresenham's algorithm */
void Graphics_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while(1)
    {
        Graphics_DrawPixel(x0, y0, color);
        
        if(x0 == x1 && y0 == y1)
            break;
        
        int16_t e2 = 2 * err;
        
        if(e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        
        if(e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

/* Draw rectangle outline */
void Graphics_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    Graphics_DrawLine(x, y, x + w - 1, y, color);
    Graphics_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
    Graphics_DrawLine(x + w - 1, y + h - 1, x, y + h - 1, color);
    Graphics_DrawLine(x, y + h - 1, x, y, color);
}

/* Fill rectangle */
void Graphics_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    for(int16_t i = x; i < x + w; i++)
    {
        for(int16_t j = y; j < y + h; j++)
        {
            Graphics_DrawPixel(i, j, color);
        }
    }
}

/* Draw circle using Bresenham's algorithm */
void Graphics_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    
    while(x >= y)
    {
        Graphics_DrawPixel(x0 + x, y0 + y, color);
        Graphics_DrawPixel(x0 + y, y0 + x, color);
        Graphics_DrawPixel(x0 - y, y0 + x, color);
        Graphics_DrawPixel(x0 - x, y0 + y, color);
        Graphics_DrawPixel(x0 - x, y0 - y, color);
        Graphics_DrawPixel(x0 - y, y0 - x, color);
        Graphics_DrawPixel(x0 + y, y0 - x, color);
        Graphics_DrawPixel(x0 + x, y0 - y, color);
        
        if(err <= 0)
        {
            y += 1;
            err += 2 * y + 1;
        }
        
        if(err > 0)
        {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

/* Fill circle */
void Graphics_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    for(int16_t y = -r; y <= r; y++)
    {
        for(int16_t x = -r; x <= r; x++)
        {
            if(x * x + y * y <= r * r)
            {
                Graphics_DrawPixel(x0 + x, y0 + y, color);
            }
        }
    }
}

/* Draw character */
void Graphics_DrawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, FontSize_t size)
{
    if(c < 32 || c > 126)
        c = 32; // Space for unsupported chars
    
    const uint8_t *glyph = font5x7[c - 32];
    
    for(uint8_t i = 0; i < 5; i++)
    {
        uint8_t line = glyph[i];
        
        for(uint8_t j = 0; j < 8; j++)
        {
            if(line & 0x01)
            {
                if(size == 1)
                {
                    Graphics_DrawPixel(x + i, y + j, color);
                }
                else
                {
                    Graphics_FillRect(x + i * size, y + j * size, size, size, color);
                }
            }
            else if(bg != color)
            {
                if(size == 1)
                {
                    Graphics_DrawPixel(x + i, y + j, bg);
                }
                else
                {
                    Graphics_FillRect(x + i * size, y + j * size, size, size, bg);
                }
            }
            
            line >>= 1;
        }
    }
}

/* Draw string */
void Graphics_DrawString(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, FontSize_t size)
{
    int16_t cursor_x = x;
    
    while(*str)
    {
        if(*str == '\n')
        {
            cursor_x = x;
            y += 8 * size;
        }
        else if(*str == '\r')
        {
            // Ignore
        }
        else
        {
            Graphics_DrawChar(cursor_x, y, *str, color, bg, size);
            cursor_x += 6 * size;
        }
        
        str++;
    }
}

/* Fill screen with color */
void Graphics_FillScreen(uint16_t color)
{
    st7789_fill(color);
}

/* Clear screen (black) */
void Graphics_Clear(void)
{
    Graphics_FillScreen(COLOR_BLACK);
}

/* Create RGB565 color */
uint16_t Graphics_Color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/* Text helpers */
void Graphics_SetTextColor(uint16_t color, uint16_t bg)
{
    _textcolor = color;
    _textbgcolor = bg;
}

void Graphics_SetTextSize(FontSize_t size)
{
    _textsize = size;
}

void Graphics_SetCursor(int16_t x, int16_t y)
{
    _cursor_x = x;
    _cursor_y = y;
}

void Graphics_Print(const char *str)
{
    Graphics_DrawString(_cursor_x, _cursor_y, str, _textcolor, _textbgcolor, _textsize);
}