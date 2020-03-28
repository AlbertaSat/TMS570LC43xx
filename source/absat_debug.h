#ifndef _ABSAT_DEBUG_H_
#define _ABSAT_DEBUG_H_

#include "absat_lib.h"
/*
    Alberta Sat multi-channel debug library
*/
#include "FreeRTOS.h"
#include "os_queue.h"
#include "os_task.h"

/* 
    A debug packet is an array of debugPacketNumItems, each item
    being a uint16_t encoding the channel and character.

    To make addressing simple, 
        debugPacketNumItems = 2^n
        debugItemAddrMask = debugPacketNumItems - 1
    Then when we has a sequence of characters, the character in 
    position i is mapped to 
        packet number: i >> n
        item at position in the packet: i & debugItemAddrMask.
*/
#define debugPacketNumItems 8
#define debugItemAddrMask (debugPacketNumItems - 1)

typedef uint16_t debugQPacket_t[debugPacketNumItems];

#define debugQSize 128
#define debugQPacketSize sizeof( debugQPacket_t )

_CODE_ACCESS void DebugHandleCmd( uint32_t inCmd );

_CODE_ACCESS void DebugTask( void *pvParameters );
_CODE_ACCESS QueueHandle_t DebugInit( void );

_CODE_ACCESS BaseType_t DebugSendPacket( debugQPacket_t packet );
_CODE_ACCESS void DebugSendStr( int chanNum, char *s );
_CODE_ACCESS void DebugSendBuf( int chanNum, char *buf, uint32_t bufSize );
_CODE_ACCESS void DebugSendFmt( int chanNum, const char * __restrict format, ...
 );

// interrupt service routine safe versions
_CODE_ACCESS BaseType_t DebugSendPacketFromISR( debugQPacket_t packet );
_CODE_ACCESS void DebugSendStrFromISR( int chanNum, char *s );
_CODE_ACCESS void DebugSendBufFromISR( int chanNum, char *buf, uint32_t bufSize );
_CODE_ACCESS void DebugSendFmtFromISR( int chanNum, const char * __restrict format, ... );
#endif
