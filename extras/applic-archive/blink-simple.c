/* Include Files */

#include "HL_sys_common.h"

/* Include FreeRTOS scheduler files */
#include "FreeRTOS.h"
#include "os_task.h"

/* Include HET header file - types, definitions and function declarations for system driver */
#include "HL_het.h"
#include "HL_gio.h"

/* define FEATURE_CONSOLE if you want to send logging to the CCS console */
#define FEATURE_CONSOLE

/* define FEATURE_SERIAL if you want to send output to SCI3 */
#undef FEATURE_SERIAL

#ifdef FEATURE_SERIAL
/* Serial port SCI3 */
#include "HL_sci.h"
#endif

/* Define Task Handles */
xTaskHandle xTask1Handle;
xTaskHandle xTask2Handle;

/* Task1 */
void vTask1(void *pvParameters)
{
    for(;;)
    {
        /* Taggle HET[1] with timer tick */
        gioSetBit(hetPORT1, 17, gioGetBit(hetPORT1, 17) ^ 1);

#ifdef FEATURE_SERIAL
        sciSend(sciREG3, 10, "abcdefghij");
#endif
        vTaskDelay(100);
    }
}

/* Task2 */
void vTask2(void *pvParameters)
{
    for(;;)
    {
        /* Taggle HET[1] with timer tick */
        gioSetBit(hetPORT1, 25, gioGetBit(hetPORT1, 25) ^ 1);
        // sciSendByte(sciREG3, 'x');
        vTaskDelay(500);
    }
}
void applic(void)
{

    /* Set high end timer GIO port hetPort pin direction to all output */
    gioSetDirection(hetPORT1, 0xFFFFFFFF);

#ifdef FEATURE_SERIAL
    /* Start serial */
    sciInit();
#endif

    /* Create Task 1 */
    if (xTaskCreate(vTask1,"Task1", configMINIMAL_STACK_SIZE, NULL, 1, &xTask1Handle) != pdTRUE)
    {
        /* Task could not be created */
        while(1);
    }

    /* Create Task 2 */
    if (xTaskCreate(vTask2,"Task2", configMINIMAL_STACK_SIZE, NULL, 1, &xTask2Handle) != pdTRUE)
    {
        /* Task could not be created */
        while(1);
    }

    /* Start Scheduler */
    vTaskStartScheduler();

    /* Run forever */
    while(1);

    /* not reached */
}

void vApplicationTickHook( void )
{
    gioSetBit(hetPORT1, 29, gioGetBit(hetPORT1, 29) ^ 1);
}
