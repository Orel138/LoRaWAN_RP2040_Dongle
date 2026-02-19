#include "rak3172.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

/* Internal state */
static QueueHandle_t xRxQueue = NULL;
static QueueHandle_t xEventQueue = NULL;
static SemaphoreHandle_t xUartMutex = NULL;
static TaskHandle_t xRAK3172TaskHandle = NULL;
static RAK3172_RxCallback_t pxRxCallback = NULL;

/* RX Buffer */
static char rxBuffer[RAK3172_RX_BUFFER_SIZE];
static volatile uint16_t rxIndex = 0;

/* UART IRQ Handler */
static void rak3172_uart_irq_handler(void)
{
    static uint32_t irqCount = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    irqCount++;
    
    // Debug : afficher chaque 10ème appel
    if((irqCount % 10) == 0)
    {
        RAK_DEBUG("[IRQ] Called %lu times\n", irqCount);
    }
    
    while(uart_is_readable(RAK3172_UART_ID))
    {
        char c = uart_getc(RAK3172_UART_ID);
        
        // Debug : afficher chaque caractère reçu
        RAK_DEBUG("[IRQ] RX: 0x%02X ('%c')\n", c, (c >= 32 && c < 127) ? c : '.');
        
        // Envoyer à la queue
        xQueueSendFromISR(xRxQueue, &c, &xHigherPriorityTaskWoken);
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* Initialize RAK3172 driver */
BaseType_t RAK3172_Init(void)
{
    printf("Initializing RAK3172...\n");
    
    /* Create queues and mutex */
    xRxQueue = xQueueCreate(RAK3172_RX_BUFFER_SIZE, sizeof(char));
    xEventQueue = xQueueCreate(10, sizeof(RAK3172_EventData_t));
    xUartMutex = xSemaphoreCreateMutex();
    
    if(!xRxQueue || !xEventQueue || !xUartMutex)
    {
        printf("ERROR: Failed to create RAK3172 resources\n");
        return pdFAIL;
    }
    
    /* Configure RST pin - TRÈS IMPORTANT */
    printf("Configuring RST pin (GP%d)...\n", RAK3172_RST_PIN);
    gpio_init(RAK3172_RST_PIN);
    gpio_set_dir(RAK3172_RST_PIN, GPIO_OUT);

    /* Initialize UART0 */
    printf("Initializing UART0 for RAK3172...\n");
    uart_init(RAK3172_UART_ID, RAK3172_BAUD_RATE);
    gpio_set_function(RAK3172_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RAK3172_RX_PIN, GPIO_FUNC_UART);
    
    /* Configure UART format (8N1) */
    uart_set_format(RAK3172_UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(RAK3172_UART_ID, true);
    
    printf("UART0 configured: %d baud, 8N1\n", RAK3172_BAUD_RATE);
    
    /* NE PAS configurer l'IRQ - utiliser polling */
    printf("Using polling mode (no IRQ)\n");

    printf("Performing hardware reset of RAK3172...\n");
    
    gpio_put(RAK3172_RST_PIN, 0);  // Assert reset
    sleep_ms(100);
    
    gpio_put(RAK3172_RST_PIN, 1);  // Release reset
    sleep_ms(500);
    
    /* Clear startup messages */
    while(uart_is_readable(RAK3172_UART_ID))
    {
        uart_getc(RAK3172_UART_ID);
    }
    
    printf("Reset complete\n");
    
    /* Create RAK3172 task */
    BaseType_t xResult = xTaskCreate(Task_RAK3172, "RAK3172", 512, NULL, 2, &xRAK3172TaskHandle);
    if(xResult != pdPASS)
    {
        printf("ERROR: Failed to create RAK3172 task\n");
        return pdFAIL;
    }
    
    printf("RAK3172 initialized successfully on UART0 (GP0=TX, GP1=RX)\n");
    
    /* Attendre un peu puis tester la communication */
    //vTaskDelay(pdMS_TO_TICKS(1000));
    
    // /* Vider le buffer (le RAK3172 peut envoyer des messages au démarrage) */
    // printf("Clearing startup messages...\n");
    // while(uart_is_readable(RAK3172_UART_ID))
    // {
    //     char c = uart_getc(RAK3172_UART_ID);
    //     printf("%c", c);
    // }
    // printf("\n");
    
    /* Test de communication */
    // printf("Testing communication with AT command...\n");
    // char testResponse[128];
    // if(RAK3172_SendCommand("AT", testResponse, 2000) == pdPASS)
    // {
    //     printf("✓ RAK3172 communication OK!\n");
    // }
    // else
    // {
    //     printf("⚠ WARNING: No response from RAK3172\n");
    // }
    
    return pdPASS;
}

/* RAK3172 Task - Polling mode */
void Task_RAK3172(void *pvParameters)
{
    char lineBuffer[RAK3172_RX_BUFFER_SIZE];
    uint16_t lineIdx = 0;
    
    printf("RAK3172 task started (polling mode)\n");
    
    vTaskDelay(pdMS_TO_TICKS(500));
    
    while(1)
    {
        /* Poll UART for data */
        while(uart_is_readable(RAK3172_UART_ID))
        {
            char c = uart_getc(RAK3172_UART_ID);
            
            /* Send to queue for RAK3172_SendCommand */
            xQueueSend(xRxQueue, &c, 0);
            
            /* Also process for events */
            if(c == '\n')
            {
                lineBuffer[lineIdx] = '\0';
                
                /* Process line if it's an event */
                if(strstr(lineBuffer, "+EVT:RXP2P"))
                {
                    RAK_DEBUG("LoRa RX: %s\n", lineBuffer);
                    
                    if(pxRxCallback)
                    {
                        RAK3172_RxData_t rxData = {0};
                        pxRxCallback(&rxData);
                    }
                }
                else if(strstr(lineBuffer, "+EVT:"))
                {
                    RAK_DEBUG("RAK3172 Event: %s\n", lineBuffer);
                }
                
                lineIdx = 0;
            }
            else if(c != '\r' && lineIdx < RAK3172_RX_BUFFER_SIZE - 1)
            {
                lineBuffer[lineIdx++] = c;
            }
        }
        
        /* Yield to other tasks */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Hardware reset of RAK3172 */
BaseType_t RAK3172_HardwareReset(void)
{
    printf("Performing hardware reset of RAK3172...\n");
    
    gpio_put(RAK3172_RST_PIN, 0);  // Assert reset
    vTaskDelay(pdMS_TO_TICKS(100));
    
    gpio_put(RAK3172_RST_PIN, 1);  // Release reset
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Clear startup messages */
    while(uart_is_readable(RAK3172_UART_ID))
    {
        uart_getc(RAK3172_UART_ID);
    }
    
    RAK_DEBUG("Reset complete\n");
    
    return pdPASS;
}

/* Send AT command and wait for response */
BaseType_t RAK3172_SendCommand(const char *cmd, char *response, uint32_t timeout_ms)
{
    if(!cmd)
        return pdFAIL;
    
    RAK_DEBUG("\n[DEBUG] === RAK3172_SendCommand ===\n");
    RAK_DEBUG("[DEBUG] Command: %s\n", cmd);
    RAK_DEBUG("[DEBUG] Timeout: %lu ms\n", timeout_ms);
    
    /* Lock UART */
    if(xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        RAK_DEBUG("[DEBUG] ERROR: Failed to acquire mutex\n");
        return pdFAIL;
    }
    
    RAK_DEBUG("[DEBUG] Mutex acquired\n");
    
    /* Clear RX queue */
    UBaseType_t itemsCleared = 0;
    char dummyChar;
    while(xQueueReceive(xRxQueue, &dummyChar, 0) == pdTRUE)
    {
        itemsCleared++;
    }
    if(itemsCleared > 0)
    {
        RAK_DEBUG("[DEBUG] Cleared %u items from RX queue\n", itemsCleared);
    }
    
    /* Check if UART is writable */
    if(!uart_is_writable(RAK3172_UART_ID))
    {
        RAK_DEBUG("[DEBUG] WARNING: UART not writable!\n");
    }
    
    /* Send command character by character for debug */
    RAK_DEBUG("[DEBUG] Sending bytes: ");
    uart_puts(RAK3172_UART_ID, cmd);
    for(size_t i = 0; cmd[i] != '\0'; i++)
    {
        RAK_DEBUG("0x%02X ", (uint8_t)cmd[i]);
    }
    uart_puts(RAK3172_UART_ID, "\r\n");
    RAK_DEBUG("0x0D 0x0A\n");
    
    /* Wait for data to be sent */
    uart_tx_wait_blocking(RAK3172_UART_ID);
    RAK_DEBUG("[DEBUG] Data sent successfully\n");
    
    /* Wait for response */
    char rxChar;
    uint16_t rxIdx = 0;
    TickType_t startTime = xTaskGetTickCount();
    bool responseComplete = false;
    uint32_t charsReceived = 0;
    
    if(response)
        response[0] = '\0';
    
    RAK_DEBUG("[DEBUG] Waiting for response...\n");
    
    while(!responseComplete && (xTaskGetTickCount() - startTime) < pdMS_TO_TICKS(timeout_ms))
    {
        if(xQueueReceive(xRxQueue, &rxChar, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            charsReceived++;
            
            RAK_DEBUG("[DEBUG] RX char %u: 0x%02X ('%c')\n", charsReceived, (uint8_t)rxChar, 
                   (rxChar >= 32 && rxChar < 127) ? rxChar : '.');
            
            if(response && rxIdx < RAK3172_RX_BUFFER_SIZE - 1)
            {
                response[rxIdx++] = rxChar;
                response[rxIdx] = '\0';
                
                /* Check for complete response (ends with OK or ERROR) */
                if(strstr(response, "OK\r\n") || strstr(response, "ERROR"))
                {
                    responseComplete = true;
                    RAK_DEBUG("[DEBUG] Response complete detected\n");
                }
            }
        }
        
        // Afficher un point toutes les 500ms pour montrer qu'on attend
        static TickType_t lastDot = 0;
        if((xTaskGetTickCount() - lastDot) > pdMS_TO_TICKS(500))
        {
            printf(".");
            lastDot = xTaskGetTickCount();
        }
    }
    
    RAK_DEBUG("\n[DEBUG] Chars received: %u\n", charsReceived);
    
    if(response && strlen(response) > 0)
    {
        RAK_DEBUG("[DEBUG] Full response:\n%s\n", response);
    }
    else
    {
        RAK_DEBUG("[DEBUG] No response received\n");
    }
    
    /* Unlock UART */
    xSemaphoreGive(xUartMutex);
    
    RAK_DEBUG("[DEBUG] === End of RAK3172_SendCommand ===\n\n");
    
    return responseComplete ? pdPASS : pdFAIL;
}

/* Get firmware version */
BaseType_t RAK3172_GetVersion(char *version, size_t max_len)
{
    char response[128];
    
    if(RAK3172_SendCommand("AT+VER=?", response, 2000) == pdPASS)
    {
        /* Parse response: AT+VER=X.X.X */
        char *ver = strstr(response, "AT+VER=");
        if(ver)
        {
            ver += 7; // Skip "AT+VER="
            char *end = strchr(ver, '\r');
            if(end)
            {
                size_t len = end - ver;
                if(len < max_len)
                {
                    strncpy(version, ver, len);
                    version[len] = '\0';
                    return pdPASS;
                }
            }
        }
    }
    
    return pdFAIL;
}

/* Join LoRaWAN network */
BaseType_t RAK3172_Join(uint32_t timeout_ms)
{
    char response[256];
    
    if(RAK3172_SendCommand("AT+JOIN=1:0:10:8", response, timeout_ms) == pdPASS)
    {
        if(strstr(response, "+EVT:JOINED"))
        {
            return pdPASS;
        }
    }
    
    return pdFAIL;
}

/* Send confirmed data */
BaseType_t RAK3172_SendData(uint8_t port, const uint8_t *data, uint16_t length)
{
    if(!data || length == 0 || length > RAK3172_MAX_PAYLOAD)
        return pdFAIL;
    
    char cmd[600];
    char hexData[RAK3172_MAX_PAYLOAD * 2 + 1];
    
    /* Convert data to hex string */
    for(uint16_t i = 0; i < length; i++)
    {
        sprintf(&hexData[i * 2], "%02X", data[i]);
    }
    hexData[length * 2] = '\0';
    
    /* Build command */
    snprintf(cmd, sizeof(cmd), "AT+SEND=%d:%s", port, hexData);
    
    char response[256];
    return RAK3172_SendCommand(cmd, response, 30000);
}

/* Send unconfirmed data */
BaseType_t RAK3172_SendDataUnconfirmed(uint8_t port, const uint8_t *data, uint16_t length)
{
    if(!data || length == 0 || length > RAK3172_MAX_PAYLOAD)
        return pdFAIL;
    
    char cmd[600];
    char hexData[RAK3172_MAX_PAYLOAD * 2 + 1];
    
    /* Convert data to hex string */
    for(uint16_t i = 0; i < length; i++)
    {
        sprintf(&hexData[i * 2], "%02X", data[i]);
    }
    hexData[length * 2] = '\0';
    
    /* Build command */
    snprintf(cmd, sizeof(cmd), "AT+SEND=%d:%s", port, hexData);
    
    char response[256];
    return RAK3172_SendCommand(cmd, response, 10000);
}

/* Set DevEUI */
BaseType_t RAK3172_SetDevEUI(const char *deveui)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+DEVEUI=%s", deveui);
    char response[128];
    return RAK3172_SendCommand(cmd, response, 2000);
}

/* Set AppEUI */
BaseType_t RAK3172_SetAppEUI(const char *appeui)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+APPEUI=%s", appeui);
    char response[128];
    return RAK3172_SendCommand(cmd, response, 2000);
}

/* Set AppKey */
BaseType_t RAK3172_SetAppKey(const char *appkey)
{
    char cmd[80];
    snprintf(cmd, sizeof(cmd), "AT+APPKEY=%s", appkey);
    char response[128];
    return RAK3172_SendCommand(cmd, response, 2000);
}

/* Set region */
BaseType_t RAK3172_SetRegion(const char *region)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+BAND=%s", region);
    char response[128];
    return RAK3172_SendCommand(cmd, response, 2000);
}

/* Register RX callback */
BaseType_t RAK3172_RegisterRxCallback(RAK3172_RxCallback_t callback)
{
    pxRxCallback = callback;
    return pdPASS;
}
