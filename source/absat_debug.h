#ifndef _ABSAT_DEBUG_H_
#define _ABSAT_DEBUG_H_
/*
    Alberta Sat multi-channel debug library
*/
#include "FreeRTOS.h"
#include "os_queue.h"
#include "os_task.h"

typedef uint16_t debugQPacket_t;
#define debugQSize 128
#define debugQPacketSize sizeof( debugQPacket_t )

_CODE_ACCESS void DebugHandleCmd( uint32_t inCmd );

_CODE_ACCESS void DebugTask( void *pvParameters );
_CODE_ACCESS QueueHandle_t DebugInit( void );

_CODE_ACCESS BaseType_t DebugSendPacket( debugQPacket_t packet );

_CODE_ACCESS void DebugSendStr( int chanNum, char *s );
_CODE_ACCESS void DebugSendBuf( int chanNum, char *buf, uint32_t bufSize );
#endif
