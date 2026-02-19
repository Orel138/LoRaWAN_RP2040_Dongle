#include "main.h"

#include <stdio.h>

#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"

#include <stdlib.h>          // for rand()
#include "pico/stdlib.h"
#include "pico/st7789.h"

#include "hardware/gpio.h"

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// CLI
#include "cli.h"

#include "rak3172.h"

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
#define UART_ID uart0
#define BAUD_RATE 115200

// Use pins 0 and 1 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 0
#define UART_RX_PIN 1

static char event_str[128];
static QueueHandle_t gpio_event_queue;

static uint32_t ulHighFrequencyTimerTicks = 0;

void gpio_event_string(char *buf, uint32_t events);

void gpio_callback(uint gpio, uint32_t events) {
    // Put the GPIO event(s) that just happened into event_str
    // so we can print it
    gpio_event_string(event_str, events);
    printf("GPIO %d %s\n", gpio, event_str);
}

void lcd_task(void *params) {
    while (1) {
        int rand_x = rand() % LCD_WIDTH;
        int rand_y = rand() % LCD_HEIGHT;
        uint16_t rand_color = rand() % 0xffff;
        
        st7789_set_cursor(rand_x, rand_y);
        
        st7789_put(rand_color);
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// void uart_task(void *params) {
//     while (1) {
//         printf("Hello, world!\n");
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

// void watchdog_task(void *params)
// {
//     while(1)
//     {
//         // Afficher l'état des tâches toutes les 5 secondes
//         printf("\n=== System Status ===\n");
//         printf("Free heap: %u bytes\n", xPortGetFreeHeapSize());
//         printf("Tasks running: %u\n", uxTaskGetNumberOfTasks());
        
//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }

int main()
{
    // Initialiser stdio en premier
    stdio_init_all();

    // Délai pour stabiliser USB CDC
    sleep_ms(2000);

    gpio_init(GPIO_WATCH_PIN2);
    gpio_set_dir(GPIO_WATCH_PIN2, GPIO_IN);
    gpio_pull_up(GPIO_WATCH_PIN2);
    gpio_set_irq_enabled_with_callback(GPIO_WATCH_PIN2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Set up our UART
    //uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    //gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    //gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // initialize the lcd
    st7789_init(&lcd_config, LCD_WIDTH, LCD_HEIGHT);

    // make screen black
    st7789_fill(0x0000);
    printf("LCD initialized\n");
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
       
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    printf("GPIO %d initial state: %d\n", GPIO_WATCH_PIN2, gpio_get(GPIO_WATCH_PIN2));

    printf("Free heap before task creation: %u bytes\n", xPortGetFreeHeapSize());

    /* Initialize RAK3172 */
    RAK3172_Init();
    
    BaseType_t xResult;

    xResult = xTaskCreate(lcd_task, "LCD_Task", 512, NULL, 1, NULL);
    if(xResult != pdPASS)
    {
        printf("ERROR: Failed to create LCD task\n");
    }

    // xTaskCreate(uart_task, "UART_Task", 256, NULL, 1, NULL);

    xResult = xTaskCreate(Task_CLI, "CLI_Task", 768, NULL, 1, NULL);
    if(xResult != pdPASS)
    {
        printf("ERROR: Failed to create CLI task\n");
    }

    // xResult = xTaskCreate(watchdog_task, "Watchdog", 256, NULL, 1, NULL);

    printf("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    printf("ERROR: Scheduler failed to start!\n");
    while (true) {

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

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // Désactiver les interruptions pour éviter d'autres problèmes
    taskDISABLE_INTERRUPTS();
    
    printf("\n\n!!! STACK OVERFLOW DETECTED !!!\n");
    printf("Task: %s\n", pcTaskName);
    printf("Handle: %p\n", xTask);
    
    // Afficher l'état de la heap
    printf("Free heap: %u bytes\n", xPortGetFreeHeapSize());
    
    // Bloquer ici
    while(1)
    {
        tight_loop_contents();
    }
}
#endif

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    
    printf("\n\n!!! MALLOC FAILED !!!\n");
    printf("Out of heap memory!\n");
    printf("Free heap: %u bytes\n", xPortGetFreeHeapSize());
    
    while(1)
    {
        tight_loop_contents();
    }
}

// Idle hook pour détecter si le système tourne
void vApplicationIdleHook(void)
{
    static uint32_t ulIdleCount = 0;
    ulIdleCount++;
    
    // Toutes les 100000 itérations, clignoter la LED intégrée si disponible
    if((ulIdleCount % 100000) == 0)
    {
        // Optionnel : toggle LED pour indiquer que le système tourne
        // gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
    }
}

void vConfigureTimerForRunTimeStats(void)
{
    ulHighFrequencyTimerTicks = 0;
}

uint32_t ulGetRunTimeCounterValue(void)
{
    return time_us_32() / 100;  // Retourne en centièmes de microsecondes
}
