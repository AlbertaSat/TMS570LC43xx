#include <Arduino.h>

/*
    DSMUX - Debugging 3 serial port multi channel tool

  This program turns an Arduino Due into a multichannel serial debug
  monitor device for any 3.3V processor board.

  The Tx and Rx serial ports (usually 1) on each device under test
  (DUT) board are used as a debug input/output channel. Output from the
  boards (up to 3) being debugged are sent to the Due UARTs Serial1,
  Serial2, and Serial3. On arrival they are interpreted, reformatted
  and then sent out over the Due usb port to a serial terminal
  emulator/monitor running on the main PC.

  Note the Due also has the usual Serial UART (pins 0, 1, but we leave this
  one alone.

  ● Output from the device under test

  The serial channels from the DUT have these possible formats:
    M1 1-byte mode: 2 sub-channels, 7 bit ascii
    M2 2-bute-mode: 32 sub-channels, 7 bit ascii

  The channel is 8-bit no-parity, but we use the parity bit to provide
  out-of-band signalling on the bit stream.

  ● Mode M1

  In 1-byte mode, the device under test (DUT) has two distinct 7-bit
  serial streams that it can send to the monitor. The channel is
  identified by the value of the parity bit: channel 0 and channel 1.
  The parity bit is set by the serial debug routines on the DUT.

  ● Mode M2

  In 2-byte mode, the device under test (DUT) sends 2 characters for
  every character of debug output.

    The first character is the channel ID with bits 1PPC CCCC. It has
    parity bit 1, PP are 00, and bits CCCCC indicate what channel 0-31
    is being multiplexed.

  The data arriving from a DUT is then packaged as a 16-bit packet internally,
  of the form
        1PPC CCCC 0DDD DDDD
  where PP is set to indicate the serial port that the data arrived on.  This
  way the data is processed uniformly regardless of it being over a 1 or 2-bit
  channel.

  NOTE - the packet ID is never 0 since we use ports 01, 10, 11

  Jumpers on the Due are used to set the mode of each Serial port.  Since
  the pullup resistors are turned on, and the normal use is mode M1, then
  a 1 on the jumper means mode M1, and a 0 on the jumper means mode M2.

  ● Input to the device under test

  To keep things simple, any non-command input by the user on the
  terminal monitor is sent to all DUT Rx inputs. Each DUT is then
  responsible for deciding how to process the command.

  ● Presentation format for the output from the devices under test

  Characters from the devices under test, from a particular stream,
  are associated with 8-bit ID's of the form:
    device stream
  where device is 2 bits (01, 10, 11), and stream is 6 bits.
  As each character arrives, it is tagged with it's ID.

  Then the question is how to present this information to the user in
  an intelligble way.

  ● Pin assignments on Due PCB

    Device 3
    6  Mode select 0 = M1, 1 = M2
    14 Serial3 Tx -> Rx on DUT
    15 Serial3 Rx <- Tx on DUT

    Device 2
    5  Mode select 0 = M1, 1 = M2
    16 Serial2 Tx -> Rx on DUT
    17 Serial2 Rx <- Tx on DUT

    Device 1
    4  Mode select 0 = M1, 1 = M2
    18 Serial1 Tx -> Rx on DUT
    19 Serial1 Rx <- Tx on DUT
*/

#define Serial1ModePin 4
#define Serial2ModePin 5
#define Serial3ModePin 6

class MultiChannelPort
{
  public:
    USARTClass* port;
    uint8_t portNum;
    uint8_t packetIDMask;

    // potocol mode 1 - M1, 2 - M2
    uint8_t mode;

    /* state variables used by 2-byte mode state machine */

    enum Mode2State { NoPacket, HaveID } packetState;
    uint8_t packetID;
    uint8_t packetByte;

    MultiChannelPort( USARTClass* port_arg, uint8_t portNum_arg ) {
      port = port_arg;
      portNum = portNum_arg & 0x3;
      mode = 2;
      // pre-compute the top bits of the packet id, parity bit 0
      packetIDMask = portNum << 5;

      // initial state of 2-byte machine
      packetState = NoPacket;
    }

    uint8_t getByte( uint8_t* inChar) {
      if ( mode == 2 ) {
        return getByteM2( inChar );
      }

      return getByteM1( inChar );
    }

