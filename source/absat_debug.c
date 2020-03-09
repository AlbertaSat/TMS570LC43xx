/*
    Debuging utilities

    There is a designated debug task that maintains a queue of uint16_t 
    packets that support the sending of 64 multiplexed channels of 
    7-bit data, usually ascii.

    Each packet is then sent over the serial port to a de-multiplexor 
    which presents the data to the user at the debug terminal.

    There is also a single 8-bit channel, usually ascii, from the debug 
    terminal back to the debug task, so that some control can be exercised
    over debugging.

    Functions called by user application::

    DebugInit - call once during initialization, allocates the queue and 
        creates the debug task.  The user application must provide a
        function, 
            void DebugHandleCmd( uint32_t recByte ) 
        that handles commands sent from the debugging channel.

    DebugSendStr( chanNum, char *s ) - send the string s over channel chanNum

    DebugSendBuf( chanNum, char *buf, uint32_t bufSize ) - send the contents
        of buffer s over channel chanNum

    Functions called by debugger task:

    DebugSendPacket - enqueues a debug packet for shipping by DebugTask

    DebugTask - the task that handles shipping packets to the debug monitor,
        and accepting commands and dispatching them

    SciSendStr, SciSendBuf - low level routines for when we need to debug the 
        debugger.

*/

#include "absat_debug.h"
#include <HL_sci.h>

/* the message queue and the task that handles it */
static QueueHandle_t debugQ;
xTaskHandle DebugTaskHandle;

_CODE_ACCESS QueueHandle_t DebugInit( void ) {

    debugQ = xQueueCreate( debugQSize, debugQPacketSize );

    if( debugQ == NULL ) {
        while(1);
        }

    if (xTaskCreate(DebugTask,"DebugTask", configMINIMAL_STACK_SIZE, NULL, 
        1, &DebugTaskHandle) != pdTRUE)
    {
        /* Task could not be created */
        while(1);
        }

    return debugQ;
    }

/*
    Queue messages to send
*/


_CODE_ACCESS void DebugSendStr( int chanNum, char *s )
{
    uint16_t streamMask = 0;
    streamMask = ((chanNum & 0x1f) | 0x80) << 8;; 

    while ( *s != '\0' ) {
        DebugSendPacket( streamMask | (*s & 0x7f) );
        s++;
        }
    }

_CODE_ACCESS void DebugSendBuf( int chanNum, char *buf, uint32_t bufSize )
{
    uint16_t streamMask = 0;
    streamMask = ((chanNum & 0x1f) | 0x80) << 8;; 

    while ( bufSize > 0 && *buf != '\0' ) {
        DebugSendPacket( streamMask | (*buf & 0x7f) );
        buf++;
        bufSize--;
        }
    }


/*
    Send a packet to the debug task
*/

_CODE_ACCESS BaseType_t DebugSendPacket( debugQPacket_t packet ) {

    BaseType_t debugQStatus;

    debugQStatus = xQueueSendToBack( debugQ, &packet, 0 );

    // ok if debugQStatus == pdPASS
    return debugQStatus;
    }

_CODE_ACCESS void DebugTask( void *pvParameters ) {

    /* process messages in the queue and send off to the serial monitor */

    debugQPacket_t packet;
    BaseType_t debugQStatus;
    // const TickType_t xTicksToWait = pdMS_TO_TICKS( 100 );

    while ( 1 ) {

        // if received a character, call the handler routine
        if ( sciIsRxReady(sciREG3) ) {
            uint32_t recByte = sciReceiveByte(sciREG3);
            DebugHandleCmd( recByte );
            }

        // if message waiting, fetch the packet and send out the 2 characters

        if( uxQueueMessagesWaiting( debugQ ) > 0  ) {
            // don't block
            debugQStatus = xQueueReceive( debugQ, &packet, 0 );
            if( debugQStatus == pdPASS ) {
                /* send packet off to serial port */
                /* we can send 1 or 2 bytes depending on the mode */
                /* we are using mode 2 - 32 channels */

                sciSendByte(sciREG3, ( packet >> 8 ) & 0xff );

                // 7-bit data
                sciSendByte(sciREG3, packet & 0x7f );
                }
            }
        }

    }


/* 
    SCI3 string and buffer routines

    These are direct output to SCI3 - not used except for low level debugging  

    For some reason, SCI3 is in interrupt mode, so 
        sciSend(sciREG3, bufLen, (uint8 *) buf); 
    fails. We use a loop around the polling
        sciSendByte(sciREG3, ... ); 
    instead.
*/

_CODE_ACCESS void SciSendStr( char *s )
{
    while ( *s != '\0' ) {
        sciSendByte(sciREG3, *s);
        s++;
        }
    }

/* Send a buffer to SCI3 */
_CODE_ACCESS void SciSendBuf( char *buf, uint32_t bufSize )
{
    while ( bufSize > 0 && *buf != '\0' ) {
        sciSendByte(sciREG3, *buf);
        buf++;
        bufSize--;
        }
    }
