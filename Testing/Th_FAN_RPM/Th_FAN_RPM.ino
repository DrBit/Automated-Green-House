// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    
 
int samples[NUMSAMPLES];
const int TH_reading_time = 2000;  // Time between RPM readings (in ms)
unsigned long lastmillisTH = 0;
 
 // read RPM
/* Using technic descrived in http://pcbheaven.com/circuitpages/PWM_3_Wires_Fan_Controller_with_RPM_feedback/  */
/* Alternative metoth of counting time - > http://www.avdweb.nl/arduino/hardware-interfacing/frequency-period-counter.html */

const int RPM_FAN_pin = 8; // input pin fixed to internal Timer (It will efectively change precission)
const int prescale = 64; // prescale factor (each tick 0.5 us @16MHz)
const byte prescaleBits = B011; // see Table 18-1 or data sheet
const int rpm_reading_time = 3000;  // Time between RPM readings (in ms)
// calculate time per counter tick in ns
const long precision = (1000000/(F_CPU/1000)) * prescale ;
volatile unsigned int rpmTiks; // note this is 16 bit value
volatile boolean read_rpm_enabled = false;
volatile boolean rpm_ready = false;


unsigned long lastmillisRPM = 0;
unsigned long lastmillisLedFade = 0;
int FAN_PulsesXRotation = 2;   // Number of pulses per rotation
int angleCount = 0;
int _lastPWM = 255;   // Record the last PWM value on the ventilater to apply after HALL snesor has been readed
const int fanPin = 3;  // Fan pin


void setup(void) {
  Serial.begin(9600);

  // Configure Timer1 for counting ticks on falling edge interruption at pin8
  TCCR1A = 0 ; // Normal counting mode
  TCCR1B = prescaleBits ; // set prescale bits
  TCCR1B |= _BV(ICES1); // enable input capture
  bitSet(TIMSK1,ICIE1); // enable input capture interrupt for timer 1
  
  pinMode(fanPin, OUTPUT);          // PWM pin for fan control
  pinMode(RPM_FAN_pin, INPUT);  // ICP pin (digital pin 8 on Arduino) as input
  TCCR2B = TCCR2B & B11111000 | B00000001;    // Convert PWM in pins 3 and 11 into high frequency To avoid noise (Timer 2)
  Serial.println("Precision of RPM is");
  Serial.print( precision); // report duration of each tick in microseconds
  Serial.println(" microseconds increments");

  //analogReference(EXTERNAL);    // If using 3.3 volts and connected 3.3volts to the external reference
}
 
void loop(void) {

  //FAN_speed_test ();
  analogWrite(fanPin, 255);
  if (!readRPM ()) Serial.println("Error reading RPM");
  readThermistor ();

}


void readThermistor () {
  if (millis() - lastmillisTH >= TH_reading_time){
    uint8_t i;
    float average;
   
    // take N samples in a row, with a slight delay
    for (i=0; i< NUMSAMPLES; i++) {
     samples[i] = analogRead(THERMISTORPIN);
     delay(10);
    }
   
    // average all the samples out
    average = 0;
    for (i=0; i< NUMSAMPLES; i++) {
       average += samples[i];
    }
    average /= NUMSAMPLES;
   
    //Serial.print("Average analog reading "); 
    //Serial.println(average);
   
    // convert the value to resistance
    average = 1023 / average - 1;
    average = SERIESRESISTOR / average;
    //Serial.print("Thermistor resistance "); 
    //Serial.println(average);
   
    float steinhart;
    steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    steinhart -= 273.15;                         // convert to C
   
    Serial.print("Temperature "); 
    Serial.print(steinhart);
    Serial.println(" *C");

    lastmillisTH = millis();
  }
 
  //delay(1000);
}



boolean readRPM () {
  if (millis() - lastmillisRPM >= rpm_reading_time){
    analogWrite(fanPin, 255);
    delay (100);    // To allow the power to stabilize, otherwise we get bad readings
    read_rpm_enabled = true;    // Enables counter
    while (!rpm_ready) {
        if (millis() - lastmillisRPM >= (rpm_reading_time + 200)){
          clear_values ();
          return false;
        }
    }   // Wait until we read our value
    analogWrite(fanPin, _lastPWM);                               // Return FAN to its original speed
    print_rpm ();
    clear_values ();
    return true;
  }
}

void clear_values () {
  rpmTiks = 0;    // reset value
  rpm_ready = false;
  lastmillisRPM = millis(); // Uptade lasmillis
}

/* ICR interrupt vector */
ISR(TIMER1_CAPT_vect)
{
  TCNT1 = 0; // reset the counter
  if (read_rpm_enabled && bitRead(TCCR1B ,ICES1) == true) {
    rpmTiks = ICR1; // save the input capture value
    rpm_ready = true;
    read_rpm_enabled = false;
    //TCCR1B ^= _BV(ICES1); // toggle bit to trigger on the other edge (It will count pulse amplitude if enabled, otherwise it counts period)
  }
}


