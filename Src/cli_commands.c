#include "FreeRTOS.h"
#include "task.h"
#include "cli_prv.h"
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include <stdio.h>
#include <string.h>

/* Command: ps - List all tasks */
static void prvPsCommand(ConsoleIO_t * const pxConsoleIO,
                         uint32_t ulArgc,
                         char * ppcArgv[])
{
    char pcBuffer[256];
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    unsigned long ulTotalRunTime, ulStatsAsPercentage;

    /* Get number of tasks */
    uxArraySize = uxTaskGetNumberOfTasks();

    /* Allocate array for task status */
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if(pxTaskStatusArray != NULL)
    {
        /* Get task information */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

        /* Avoid divide by zero */
        if(ulTotalRunTime > 0)
        {
            pxConsoleIO->print("\r\nTask Name\t\tState\tPrio\tStack\tNum\r\n");
            pxConsoleIO->print("********************************************************\r\n");

            /* For each populated position in the array */
            for(x = 0; x < uxArraySize; x++)
            {
                /* Calculate percentage of total runtime */
                ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / (ulTotalRunTime / 100UL);

                if(ulStatsAsPercentage > 0UL)
                {
                    snprintf(pcBuffer, sizeof(pcBuffer),
                            "%s\t\t%c\t%u\t%u\t%u\t%u%%\r\n",
                            pxTaskStatusArray[x].pcTaskName,
                            pxTaskStatusArray[x].eCurrentState == eRunning ? 'X' : 
                            pxTaskStatusArray[x].eCurrentState == eReady ? 'R' : 
                            pxTaskStatusArray[x].eCurrentState == eBlocked ? 'B' : 
                            pxTaskStatusArray[x].eCurrentState == eSuspended ? 'S' : 'D',
                            pxTaskStatusArray[x].uxCurrentPriority,
                            pxTaskStatusArray[x].usStackHighWaterMark,
                            pxTaskStatusArray[x].xTaskNumber,
                            ulStatsAsPercentage);
                }
                else
                {
                    snprintf(pcBuffer, sizeof(pcBuffer),
                            "%s\t\t%c\t%u\t%u\t%u\t<1%%\r\n",
                            pxTaskStatusArray[x].pcTaskName,
                            pxTaskStatusArray[x].eCurrentState == eRunning ? 'X' : 
                            pxTaskStatusArray[x].eCurrentState == eReady ? 'R' : 
                            pxTaskStatusArray[x].eCurrentState == eBlocked ? 'B' : 
                            pxTaskStatusArray[x].eCurrentState == eSuspended ? 'S' : 'D',
                            pxTaskStatusArray[x].uxCurrentPriority,
                            pxTaskStatusArray[x].usStackHighWaterMark,
                            pxTaskStatusArray[x].xTaskNumber);
                }

                pxConsoleIO->print(pcBuffer);
            }
        }

        /* Free allocated memory */
        vPortFree(pxTaskStatusArray);
    }
    else
    {
        pxConsoleIO->print("Error: Unable to allocate memory for task list\r\n");
    }
}

const CLI_Command_Definition_t xCommandDef_ps =
{
    "ps",
    "ps:\r\n"
    "    List all running tasks with their status\r\n"
    "    Usage: ps\r\n\n",
    prvPsCommand
};

/* Command: heap - Show heap statistics */
static void prvHeapStatCommand(ConsoleIO_t * const pxConsoleIO,
                               uint32_t ulArgc,
                               char * ppcArgv[])
{
    char pcBuffer[128];
    HeapStats_t xHeapStats;

    vPortGetHeapStats(&xHeapStats);

    snprintf(pcBuffer, sizeof(pcBuffer),
            "\r\nHeap Statistics:\r\n"
            "  Available heap space: %u bytes\r\n"
            "  Largest free block:   %u bytes\r\n"
            "  Smallest free block:  %u bytes\r\n"
            "  Number of free blocks: %u\r\n"
            "  Minimum ever free:    %u bytes\r\n"
            "  Successful allocs:    %u\r\n"
            "  Successful frees:     %u\r\n\n",
            (unsigned int)xHeapStats.xAvailableHeapSpaceInBytes,
            (unsigned int)xHeapStats.xSizeOfLargestFreeBlockInBytes,
            (unsigned int)xHeapStats.xSizeOfSmallestFreeBlockInBytes,
            (unsigned int)xHeapStats.xNumberOfFreeBlocks,
            (unsigned int)xHeapStats.xMinimumEverFreeBytesRemaining,
            (unsigned int)xHeapStats.xNumberOfSuccessfulAllocations,
            (unsigned int)xHeapStats.xNumberOfSuccessfulFrees);

    pxConsoleIO->print(pcBuffer);
}

const CLI_Command_Definition_t xCommandDef_heapStat =
{
    "heap",
    "heap:\r\n"
    "    Display heap memory statistics\r\n"
    "    Usage: heap\r\n\n",
    prvHeapStatCommand
};

/* Command: reset - Reset the RP2040 */
static void prvResetCommand(ConsoleIO_t * const pxConsoleIO,
                            uint32_t ulArgc,
                            char * ppcArgv[])
{
    pxConsoleIO->print("Resetting RP2040...\r\n");
    
    /* Small delay to allow message to be sent */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    /* Reset using watchdog */
    watchdog_enable(1, 1);
    while(1);
}

const CLI_Command_Definition_t xCommandDef_reset =
{
    "reset",
    "reset:\r\n"
    "    Reset the RP2040 microcontroller\r\n"
    "    Usage: reset\r\n\n",
    prvResetCommand
};

/* Command: clear - Clear the terminal screen */
static void prvClearCommand(ConsoleIO_t * const pxConsoleIO,
                           uint32_t ulArgc,
                           char * ppcArgv[])
{
    /* ANSI escape sequence to clear screen and move cursor to home */
    pxConsoleIO->print("\033[2J\033[H");
}

const CLI_Command_Definition_t xCommandDef_clear =
{
    "clear",
    "clear:\r\n"
    "    Clear the terminal screen\r\n"
    "    Usage: clear\r\n\n",
    prvClearCommand
};

/* Command: uptime - Show system uptime */
static void prvUptimeCommand(ConsoleIO_t * const pxConsoleIO,
                             uint32_t ulArgc,
                             char * ppcArgv[])
{
    char pcBuffer[128];
    TickType_t xUptime = xTaskGetTickCount();
    
    uint32_t ulSeconds = xUptime / configTICK_RATE_HZ;
    uint32_t ulMinutes = ulSeconds / 60;
    uint32_t ulHours = ulMinutes / 60;
    uint32_t ulDays = ulHours / 24;
    
    ulSeconds %= 60;
    ulMinutes %= 60;
    ulHours %= 24;
    
    snprintf(pcBuffer, sizeof(pcBuffer),
            "\r\nSystem uptime: %lu days, %lu hours, %lu minutes, %lu seconds\r\n"
            "Total ticks: %lu\r\n\n",
            ulDays, ulHours, ulMinutes, ulSeconds,
            (unsigned long)xUptime);
    
    pxConsoleIO->print(pcBuffer);
}

const CLI_Command_Definition_t xCommandDef_uptime =
{
    "uptime",
    "uptime:\r\n"
    "    Display system uptime\r\n"
    "    Usage: uptime\r\n\n",
    prvUptimeCommand
};
