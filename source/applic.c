/*  
    Demo program for core Alberta Sat features on the main cpu Cortex R5
    on a TMS570LC43xx development kit running FreeRTOS 9.

    blink-sci3-io

    This demo has three tasks.  

    Task 1 and 2 blink LEDs on the board.
    Every activation, they send their task number as a single character to SCI3.

    Task 3 looks for input from SCI3. When a character is received, it
    sends its task number to SCI3, Every time a task is invoked, acts
    on the command indicated by the character, and then echos the
    character back to SCI3.

    Commands are described in Task3 below

NOTE: this version is doing an experiment with stdio.h to see if snprintf
is going to cause malloc issues.

*/

/* Include Files */

#include "HL_sys_common.h"

/* Include FreeRTOS scheduler files */
#include "FreeRTOS.h"
#include "os_task.h"

/* Include HET header file - types, definitions and function declarations for system driver */
#include "HL_het.h"
#include "HL_gio.h"

/* Include Alberta Sat library */
#include "HL_sci.h"
#include "absat_lib.h"
#include "absat_debug.h"

/* CAN bus testing */
#define FEATURE_CAN
#undef FEATURE_CAN_DUAL_SEND
#ifdef FEATURE_CAN
#include "HL_can.h"
#endif

/* Define Task Handles */
xTaskHandle xTask1Handle;
xTaskHandle xTask2Handle;

/* Blinking LEDS with SCI3 serial port output and input

LEDs on hetPORT1 pin number:
    D3 Left Top     17
    D4 Top          31
    D5 Right Top    00
    D6 Right Bottom 25
    D7 Bottom       18
    D8 Left bottom  29
    LED1 Left       27
    LED2 Right      05
*/

/* Task1 */
const TickType_t task1DelayInc = 100;
const TickType_t task2DelayInc = 110;

/* task1Delay is shared, needs to be marked as volatile */
volatile static TickType_t task1Delay;
volatile static uint8_t task1EnableDelay;
volatile static uint8_t task1EnableSerial;

volatile static TickType_t task2Delay;
volatile static uint8_t task2EnableDelay;
volatile static uint8_t task2EnableSerial;

void vTask1(void *pvParameters)
{
    // Task1 uses channel 1
    int chanNum = 1;

    uint32_t rc;

    // message buffer
    size_t bufSize = 64;
    char buf[32];

    // CAN data buffer
    size_t dataSize = 8;
    uint8_t data[8];

    int32_t tag = 0;
    int32_t i;

    for(;;)
    {
        /* Toggle Left Top HET[1] with timer tick */
        gioSetBit(hetPORT1, 17, gioGetBit(hetPORT1, 17) ^ 1);

        #ifdef FEATURE_CAN
        /* 
            Task1 is going to send CAN messages on canREG1
        */
        tag++;
        data[0] = tag;
        for ( i = 1; i < dataSize; i++ ) {
            data[i] = i;
            }

        while ( canIsTxMessagePending(canREG1, canMESSAGE_BOX1) ) {
            }

        rc = canTransmit(canREG1, canMESSAGE_BOX1, data);

        if ( task1EnableSerial ) {
            buf[0] = '\0';
            StrApStr(buf, bufSize, "<");
            for ( i = 0; i < dataSize; i++ ) {
                StrApDec(buf, bufSize, data[i]);
                StrApStr(buf, bufSize, "|");
                }
            StrApStr(buf, bufSize, " rc=");
            StrApDec(buf, bufSize, rc);
            StrApStr(buf, bufSize, ">\n");
            DebugSendStr(chanNum, buf);
            }
        #endif

        if ( task1EnableDelay ) {
            vTaskDelay(task1Delay);
            }
    }
}

