#include <Arduino.h>

/*
    DSMUX TEST driver for DSMUX Debugging 3 serial port multi channel tool

    It simulates 3 devices under test, running in modes:
        M1 1-byte mode: 2 sub-channels, 7 bit ascii
        M2 2-bute-mode: 32 sub-channels, 7 bit ascii

    See the serial multiplex specification in
        MultiChanSerialDue/MultiChanSerialDue.ino

    UI input and output is over SerialUSB.

    DUT simulated signals are sent and received on Serial1, Serial2, Serial3

  ● Pin assignments on Due PCB

    Device 3
    6  Mode select 0 = M1, 1 = M2
    14 Serial3 Tx -> Rx on DSMUX
    15 Serial3 Rx <- Tx on DSMUX

    Device 2
    5  Mode select 0 = M1, 1 = M2
    16 Serial2 Tx -> Rx on DSMUX
    17 Serial2 Rx <- Tx on DSMUX

    Device 1
    4  Mode select 0 = M1, 1 = M2
    18 Serial1 Tx -> Rx on DSMUX
    19 Serial1 Rx <- Tx on DSMUX
*/

#define Serial1ModePin 4
#define Serial2ModePin 5
#define Serial3ModePin 6

/*
    Get a byte being sent to given device under test by the DSMUX board.
    i.e. The DSMUX user is sending a command to the DUT

    By convention, these "bytes" are manipulated as int, so that out of
    band signals like -1 can be used to signal "no data".  Ugh, legacy.
*/
int readByteFromPort ( USARTClass* port ) {
  int inByte;
  if ( ! port || ! port->available() ) { return -1; }

  inByte = port->read();
  return inByte & 0xff;
}

/*
    Send a byte from the DUT to the corresponding port on the DSMUX board.
    Depending on the mode, 1 or 2 bytes will be simulated as coming from
    the DUT on a particuar channel.
*/
void writeByteToPort ( USARTClass* port, int outByte ) {
  if ( ! port ) { return; }
  port->write( outByte & 0xff );
}

/*
    DUT simulation state machine.

    Each machine is described by a port, channel, mode, and current
    state.  On each state transition it sends the appropriate bytes
    over the serial port on the specified channel.

    When a channel number in the range 2-31 is used in Mode 1, then only
    the last bit of the channel number is used to specify channel 0 or 1.

    Two kinds of messages can be generated by the machine.
    when msgLen == 0:
      infinite messages which are just a continuous cyclic sequence of
      characters form the symbols string.

    when msgLen > 0:
      continuous cyclic sequence of characters form the symbols string,
      terminated by a \n, of at most msgLen characters (including the \n).

    these fixed length messages have two additional parameters:

    useDelay - at the mid point of the message, the next transition of
        the state machine will be delayed by useDelay mS.

    useRandLen - the message can be terminated at any point with probability
        1/useRandLen

    DSMUX is supposed to buffer messages until the buffer is full, a \n
    is encountered, or it has been too long since a char was appended to
    a buffer.  The above options let us test all of these.
*/

char* symbols = "abcdefghijklmnopqrstuvwxyz";
const int numSymbols = 26;

class DUTMachine
{
  public:
    // actual serial port associated with portNum
    USARTClass* port;
    // port number 1-3, and channel number 0-1 Mode 1, or 0-31 Mode 2
    uint8_t portNum;
    uint8_t chanNum;
    // Mode 1 or Mode 2 protocol
    uint8_t mode;

    // pre-computed mask with channel number and parity bit.
    uint8_t packetIDMask;

    int msgLen;
    int useRandLen;
    int useDelay;

    /*
        Current state of machine
        For fixed length messages, curState is number of symbols in the
        message so far.
        For infinite messages, curState is just the current position in
        the symbols string.
    */
    int curState;

    // boot clock time of next state change when delay in effect
    unsigned long nextStateChangeTime;

    DUTMachine( USARTClass* port_arg, int portNum_arg, int chanNum_arg,
                int msgLen_arg, int useRandLen_arg, int useDelay_arg )
    {

      port = port_arg;
      portNum = portNum_arg & 0x3;
      chanNum = chanNum_arg & 0x1f;
      mode = 2;

      // no port components of a DUT packet in M2 mode
      packetIDMask = 0x80 | chanNum;

      msgLen = max(0, msgLen_arg);
      useRandLen = max(0, useRandLen_arg);
      useDelay = max(0, useDelay_arg);

      // initialize state machine
      curState = 0;
      nextStateChangeTime = 0;
    }