    /* get unprocessed byte from the port, for debugging */

    uint8_t getByteRaw( uint8_t* inChar ) {

      if (! port->available()) { return 0; }
      *inChar = ( port->read() ) & 0xff;
      return 1;
    }

    /*
        Mode M1 - read in a 7-bit character from this channel

        returns the packet ID for the character, and modifies inChar to
        hold the character just read.

        If no data available, simply returns 0, and leaves inChar
        untouched.
    */

    uint8_t getByteM1( uint8_t* inChar ) {
      int inByte;
      uint8_t chanNum;
      uint8_t id = 0;

      if (! port->available()) {
        return id;
      }

      inByte = port->read();

      if ( inByte & 0x80 ) {
        /* channel 1 */
        chanNum = 1;
      }
      else {
        /* channel 0 */
        chanNum = 0;
      }

      *inChar = inByte & 0x7f;

      id = packetIDMask | chanNum;

      return id;
    }

    /*
        Mode M2 - read in a 2-byte packet character from this channel

        This is a little 2 state machine.  On first call it looks
        for the id byte, and returns 0.  On second call it fetches
        the data byte,

        returns the packet id for the character, and modifies inChar to
        hold the character just read.

        If no data available, simply returns 0, and leaves inChar
        untouched.

    */

    uint8_t getByteM2( uint8_t* inChar ) {
      int inByte;
      uint8_t rc = 0;

      if (! port->available()) {
        return rc;
      }

      inByte = port->read();

      if ( packetState == NoPacket ) {
        /* ID part of the packet has parity bit 1 */
        if ( inByte & 0x80 ) {
          /* add the channel to get the packet ID */
          packetID = packetIDMask | ( inByte & 0x1f );
          packetState = HaveID;
        }
        else {
          /* out of sync */
          SerialUSB.print(portNum);
          SerialUSB.print("- M2 ID sync err ");
          SerialUSB.println(inByte, HEX);
        }
      }
      else {
        /* data part of the packet has parity bit 0 */
        if ( ! (inByte & 0x80) ) {
          packetByte = inByte & 0x7f;
          packetState = NoPacket;
          *inChar = packetByte;
          rc = packetID;
        }
        else {
          /* out of sync */
          SerialUSB.print(portNum);
          SerialUSB.print("- M2 Data sync err ");
          SerialUSB.println(inByte, HEX);
        }
      }
      return rc;
    }

};

MultiChannelPort Port1(&Serial1, 1);
MultiChannelPort Port2(&Serial2, 2);
MultiChannelPort Port3(&Serial3, 3);

int firstPort;
int lastPort;
MultiChannelPort* ports[4];

/*  Run mode, set with ?n ?d
    n - normal mode
    d - debug mode, display the raw data coming from the DUT ports
*/
int runMode;

/*
    Message buffers to hold channel data while prepraring for output

    lightweight, indexed by packet ID - bufOffset
    buffer is always \0 terminated, len is the position of the \0

    OK, this should have been an object, but is an ADT instead.
*/

// all message buffers are the same size
#define msgBufSize 64

typedef struct msgBuf {
  // the port-chan id associated with this buffer
  uint8_t portNum;
  uint8_t chanNum;

  // current string length of buffer, that is, not counting the \0.
  // actual bytes of content is len+1
  int len;

  // time in mS since boot of last append, 0 means no append yet.
  unsigned long lastAppendTime;

  // buffer, \0 terminated
  char buf[msgBufSize];  // actual buffer
};

// all buffers are statically allocated
#define numPorts 3
#define numChans 32
#define numChanBufs (numPorts * numChans)
#define bufOffset 32

msgBuf chanBuf[numChanBufs];

/* index-checked access to message buffer pointer */

msgBuf* mbIdx(int idx) {
  if ( idx < 0 || idx >= numChanBufs ) {
    SerialUSB.print("FATAL: Msg buf index error ");
    SerialUSB.println(idx);
    while ( 1 ) { }
    }

  return &chanBuf[idx];
  }

/*  Reset the message buffer to empty, return num chars remaining */

int mbReset( msgBuf* mb ) {
  mb->len = 0;
  mb->lastAppendTime = 0;
  mb->buf[0] = '\0';
  return msgBufSize - mb->len - 1;
}

/*  Append a char to message buffer if not full, return num chars remaining */