#ifdef FEATURE_CAN_DUAL_SEND
/* Task2 - with two senders to CAN bus */
void vTask2(void *pvParameters)
{
    // Task2 uses channel 2
    int chanNum = 2;

    uint32_t rc;

    // message buffer
    size_t bufSize = 64;
    char buf[32];

    // CAN data buffer
    size_t dataSize = 8;
    uint8_t data[8];

    int32_t trycnt;
    int32_t tag = 0;
    int32_t i;

    for(;;)
    {
        /* Toggle Left Top HET[1] with timer tick */
        gioSetBit(hetPORT1, 17, gioGetBit(hetPORT1, 17) ^ 1);

        #ifdef FEATURE_CAN
        /* 
            Task2 is going to send CAN messages on canREG2
        */
        tag++;
        data[0] = tag;
        for ( i = 1; i < dataSize; i++ ) {
            data[i] = 10*i;
            }

        if ( task2EnableSerial ) {
            buf[0] = '\0';
            StrApStr(buf, bufSize, "TRY2\n");
            DebugSendStr(chanNum, buf);
            }

        trycnt = 200;
        while ( trycnt > 0 ) {
            rc = canTransmit(canREG2, canMESSAGE_BOX2, data);

            if ( rc ) { 
                if ( task2EnableSerial ) {
                    buf[0] = '\0';
                    StrApStr(buf, bufSize, "OK: ");
                    StrApDec(buf, bufSize, trycnt);
                    StrApStr(buf, bufSize, "\n");
                    DebugSendStr(chanNum, buf);
                    }
                }
            trycnt--;
            }

        if ( trycnt <= 0 ) {
            if ( task2EnableSerial ) {
                buf[0] = '\0';
                StrApStr(buf, bufSize, "FAIL\n");
                DebugSendStr(chanNum, buf);
                }
            }
            
        if ( task2EnableSerial ) {
            buf[0] = '\0';
            StrApStr(buf, bufSize, "<");
            for ( i = 0; i < dataSize; i++ ) {
                StrApDec(buf, bufSize, data[i]);
                StrApStr(buf, bufSize, "|");
                }
            StrApStr(buf, bufSize, " rc=");
            StrApDec(buf, bufSize, rc);
            StrApStr(buf, bufSize, ">\n");
            DebugSendStr(chanNum, buf);
            }

        #endif

        if ( task2EnableDelay ) {
            vTaskDelay(task2Delay);
            }
    }
}
#else
/* Task2  - receive task */
void vTask2(void *pvParameters)
{
    // Task2 uses channel 2
    int chanNum = 2;

    uint32_t rc;

    // message buffer
    size_t bufSize = 64;
    char buf[32];

    // CAN data buffer
    size_t dataSize = 8;
    uint8_t data[8];

    int32_t i;

    for(;;)
    {

        /* Toggle Right Bottom HET[1] with timer tick */
        gioSetBit(hetPORT1, 25, gioGetBit(hetPORT1, 25) ^ 1);

        #ifdef FEATURE_CAN
        /* 
            Task2 is going to receive CAN messages on canREG2
        */
        /* clear the data buffer */
        for ( i = 0; i < dataSize; i++ ) {
            data[i] = 0;
            }

        while(!  canIsRxMessageArrived(canREG2, canMESSAGE_BOX1) ) {
            }

        rc = canGetData(canREG2, canMESSAGE_BOX1, data);

        if ( task2EnableSerial ) {
            buf[0] = '\0';
            StrApStr(buf, bufSize, "<");
            for ( i = 0; i < dataSize; i++ ) {
                StrApDec(buf, bufSize, data[i]);
                StrApStr(buf, bufSize, "|");
                }
            StrApStr(buf, bufSize, " rc=");
            StrApDec(buf, bufSize, rc);
            StrApStr(buf, bufSize, ">\n");
            DebugSendStr(chanNum, buf);
            }
        #endif

        if ( task2EnableDelay ) {
            vTaskDelay(task2Delay);
            }
    }
}
#endif