void print_rpm () {
    //calculate rpm
    long duration = (rpmTiks * precision)/1000; // pulse duration in nanoseconds of the pulse period (not a real revolution)
    long period = (duration*FAN_PulsesXRotation)/1000;
    long frequency = 1000000/(duration*FAN_PulsesXRotation);
    long rpm = 60000000/(duration*FAN_PulsesXRotation);

    // Print
    Serial.print("RPM =\t"); //print the word "RPM" and tab.
    Serial.print(rpm); // print the rpm value.
    Serial.print("\t Hz =\t"); //print the word "RPM" and tab.
    Serial.print(frequency); // print the rpm value.
    Serial.print("\t Period (ms) =\t"); //print the word "Hz".
    Serial.println(period); //print revolutions per second or Hz. And print new line or enter.
}

void FAN_speed_test() { 
  // Every 15ms do
  if (millis() - lastmillisLedFade >= 215){ 
  
    angleCount = angleCount == 360 ? 0 : angleCount+1;
    /*this part equals:
    if (angleCount == 360) {
      angleCount =0;
    }else{
      angleCount++;
    }*/

    //convert 0-360 angle to radian (needed for sin function)
    float rad = DEG_TO_RAD * angleCount;
    //calculate sin of angle as number between 0 and 255
    _lastPWM = constrain((sin(rad) * 128) + 128, 0, 255); 
    analogWrite(fanPin, _lastPWM);
    lastmillisLedFade = millis(); // Uptade lasmillis
  }
}