int mbAppend( msgBuf* mb, char c ) {
  if ( mb->len < msgBufSize - 1 ) {
    mb->lastAppendTime = millis();
    mb->buf[mb->len++] = c;
    mb->buf[mb->len] = '\0';
    return msgBufSize - mb->len - 1;
  }
  // buffer full, n
  return 0;
}

/* check if buffer has timed out */

int mbTimedOut( msgBuf* mb ) {
  return mb->lastAppendTime + 1000 < millis();
}

/* return remaining space in buffer */

int mbRemain( msgBuf* mb ) {
  return msgBufSize - mb->len - 1;
}

/* 
  Messages terminate with an EOL.  Our notion of EOL
  covers all the various OS kinds: EOL can be \n, \r, \n\r, or \r\n
*/

/* true if the buffer ends in a end of line sequence */

int mbHasEOL( msgBuf* mb ) {
  uint8_t c;
  if ( mb->len <= 0 ) { return 0; }
  c = mb->buf[mb->len-1];
  return ( c == '\n' || c == '\r');
}

/* remove the end of line characters from the end of the buffer */

void mbRemoveEOL( msgBuf* mb ) {
  uint8_t c;
  if ( mbHasEOL( mb ) ) { mb->len--; }
  if ( mbHasEOL( mb ) ) { mb->len--; }
  mb->buf[mb->len] = '\0';
  return;
}

void setup() {
  int i;
  msgBuf* mb;

  // enable input on the mode selector, and turn on pullup resistor
  // so that open input is HIGH 1
  pinMode(Serial1ModePin, INPUT);
  digitalWrite(Serial1ModePin, HIGH);
  pinMode(Serial2ModePin, INPUT);
  digitalWrite(Serial2ModePin, HIGH);
  pinMode(Serial3ModePin, INPUT);
  digitalWrite(Serial3ModePin, HIGH);

  // we don't use port Serial
  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);
  SerialUSB.begin(9600);

  // enable indexing by port number
  firstPort = 1;
  lastPort = 3;
  ports[0] = 0;
  ports[1] = &Port1;
  ports[2] = &Port2;
  ports[3] = &Port3;

  // set default port mode from digital pins, default is mode 2
  if ( digitalRead( Serial1ModePin ) == LOW ) {
    Port1.mode = 1;
  }
  if ( digitalRead( Serial2ModePin ) == LOW ) {
    Port2.mode = 1;
  }
  if ( digitalRead( Serial3ModePin ) == LOW ) {
    Port3.mode = 1;
  }

  // initialize buffers for channel data
  for ( i = 0; i < numChanBufs; i++ ) {
    mb = mbIdx( i );
    mbReset( mb );
    // fabricate the id field
    mb->chanNum = i & 0x1f;
    mb->portNum = 1 + (i >> 5);
  }

  runMode = 'n';
}

/* ansi term color change escape sequences - don't work yet! */
int useAnsiColor = 0;

void ansiRed() {
  if ( ! useAnsiColor ) { return; }
  SerialUSB.print("\e[0;31m");
}

void ansiBoldRed() {
  if ( ! useAnsiColor ) { return; }
  SerialUSB.print("\e[1;31m");
}

void ansiReset() {
  if ( ! useAnsiColor ) { return; }
  SerialUSB.print("\e[0");
}
const char cmdChar  = '?';

