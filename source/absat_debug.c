/* OPTIMIZE - make packets longer */

/*
    Debuging utilities

    There is a designated debug task that maintains a queue of uint16_t 
    packets that support the sending of 32 multiplexed channels of 
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

    DebugSendStr( int chanNum, char *s ) 
        - send the string s over channel chanNum
    DebugSendStrFromISR - interrupt service routine version

    DebugSendBuf( int chanNum, char *buf, uint32_t bufSize ) 
        - send the contents of buffer s over channel chanNum
    DebugSendBufFromISR - interrupt service routine version

    DebugSendFmt( chanNum, const char * format, ... )
        - printf-like version, stack allocated buffer, limited formats
        no modifiers, e.g. no %2d (see StrApFmt in absat_lib.c).  
        Format specs:
            %s - string pointer
            %d - signed decimal value
            %x - 32 bit unsigned hex value, suppress leading 0
            %X - 32 bit unsigned hex value, all 8 digits
            %% - the % sign
    DebugSendFmtFromISR - interrupt service routine version

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

    /* HJH - Need to specify the stack size for the debug task */

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
    debugQPacket_t packet;
    int i;
    int itemIdx;
    uint16_t item;

    /* Each packet consists of debugPacketNumItems, each item being
       a uint16_t of channel and data.  If there are fewer than
       debugPacketNumItems, the remaining are set to 0.
    */

    i=0;
    while ( *s != '\0' ) {
        item = streamMask | (*s & 0x7f);
        itemIdx = i & debugItemAddrMask;   // modular counter
        packet[ itemIdx ] = item;

        // send off a packet when debugPacketNumItems items processed or 
        // s is done, i.e. next char is a \0

        if ( itemIdx == debugItemAddrMask ) {
            DebugSendPacket( packet );
            }
        else if ( s[1] == '\0' ) {
            // put in a 0 item, safe to do since itemIdx is not at max
            packet[itemIdx +1] = 0;
            DebugSendPacket( packet );
            break;
            }
        i++;
        s++;
        }
    }

_CODE_ACCESS void DebugSendBuf( int chanNum, char *buf, uint32_t bufSize )
{
    uint16_t streamMask = 0;
    streamMask = ((chanNum & 0x1f) | 0x80) << 8;; 
    debugQPacket_t packet;
    int i;
    int itemIdx;
    uint16_t item;

    for (i=0; 0 < bufSize ; i++ ) {
        item = streamMask | (*buf & 0x7f);
        itemIdx = i & debugItemAddrMask;   // modular counter
        packet[ itemIdx ] = item;

        // send off a packet when debugPacketNumItems items processed or 
        // buf is done, i.e. next char is a \0 or we are at last position
        // of buffer.

        if ( itemIdx == debugItemAddrMask ) {
            DebugSendPacket( packet );
            }
        else if ( buf[1] == '\0' || 1 == bufSize ) {
            // put in a 0 item, safe to do since itemIdx is not at max
            packet[itemIdx +1] = 0;
            DebugSendPacket( packet );
            break;
            }
        i++;
        buf++;
        bufSize--;
        }
    }

// default buffer sizes for normal version of DebugSendFmt
#define BUFSIZE 64

_CODE_ACCESS void DebugSendFmt( int chanNum, const char * format, ... )
{
    va_list ap;
    va_start(ap, format);

    int bufSize = BUFSIZE;
    char buf[BUFSIZE];

    // make sure buffer is a proper \0 terminated string
    buf[0] = '\0';

    (void) StrApFmtV(buf, bufSize, format, ap);
    va_end(ap);

    DebugSendStr(chanNum, buf);
    }

/*
    Send a packet to the debug task
*/

_CODE_ACCESS BaseType_t DebugSendPacket( debugQPacket_t packet ) {

    BaseType_t debugQStatus;

    debugQStatus = xQueueSendToBack( debugQ, packet, 0 );

    // ok if debugQStatus == pdPASS
    return debugQStatus;
    }

/*
    Queue messages to send - ISR version
*/

