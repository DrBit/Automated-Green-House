/*
SilverCrest 91210 RCS AAA3680 - A IP20
Below code will transmit on and off signals to SilverCrest 91210 Remote Electrical Sockets

Based on code from http://forum.arduino.cc/index.php?topic=202556.msg1492685#msg1492685
Modifed by Poopi, Tested on Teensy 2.0++

Sequence had two distinct sub sequences - 
1. 4x with short sync and short '1's 
2. 4x with long sync and longer '1's

Codes were cycled - 4 different codes per each plug but you can choose any of these codes - I used all to mimic original RCU
Seems that 4 last bits are a PLUG identifier and first 4 bits are a remote identifier

Remaining 16 bits seems to be common for all plugs ( Plug C have reverted On/Off sequences sic! )
All codes captured from original RCU usin logic analyzer


Below setup as two Button Pins (each button pin 1 is sent to arduino pins 5 and 8 and pin 4 sent to ground)
Button 1 sends the signal
Button 2 cycles through the functions (A, B, C, D and Master)
433Mhz Transmitter set up on pins 7 and 24
Pin 27 - transmit
Pin 24 - VCC of the transmitter

LED on pin 6 flashes while sending signal
*/

#define DEBUG

/*Setup*/
#define BUTTON_PIN1    26
#define BUTTON_PIN2    25

#define RF_DATA_PIN    7
#define RF_ENABLE_PIN  24
#define BLINK_LED_PIN  6
 
#define MAX_CODE_CYCLES 4

#define SHORT_DELAY       380
#define NORMAL_DELAY      500
#define SIGNAL_DELAY     1500

#define SYNC_DELAY       2650
#define EXTRASHORT_DELAY 3000
#define EXTRA_DELAY     10000

enum {
  PLUG_A = 0,
  PLUG_B,
  PLUG_C,
  PLUG_D,
  PLUG_MASTER,
  PLUG_LAST
};

unsigned long signals[PLUG_LAST][2][MAX_CODE_CYCLES] = {
  { /*A*/
    {0b101111000001000101011100, 0b101100010110110110101100, 0b101110101110010001101100, 0b101101000101010100011100}, 
    {0b101101010010011101111100, 0b101111100011110000101100, 0b101111110111001110001100, 0b101110111000101110111100}
  },  
  { /*B*/
    {0b101101110100001000110101, 0b101101101010100111100101, 0b101110011101111000000101, 0b101100100000100011110101}, 
    {0b101111011001101011010101, 0b101100111011111101000101, 0b101110001100011010010101, 0b101100001111000011000101}
  },  
  { /*C*/
    {0b101101010010011101111110, 0b101111100011110000101110, 0b101111110111001110001110, 0b101110111000101110111110},
    {0b101110101110010001101110, 0b101101000101010100011110, 0b101111000001000101011110, 0b101100010110110110101110} 
  },  
  { /*D*/
    {0b101111011001101011010111, 0b101100111011111101000111, 0b101100001111000011000111, 0b101110001100011010010111}, 
    {0b101101110100001000110111, 0b101100100000100011110111, 0b101101101010100111100111, 0b101110011101111000000111}
  },  
  { /*MASTER*/
    {0b101111100011110000100010, 0b101110111000101110110010, 0b101101010010011101110010, 0b101111110111001110000010}, 
    {0b101111000001000101010010, 0b101101000101010100010010, 0b101110101110010001100010, 0b101100010110110110100010}
  },  
};

boolean       onOff;
unsigned char plug;
unsigned char swap;
 
void setup() {

  pinMode(RF_DATA_PIN, OUTPUT);
  pinMode(RF_ENABLE_PIN, OUTPUT);
  digitalWrite(RF_ENABLE_PIN,LOW);

  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  Serial.println("Ready");
  swap  = 0;
  plug  = PLUG_C;
}
 
void loop() {

    plug++;
    plug%=PLUG_LAST;
    for (byte a=0; a<=1; a++) {
      onOff = a;
      digitalWrite(BLINK_LED_PIN,HIGH);
      ActivatePlug(plug,onOff);
      digitalWrite(BLINK_LED_PIN,LOW);
      #ifdef DEBUG
      Serial.print("PLUG_");
      Serial.print(plug == PLUG_MASTER?'M':(char)('A'+plug));
      Serial.print(" ");
      Serial.println(onOff?"Off":"On");
      //Serial.println(signal, BIN);
      #endif
      delay(1000);
    }
    Serial.println("===========");
    delay(500);
}


void sendSync(){
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(SHORT_DELAY);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(SYNC_DELAY - SHORT_DELAY);  
}

void sendValue(boolean value, unsigned int base_delay){
  unsigned long d = value? SIGNAL_DELAY - base_delay : base_delay;
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(d);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(SIGNAL_DELAY - d);
}

void longSync(){
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(EXTRASHORT_DELAY);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(EXTRA_DELAY - EXTRASHORT_DELAY);  
}

void ActivatePlug(unsigned char PlugNo, boolean On) {
  if( PlugNo < PLUG_LAST ) {

    digitalWrite(RF_DATA_PIN,LOW);
    digitalWrite(RF_ENABLE_PIN,HIGH);
    delayMicroseconds(1000);

    unsigned long signal = signals[PlugNo][On][swap];
    
    swap++;
    swap%=MAX_CODE_CYCLES;
 
    for (unsigned char i=0; i<4; i++) { // repeat 1st signal sequence 4 times
      sendSync();
      for (unsigned char k=0; k<24; k++) { //as 24 long and short signals, this loop sends each one and if it is long, it takes it away from total delay so that there is a short between it and the next signal and viceversa
        sendValue(bitRead(signal, 23-k),SHORT_DELAY);
      }    
    }
    for (unsigned char i=0; i<4; i++) { // repeat 2nd signal sequence 4 times with NORMAL DELAY
      longSync();  
      for (unsigned char k=0; k<24; k++) {
        sendValue(bitRead(signal, 23-k),NORMAL_DELAY);
      }
    }
    digitalWrite(RF_ENABLE_PIN,LOW);
  }
}

/*

A on codes:
101100010110110110101100  
101110101110010001101100
101111000001000101011100
101101000101010100011100

A off codes:
101101010010011101111100
101111100011110000101100  
101111110111001110001100  
101110111000101110111100  

B on codes:
101101110100001000110101  
101101101010100111100101  
101110011101111000000101  
101100100000100011110101

B off codes:
101111011001101011010101
101100111011111101000101
101110001100011010010101
101100001111000011000101

C on codes:
101101010010011101111110
101111100011110000101110
101111110111001110001110
101110111000101110111110

C off codes:
101110101110010001101110
101101000101010100011110
101111000001000101011110
101100010110110110101110

D on codes:
101111011001101011010111
101100111011111101000111
101100001111000011000111
101110001100011010010111

D off codes:
101101110100001000110111
101100100000100011110111
101101101010100111100111
101110011101111000000111

MASTER on codes:
101111100011110000100010
101110111000101110110010
101101010010011101110010
101111110111001110000010

MASTER off codes:
101111000001000101010010
101101000101010100010010
101110101110010001100010
101100010110110110100010

*/