void loop() {
  int inByte;
  int i;
  uint8_t id;
  uint8_t inChar;

  msgBuf* mb;
  int rc;

  /*  Input -> Devices Under Test */
  if (SerialUSB.available()) {
    inByte = SerialUSB.read();

    /* echo the character just typed, handle <CR> */
    if ( inByte == '\r' ) {
      SerialUSB.write('\n');
    }
    SerialUSB.write(inByte);

    /*  Commands to DSMUX are indicated by sending cmdChar followed
        by the actual command:
        ? - identify this program
        n - switch to normal output
        v - switch to verbose output
        d - switch to debug output

        a - put port 1 into mode 1
        A - put port 1 into mode 2
        b - put port 2 into mode 1
        B - put port 2 into mode 2
        c - put port 3 into mode 1
        C - put port 3 into mode 2
    */

    if ( inByte == cmdChar ) {
      while ( ! SerialUSB.available()) { }
      inByte = SerialUSB.read();
      SerialUSB.write(inByte);

      if ( inByte == '?' ) {
        SerialUSB.print("\n\r*This is DSMUX*\n\r");
        for ( i = firstPort; i <= lastPort; i++ ) {
          SerialUSB.print("Port ");
          SerialUSB.print(i);
          SerialUSB.print(" Mode ");
          SerialUSB.println(ports[i]->mode);
        }
        SerialUSB.println();
      }
      else if ( inByte == 'n' || inByte == 'd' || inByte == 'v' ) {
        runMode = inByte;
      } else {
        /* adjust the DUT port parameters
            a - put port 1 into mode 1
            A - put port 1 into mode 2
            b - put port 2 into mode 1
            B - put port 2 into mode 2
            c - put port 3 into mode 1
            C - put port 3 into mode 2

        channel filtering
            suppress port n
            suppress port n chan c

        */
        int pnum = 1;
        int pmode = 2;

        int action = 0;

        switch ( inByte ) {
          case 'a' :
            pmode = 1;
          case 'A' :
            pnum = 1;
            action = 1;
            Port1.mode = pmode;
            break;
          case 'b' :
            pmode = 1;
          case 'B' :
            pnum = 2;
            action = 1;
            Port2.mode = pmode;
            break;
          case 'c' :
            pmode = 1;
          case 'C' :
            pnum = 3;
            action = 1;
            Port3.mode = pmode;
            break;
          default:
            action = 0;
        }

        if ( action ) {
          SerialUSB.print("Set port ");
          SerialUSB.print(pnum);
          SerialUSB.print(" mode ");
          SerialUSB.println(pmode);
        }
      }
    }
    else {
      /* Pass anything else off to the devices under test */

      Serial1.write(inByte);
      Serial2.write(inByte);
      Serial3.write(inByte);
    }
  }  // end cmd processing

  /* Accumulate messages from the devices under test, for each channel,
      and output them in a user-friendly form
  */

  if ( runMode == 'n' || runMode == 'v' ) {

    // Update all the buffers, then decide what to print

    for ( i = firstPort; i <= lastPort; i++ ) {
      if ( id = ports[i]->getByte( &inChar ) ) {
        // make sure we don't go out of bounds
        if ( bufOffset <= id && id < 128 ) {
          mb = mbIdx( id - bufOffset );
          mbAppend( mb , inChar );
        } else {
          SerialUSB.print("Bad id ");
          SerialUSB.println(id, HEX);
        }
      }
    }

    for ( i = 0; i < numChanBufs; i++ ) {
      mb = mbIdx( i );

      // skip empty buffers, print those that are full, have EOL, or timed out

      if ( mb->len  && 
         ( ! mbRemain( mb ) || mbTimedOut( mb ) || mbHasEOL( mb ) ) ) 
      {

          // delete any end of line so we don't mess up printing
          mbRemoveEOL( mb );

          if ( runMode == 'v' ) {
            SerialUSB.print(i);
            SerialUSB.print(':');
            SerialUSB.print(mb->portNum, HEX);
            SerialUSB.print(':');
            SerialUSB.print(mb->chanNum, HEX);
            SerialUSB.print(':');
            SerialUSB.print(mb->buf);
            SerialUSB.print(':');
          }
          else {
            /* wrap output with color escapes */
            ansiRed();
            SerialUSB.print(mb->portNum);
            SerialUSB.print('-');
            ansiBoldRed();
            SerialUSB.print(mb->chanNum);
            SerialUSB.print(':');
            ansiReset();
            SerialUSB.print(mb->buf);
            SerialUSB.print(':');
          }

          SerialUSB.print("\n\r");
          mbReset( mb );
      }
    }
  }  // end runMode n, v

  /* In debug mode, just dump the full 8-bit byte stream from each port */
  if ( runMode == 'd' ) {
    for ( i = firstPort; i <= lastPort; i++ ) {
      if ( ports[i]->getByteRaw( &inChar ) ) {
        SerialUSB.print(i);
        SerialUSB.print(':');
        SerialUSB.print(inChar, HEX);
        SerialUSB.print(':');
        SerialUSB.write(inChar);
        SerialUSB.print(':');
        SerialUSB.println();
      }
    }
  } // end runMode d

} /* end loop */