/*

If you want more information about configuring timers and the different modes then check these tutorials out. 
Ken Shirriff's secrets of Arduino PWMs: http://www.righto.com/2009/07/secrets-of-arduino-pwm.html
RobotFreaks's Arduino 101: Timers and interrupts: http://letsmakerobots.com/node/28278


Note: Divide the PWM frequency by 2 for an 8MHz clock, multiply by 1.25 for a 20MHz clock.

For Arduino Uno, Nano, Micro Magician, Mini Driver, Lilly Pad and any other board using ATmega 8, 168 or 328

//---------------------------------------------- Set PWM frequency for D5 & D6 -------------------------------
  
//TCCR0B = TCCR0B & B11111000 | B00000001;    // set timer 0 divisor to     1 for PWM frequency of 62500.00 Hz
//TCCR0B = TCCR0B & B11111000 | B00000010;    // set timer 0 divisor to     8 for PWM frequency of  7812.50 Hz
  TCCR0B = TCCR0B & B11111000 | B00000011;    // set timer 0 divisor to    64 for PWM frequency of   976.56 Hz
//TCCR0B = TCCR0B & B11111000 | B00000100;    // set timer 0 divisor to   256 for PWM frequency of   244.14 Hz
//TCCR0B = TCCR0B & B11111000 | B00000101;    // set timer 0 divisor to  1024 for PWM frequency of    61.04 Hz


//---------------------------------------------- Set PWM frequency for D9 & D10 ------------------------------
  
//TCCR1B = TCCR1B & B11111000 | B00000001;    // set timer 1 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR1B = TCCR1B & B11111000 | B00000010;    // set timer 1 divisor to     8 for PWM frequency of  3921.16 Hz
  TCCR1B = TCCR1B & B11111000 | B00000011;    // set timer 1 divisor to    64 for PWM frequency of   490.20 Hz
//TCCR1B = TCCR1B & B11111000 | B00000100;    // set timer 1 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR1B = TCCR1B & B11111000 | B00000101;    // set timer 1 divisor to  1024 for PWM frequency of    30.64 Hz

//---------------------------------------------- Set PWM frequency for D3 & D11 ------------------------------
  
//TCCR2B = TCCR2B & B11111000 | B00000001;    // set timer 2 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR2B = TCCR2B & B11111000 | B00000010;    // set timer 2 divisor to     8 for PWM frequency of  3921.16 Hz
//TCCR2B = TCCR2B & B11111000 | B00000011;    // set timer 2 divisor to    32 for PWM frequency of   980.39 Hz
  TCCR2B = TCCR2B & B11111000 | B00000100;    // set timer 2 divisor to    64 for PWM frequency of   490.20 Hz
//TCCR2B = TCCR2B & B11111000 | B00000101;    // set timer 2 divisor to   128 for PWM frequency of   245.10 Hz
//TCCR2B = TCCR2B & B11111000 | B00000110;    // set timer 2 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR2B = TCCR2B & B11111000 | B00000111;    // set timer 2 divisor to  1024 for PWM frequency of    30.64 Hz



For Arduino Mega1280, Mega2560, MegaADK, Spider or any other board using ATmega1280 or ATmega2560

//---------------------------------------------- Set PWM frequency for D4 & D13 ------------------------------
  
//TCCR0B = TCCR0B & B11111000 | B00000001;    // set timer 0 divisor to     1 for PWM frequency of 62500.00 Hz
//TCCR0B = TCCR0B & B11111000 | B00000010;    // set timer 0 divisor to     8 for PWM frequency of  7812.50 Hz
  TCCR0B = TCCR0B & B11111000 | B00000011;    // set timer 0 divisor to    64 for PWM frequency of   976.56 Hz
//TCCR0B = TCCR0B & B11111000 | B00000100;    // set timer 0 divisor to   256 for PWM frequency of   244.14 Hz
//TCCR0B = TCCR0B & B11111000 | B00000101;    // set timer 0 divisor to  1024 for PWM frequency of    61.04 Hz


//---------------------------------------------- Set PWM frequency for D11 & D12 -----------------------------
  
//TCCR1B = TCCR1B & B11111000 | B00000001;    // set timer 1 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR1B = TCCR1B & B11111000 | B00000010;    // set timer 1 divisor to     8 for PWM frequency of  3921.16 Hz
  TCCR1B = TCCR1B & B11111000 | B00000011;    // set timer 1 divisor to    64 for PWM frequency of   490.20 Hz
//TCCR1B = TCCR1B & B11111000 | B00000100;    // set timer 1 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR1B = TCCR1B & B11111000 | B00000101;    // set timer 1 divisor to  1024 for PWM frequency of    30.64 Hz

//---------------------------------------------- Set PWM frequency for D9 & D10 ------------------------------
  
//TCCR2B = TCCR2B & B11111000 | B00000001;    // set timer 2 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR2B = TCCR2B & B11111000 | B00000010;    // set timer 2 divisor to     8 for PWM frequency of  3921.16 Hz
//TCCR2B = TCCR2B & B11111000 | B00000011;    // set timer 2 divisor to    32 for PWM frequency of   980.39 Hz
  TCCR2B = TCCR2B & B11111000 | B00000100;    // set timer 2 divisor to    64 for PWM frequency of   490.20 Hz
//TCCR2B = TCCR2B & B11111000 | B00000101;    // set timer 2 divisor to   128 for PWM frequency of   245.10 Hz
//TCCR2B = TCCR2B & B11111000 | B00000110;    // set timer 2 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR2B = TCCR2B & B11111000 | B00000111;    // set timer 2 divisor to  1024 for PWM frequency of    30.64 Hz


//---------------------------------------------- Set PWM frequency for D2, D3 & D5 ---------------------------
  
//TCCR3B = TCCR3B & B11111000 | B00000001;    // set timer 3 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR3B = TCCR3B & B11111000 | B00000010;    // set timer 3 divisor to     8 for PWM frequency of  3921.16 Hz
  TCCR3B = TCCR3B & B11111000 | B00000011;    // set timer 3 divisor to    64 for PWM frequency of   490.20 Hz
//TCCR3B = TCCR3B & B11111000 | B00000100;    // set timer 3 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR3B = TCCR3B & B11111000 | B00000101;    // set timer 3 divisor to  1024 for PWM frequency of    30.64 Hz

  
//---------------------------------------------- Set PWM frequency for D6, D7 & D8 ---------------------------
  
//TCCR4B = TCCR4B & B11111000 | B00000001;    // set timer 4 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR4B = TCCR4B & B11111000 | B00000010;    // set timer 4 divisor to     8 for PWM frequency of  3921.16 Hz
  TCCR4B = TCCR4B & B11111000 | B00000011;    // set timer 4 divisor to    64 for PWM frequency of   490.20 Hz
//TCCR4B = TCCR4B & B11111000 | B00000100;    // set timer 4 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR4B = TCCR4B & B11111000 | B00000101;    // set timer 4 divisor to  1024 for PWM frequency of    30.64 Hz


//---------------------------------------------- Set PWM frequency for D44, D45 & D46 ------------------------
  
//TCCR5B = TCCR5B & B11111000 | B00000001;    // set timer 5 divisor to     1 for PWM frequency of 31372.55 Hz
//TCCR5B = TCCR5B & B11111000 | B00000010;    // set timer 5 divisor to     8 for PWM frequency of  3921.16 Hz
  TCCR5B = TCCR5B & B11111000 | B00000011;    // set timer 5 divisor to    64 for PWM frequency of   490.20 Hz
//TCCR5B = TCCR5B & B11111000 | B00000100;    // set timer 5 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR5B = TCCR5B & B11111000 | B00000101;    // set timer 5 divisor to  1024 for PWM frequency of    30.64 Hz

*/