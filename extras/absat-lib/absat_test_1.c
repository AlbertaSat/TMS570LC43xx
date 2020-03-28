/*
    Test harness for long packets
*/

#include <stdio.h>

/* used in TI code base, not needed here */
#define _CODE_ACCESS

#include "absat_lib.c"

#define pktSize 8
typedef uint8_t debugQPacket_t [pktSize];

void set ( debugQPacket_t pkt ) 
{
    int i;
    for (i=0; i < pktSize; i++ ) {
        pkt[i] = i;
        }
    }

void get ( debugQPacket_t pkt ) 
{
    int i;
    for (i=0; i < pktSize; i++ ) {
        printf("[%d]=%d\n", i, pkt[i]);
        }
    }

int main( int argc, char *argv[] )
{
    debugQPacket_t pkt;
    int i;
    uint32_t s;
    uint32_t b;

    printf("Testing begins ...\n");

    set(pkt);
    get(pkt);

    printf("... testing ends.\n");
    }
