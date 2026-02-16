#include "main.h"

#include <stdio.h>

#include "hardware/uart.h"

#include <stdlib.h>          // for rand()
#include "pico/stdlib.h"
#include "pico/st7789.h"

#include "hardware/gpio.h"

#define TFT_SPI_PORT spi1

// LCD configuration
const struct st7789_config lcd_config = {
    .spi      = TFT_SPI_PORT, //PICO_DEFAULT_SPI,
    .gpio_din = 11, //PICO_DEFAULT_SPI_TX_PIN,
    .gpio_clk = 10, //PICO_DEFAULT_SPI_SCK_PIN,
    .gpio_cs  = 13, //PICO_DEFAULT_SPI_CSN_PIN,
    .gpio_dc  = 8,
    .gpio_rst = 12,
    .gpio_bl  = 9,
};

const int LCD_WIDTH = 240;
const int LCD_HEIGHT = 320;

#define GPIO_WATCH_PIN 14
#define GPIO_WATCH_PIN2 15

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5

static char event_str[128];

void gpio_event_string(char *buf, uint32_t events);

void gpio_callback(uint gpio, uint32_t events) {
    // Put the GPIO event(s) that just happened into event_str
    // so we can print it
    gpio_event_string(event_str, events);
    printf("GPIO %d %s\n", gpio, event_str);
}

int main()
{
    stdio_init_all();

    gpio_init(GPIO_WATCH_PIN2);
    gpio_set_dir(GPIO_WATCH_PIN2, GPIO_IN);
    gpio_pull_up(GPIO_WATCH_PIN2);
    gpio_set_irq_enabled_with_callback(GPIO_WATCH_PIN2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // initialize the lcd
    st7789_init(&lcd_config, LCD_WIDTH, LCD_HEIGHT);

    // make screen black
    st7789_fill(0x0000);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\n");
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    printf("GPIO %d initial state: %d\n", GPIO_WATCH_PIN2, gpio_get(GPIO_WATCH_PIN2));

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);

        // create a random x, y, and color value
        int rand_x = rand() % LCD_WIDTH;
        int rand_y = rand() % LCD_HEIGHT;
        uint16_t rand_color = rand() % 0xffff;
        
        // move the cursor to the random x and y position
        st7789_set_cursor(rand_x, rand_y);

        // put the random color as the pixel value
        st7789_put(rand_color);
    }
}

static const char *gpio_irq_str[] = {
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // If more events add ", "
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf++ = '\0';
}