_CODE_ACCESS void DebugSendStrFromISR( int chanNum, char *s )
{
    uint16_t streamMask = 0;
    streamMask = ((chanNum & 0x1f) | 0x80) << 8;; 
    debugQPacket_t packet;
    int i;
    int itemIdx;
    uint16_t item;

    /* Each packet consists of debugPacketNumItems, each item being
       a uint16_t of channel and data.  If there are fewer than
       debugPacketNumItems, the remaining are set to 0.
    */

    i=0;
    while ( *s != '\0' ) {
        item = streamMask | (*s & 0x7f);
        itemIdx = i & debugItemAddrMask;   // modular counter
        packet[ itemIdx ] = item;

        // send off a packet when debugPacketNumItems items processed or 
        // s is done, i.e. next char is a \0

        if ( itemIdx == debugItemAddrMask ) {
            DebugSendPacketFromISR( packet );
            }
        else if ( s[1] == '\0' ) {
            // put in a 0 item, safe to do since itemIdx is not at max
            packet[itemIdx +1] = 0;
            DebugSendPacketFromISR( packet );
            break;
            }
        i++;
        s++;
        }
    }

_CODE_ACCESS void DebugSendBufFromISR( int chanNum, char *buf, uint32_t bufSize )
{
    uint16_t streamMask = 0;
    streamMask = ((chanNum & 0x1f) | 0x80) << 8;; 
    debugQPacket_t packet;
    int i;
    int itemIdx;
    uint16_t item;

    for (i=0; 0 < bufSize ; i++ ) {
        item = streamMask | (*buf & 0x7f);
        itemIdx = i & debugItemAddrMask;   // modular counter
        packet[ itemIdx ] = item;

        // send off a packet when debugPacketNumItems items processed or 
        // buf is done, i.e. next char is a \0 or we are at last position
        // of buffer.

        if ( itemIdx == debugItemAddrMask ) {
            DebugSendPacketFromISR( packet );
            }
        else if ( buf[1] == '\0' || 1 == bufSize ) {
            // put in a 0 item, safe to do since itemIdx is not at max
            packet[itemIdx +1] = 0;
            DebugSendPacketFromISR( packet );
            break;
            }
        i++;
        buf++;
        bufSize--;
        }
    }

// default buffer sizes for ISR versions of DebugSendFmt
#define BUFSIZE_ISR 64

void DebugSendFmtFromISR( int chanNum, const char * format, ... )
{
    va_list ap;
    va_start(ap, format);

    int bufSize = BUFSIZE_ISR;
    char buf[BUFSIZE_ISR];

    // make sure buffer is a proper \0 terminated string
    buf[0] = '\0';

    (void) StrApFmtV(buf, bufSize, format, ap);
    va_end(ap);

    DebugSendStrFromISR(chanNum, buf);
    }

/*
    Send a packet to the debug task - ISR version
*/

_CODE_ACCESS BaseType_t DebugSendPacketFromISR( debugQPacket_t packet ) {

    BaseType_t debugQStatus;

    debugQStatus = xQueueSendToBackFromISR( debugQ, packet, 0 );

    // ok if debugQStatus == pdPASS
    return debugQStatus;
    }

/*
    Debug task - interface to DSMUX serial debug tool

    This task is responsible for collecting and sending messages to
    DSMUX, and handing any chars coming from DSMUX to a handling
    routine.
*/

_CODE_ACCESS void DebugTask( void *pvParameters ) {

    /* process messages in the queue and send off to the serial monitor */

    debugQPacket_t packet;
    BaseType_t debugQStatus;
    // const TickType_t xTicksToWait = pdMS_TO_TICKS( 100 );

    while ( 1 ) {

        // ship out pending messages so queue is empty
        // then process debug commands

        // if message waiting, fetch the packet and send out its content

        if( uxQueueMessagesWaiting( debugQ ) > 0  ) {
            // don't block
            debugQStatus = xQueueReceive( debugQ, packet, 0 );
            if( debugQStatus == pdPASS ) {
                /*
                send packet off to serial port
                we can send 1 or 2 bytes depending on the mode
                we are using mode 2 - 32 channels
                walk through the items in packet, stopping when hit
                last or a 0 item
                */
                int i;
                uint16_t item;
                for (i = 0; i < debugPacketNumItems; i++) {
                    item = packet[i];
                    if ( item == 0 ) { break; }

                    // channel ID
                    sciSendByte(sciREG3, ( item >> 8 ) & 0xff );

                    // 7-bit data
                    sciSendByte(sciREG3, item & 0x7f );
                    }
                }
            }

        // if received a character, call the handler routine
        if ( sciIsRxReady(sciREG3) ) {
            uint32_t recByte = sciReceiveByte(sciREG3);
            DebugHandleCmd( recByte );
            }

        } // end while(1)

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