    void nextState() {
      /* make a transtion on the state machine */

      uint8_t transByte;
      unsigned long curTime;

      if ( msgLen == 0 ) {
        transByte = 0x7f & symbols[curState];
        curState = (curState + 1) % numSymbols;
      } else {
        // don't do a state transition if we are at the mid point
        if ( useDelay ) {
          if ( curState == msgLen / 2 ) {
            curTime = millis();
            if ( curTime < nextStateChangeTime ) {
              return;
            } else {
              // time to next mid-point delay
              nextStateChangeTime = curTime + useDelay;
            }
          }
        }

        // randomly terminate the message with P[1/useRandLen]

        if ( curState < msgLen && ! ( useRandLen && ! random(useRandLen) )) {
          // message not finished, so send the symbol
          transByte = 0x7f & symbols[ (curState % msgLen) % numSymbols];
          curState++;
        } else {
          // message finished, send the \n
          transByte = '\n';
          curState = 0;
        }
      }

      if ( mode == 2 ) {
        writeByteToPort( port, packetIDMask);
        writeByteToPort( port, transByte );
      }
      else {
        /* in Mode 1, just send a byte, mapping to chanNum % 2 */
        if ( chanNum & 1 ) {
          writeByteToPort( port, (transByte & 0x7f) | 0x80 );
        }
        else {
          writeByteToPort( port, (transByte & 0x7f) );
        }
      }
    }

};

int numMachines;
DUTMachine* machine[9];

/* set the protocol mode for all DUT attached to portNum */

void setPortMode( int portNum, int mode ) {
  int i;
  for ( i = 0; i < numMachines; i++ ) {
    if ( machine[i]->portNum == portNum ) {
      machine[i]->mode = mode;
    }
  }
    
  // Warn if we have now aliased channels in mode 1
  if ( mode == 1 ) {
    SerialUSB.print("NOTE Port ");
    SerialUSB.println(portNum);
    
    SerialUSB.print("Chan 0 aliases:"); 
    for ( i = 0; i < numMachines; i++ ) {
      if ( machine[i]->portNum == portNum && ! (machine[i]->chanNum & 0x1)) {
        SerialUSB.print(" ");
        SerialUSB.print( machine[i]->chanNum );       
      }
    }
    SerialUSB.println();

    SerialUSB.print("Chan 1 aliases:"); 
    for ( i = 0; i < numMachines; i++ ) {
      if ( machine[i]->portNum == portNum && (machine[i]->chanNum & 0x1)) {
        SerialUSB.print(" ");
        SerialUSB.print( machine[i]->chanNum );       
      }
    }
    SerialUSB.println();
  }
}

int firstPort;
int lastPort;
USARTClass* ports[4];


void setup() {
  int i;

  // enable input on the mode selector, and turn on pullup resistor
  pinMode(Serial1ModePin, INPUT);
  digitalWrite(Serial1ModePin, HIGH);
  pinMode(Serial2ModePin, INPUT);
  digitalWrite(Serial2ModePin, HIGH);
  pinMode(Serial3ModePin, INPUT);
  digitalWrite(Serial3ModePin, HIGH);

  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);
  SerialUSB.begin(9600);

  firstPort = 1;
  lastPort = 3;
  // ports[0] = &Serial;  // Serial is UARTClass not a USARTClass
  ports[0] = 0;
  ports[1] = &Serial1;
  ports[2] = &Serial2;
  ports[3] = &Serial3;

  // Set up the various state machines
  // port, portNum, chanNum, msgLen, userandom, useDelay
  i = 0;

  machine[i++] = new DUTMachine( &Serial1, 1, 0, 4, 0, 0);
  machine[i++] = new DUTMachine( &Serial1, 1, 6, 0, 0, 0);
  machine[i++] = new DUTMachine( &Serial1, 1, 31, 9, 3, 3000);

  // Port 2 interleaves channel 31
  machine[i++] = new DUTMachine( &Serial2, 2, 0, 10, 2, 0);
  machine[i++] = new DUTMachine( &Serial2, 2, 7, 5, 0, 0);
  machine[i++] = new DUTMachine( &Serial2, 2, 31, 23, 10, 0);
  machine[i++] = new DUTMachine( &Serial3, 2, 31, 3, 0, 0);

  // Port 3 will test mode 1, so only 2 channels
  machine[i++] = new DUTMachine( &Serial3, 3, 0, 27, 0, 5000);
  machine[i++] = new DUTMachine( &Serial3, 3, 1, 0, 0, 0);
  

  numMachines = i;

  /* set the boot up port mode, default is mode 2 */
  if ( digitalRead( Serial1ModePin ) == LOW ) {
    SerialUSB.println("Port 1 Mode 1");
    setPortMode( 1, 1 );
  }
  if ( digitalRead( Serial2ModePin ) == LOW ) {
    SerialUSB.println("Port 2 Mode 1");
    setPortMode( 2, 1 );
  }
  if ( digitalRead( Serial3ModePin ) == LOW ) {
    SerialUSB.println("Port 3 Mode 1");
    setPortMode( 3, 1 );
  }
}