void DebugHandleCmd( uint32 cmdByte )
{
    // Debug Task uses channel 0
    int chanNum = 0;

    // message buffer
    size_t bufSize = 64;
    char buf[64];
    int32_t bufLen = 0;

    /* process the command character */

    /* wait for a control command from SCI3 
        u - increase task1 delay
        d - decrease task1 delay
        r - reset task1 delay
        n - no task1 delay
        f - turn off task1 serial output
        g - turn on task1 serial output

        U - increase task2 delay
        D - decrease task2 delay
        R - reset task2 delay
        N - no task2 delay
        F - turn off task2 serial output
        G - turn on task2 serial output
    */

    /* references and updates to task1Delay need to be in a critical section */

    /* ensure taskDelay never set below 0 or greater than 50 * inc */

    switch ( cmdByte ) {
    case 'u':
        if ( task1Delay < 50 * task1DelayInc ) { 
            task1Delay += task1DelayInc; 
            task1EnableDelay = 1;
            }
        break;
    case 'd':
        if ( task1Delay > task1DelayInc ) { 
            task1Delay -= task1DelayInc; 
            task1EnableDelay = 1;
            }
        break;
    case 'r':
        task1Delay = 2 * task1DelayInc;
        task1EnableDelay = 1;
        break;
    case 'n':
        task1EnableDelay = 0;
        break;
    case 'f':
        task1EnableSerial = 0;
        break;
    case 'g':
        task1EnableSerial = 1;
        break;

    case 'U':
        if ( task2Delay < 50 * task2DelayInc ) { 
            task2Delay += task2DelayInc; 
            task2EnableDelay = 1;
            }
        break;
    case 'D':
        if ( task2Delay > task2DelayInc ) { 
            task2Delay -= task2DelayInc; 
            task2EnableDelay = 1;
            }
        break;
    case 'R':
        task2Delay = 2 * task2DelayInc;
        task2EnableDelay = 1;
        break;
    case 'N':
        task2EnableDelay = 0;
        break;
    case 'F':
        task2EnableSerial = 0;
        break;
    case 'G':
        task2EnableSerial = 1;
        break;

    // default:
    }

    bufLen = 0;
    buf[0] = '\0';
    bufLen += StrApStr(buf, bufSize, "<");
    bufLen += StrApChar(buf, bufSize, cmdByte);
    bufLen += StrApStr(buf, bufSize, ", ");
    bufLen += StrApDec(buf, bufSize, cmdByte);
    bufLen += StrApStr(buf, bufSize, ">\n");

    DebugSendBuf(chanNum, buf, bufLen);
    }

void applic(void)
{

    /* Set high end timer GIO port hetPort pin direction to all output */
    gioSetDirection(hetPORT1, 0xFFFFFFFF);

    /* guard for non init */
    DebugInit();

    /* Start serial */
    sciInit();

    /* turn off SCI3 interrupts so we are in polling mode.  Interrupt mode is non
       blocking so we need to me more careful when calling the sci send and receive
       functions.  This actually doesn't seem to do anything.
    */

#ifdef IGNORE
    sciDisableNotification(sciREG3, 
        SCI_FE_INT ||
        SCI_OE_INT ||
        SCI_PE_INT ||
        SCI_RX_INT ||
        SCI_TX_INT ||
        SCI_WAKE_INT  ||
        SCI_BREAK_INT ||
        0 );
#endif

#ifdef FEATURE_CAN
    canInit();
#endif

    /* control variables */
    task1Delay = 20 * task1DelayInc;
    task1EnableDelay = 1;
    task1EnableSerial = 1;

    task2Delay = 30 * task2DelayInc;
    task2EnableDelay = 1;
    task2EnableSerial = 1;

    /* Create Task 1 - set defaults before creation to ensure initialized on restart */

    if (xTaskCreate(vTask1,"Task1", 
        configMINIMAL_STACK_SIZE, NULL, 1, &xTask1Handle) != pdTRUE)
    {
        /* Task could not be created */
        while(1);
    }

    /* Create Task 2 */
    if (xTaskCreate(vTask2,"Task2", 
        configMINIMAL_STACK_SIZE, NULL, 1, &xTask2Handle) != pdTRUE)
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