const char cmdChar  = '?';

void loop() {
  int inByte;
  uint8_t id;
  uint8_t inChar;
  int i;

  /* the testing board accepts characters and sends them off to the
    debugger as if they were generated by the DUT */

  /*  Input -> Commands from the usb port control the test generation */
  if (SerialUSB.available()) {
    inByte = SerialUSB.read();

    /* echo the character just typed, handle <CR> */
    if ( inByte == '\r' ) {
      SerialUSB.write('\n');
    }
    SerialUSB.write(inByte);

    if ( inByte == cmdChar ) {
      while ( ! SerialUSB.available()) { }
      inByte = SerialUSB.read();
      SerialUSB.write(inByte);

      if ( inByte == '?' ) {
        SerialUSB.print("\n\r*This is DSMUX TESTOR*\n\r");

        for ( i = 0; i < numMachines; i++ ) {
          DUTMachine* m = machine[i];
          SerialUSB.print("DUT ");
          SerialUSB.print(i);
          SerialUSB.print(" Port ");
          SerialUSB.print(m->portNum);
          SerialUSB.print(" Chan ");
          SerialUSB.print(m->chanNum);
          SerialUSB.print(" Mode ");
          SerialUSB.print(m->mode);
          SerialUSB.print(" Mask ");
          SerialUSB.print(m->packetIDMask, HEX);
          SerialUSB.print(" Cycl ");
          SerialUSB.print(m->msgLen);
          SerialUSB.print(" Rand ");
          SerialUSB.print(m->useRandLen);
          SerialUSB.print(" Dly ");
          SerialUSB.print(m->useDelay);
          SerialUSB.print(" State ");
          SerialUSB.print(m->curState);
          SerialUSB.print(" DTime ");
          SerialUSB.print(m->nextStateChangeTime);
          SerialUSB.println();
        }
        SerialUSB.println("\n\r\n\r");
      }  // end ? cmd
    else {
      /* adjust the DUT simulation parameters
          a - put port 1 into mode 1
          A - put port 1 into mode 2
          b - put port 2 into mode 1
          B - put port 2 into mode 2
          c - put port 3 into mode 1
          C - put port 3 into mode 2
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
          break;
        case 'b' :
          pmode = 1;
        case 'B' :
          pnum = 2;
          action = 1;
          break;
        case 'c' :
          pmode = 1;
        case 'C' :
          pnum = 3;
          action = 1;
          break;
        default:
          action = 0;
      }

      if ( action ) {
        SerialUSB.print("Set port ");
        SerialUSB.print(pnum);
        SerialUSB.print(" mode ");
        SerialUSB.println(pmode);
        setPortMode( pnum, pmode );
      }
    }
  }
  }

  /* make each DUT state machine do one transition, sending its result */
  for ( i = 0; i < numMachines; i++ ) {
    machine[i]->nextState();
  }

  /* Echo any commands from DSMUX being sent to the DUT ports */
  for ( i = firstPort; i <= lastPort; i++ ) {
    if ( (inByte = readByteFromPort( ports[i] )) >= 0 ) {
      SerialUSB.print(i);
      SerialUSB.print(':');
      SerialUSB.print(inByte, HEX);
      SerialUSB.print('<');
      SerialUSB.write(inByte);
      SerialUSB.print(">\n\r");
    }
  }

  delay(100);
  
}
