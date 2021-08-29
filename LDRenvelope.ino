// Requires headers for AVR defines and ISR function
#include <avr/io.h>
#include <avr/interrupt.h>


volatile int inputPin = 2;        //pin 7, INT0, recieves trigger to activate the block
int envDial = A3;                 //pin 2, for mode selection
int speedDial = A2;               //pin 3, to set speed of envelope
int LED = 0;                      //pin 5, "output" to control the amplifier/attenuator

int mode = 0;                     //mode selection

int envSel[ 3 ][ 5 ] = {  {  160, 500, 160,  0,  1  },      //parameters for first envelope;  1. attack time, 2. hold time, 3. release time, 4. attack/release shape
                          {   1,   1,  320,  0,  0  },      //parameters for second envelope; 1. attack time, 2. hold time, 3. release time, 4. attack/release shape
                          {  160, 500, 320,  1,  1  }   };  //parameters for third envelope;  1. attack time, 2. hold time, 3. release time, 4. attack/release shape
                                                            //first 3 values are in ms, second 2 values are shape for that portion of the wave - 1 is linear and 2 is an exponential curve
                                                            //5th parameter is for separate attack and release shapes, not yet implemented

int tremLevel = 150;
int fadeAmount = 20;

volatile bool envelope = false;


int brightness = 0;
volatile int brightnessTarget = 0;

//brightness values stored in a 2D array for ease of access. brightness of LED controls analogue circuitry through a light-dependent resistor,
//so different responses can be controlled by simply using code to change the brightness of that LED in different patterns
byte ledLookupTable[2][17] = {  { 0,  1,  2,  3,  4,  6,  8,  12,  16,  23,  32,  45,  64,  90, 128, 180, 255  },         //creates a linear attack/release curve
                                { 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255  }    };    //creates an exponential attack/release curve

void setup()
{
  cli();                              //disable interrupts during setup

  MCUCR |= B00000011;                 //watch for rising edge, B00000010 is falling edge
  GIMSK |= B01000000;                 //enable external interrupt on pin 7
  SREG  |= B10000000;                 //global interrupt enable
  
  pinMode(inputPin, INPUT);           //interrupt trigger pin
  pinMode(LED, OUTPUT);               //used to trigger the next block
  
  sei();                              //last line of setup - enable interrupts after setup
}

ISR(INT0_vect)
{  
  brightnessTarget = 16;
}

void envelopeCreator(int env)
{    
  if(brightness <= brightnessTarget)   //attack
  {
    if(brightness < 16)
      brightness++;
    for(int i=0; i<(envSel[env][0] / 16); i++)    //for this amount of time determined by the envelope array
      delay(1);                                   //should ideally be done with internal interrupt
  }
  
  if(brightness == 16)                //hold
  {
    for(int i=0; i<(envSel[env][1]); i++)        //for this amount of time determined by the envelope array
      delay(1);
    brightnessTarget = 0;
  }

  if(brightness > brightnessTarget)   //release
  {
    if(brightness > 0)
      brightness--;
    for(int i=0; i<(envSel[env][2] / 16); i++)   //for this amount of time determined by the envelope array
      delay(1);
  }
  analogWrite(LED, ledLookupTable[envSel[env][3]][brightness]);
  
}

void loop()
{
  mode = map(analogRead(envDial),0,1023,0,3);

  if(mode !=0)    //switch through custom envelope modes by turning dial
  {
    envelopeCreator(mode-1);
  }
  else            //higher portion of dial is reserved for tremolo mode, the same dial controls frequency of tremolo effect
  {
    tremolo();
  }
}

void tremolo()
{
  int freq = map(analogRead(envDial), 0, 512, 1, 10);   //scale the end portion of the potentiometer to double as a frequency tuner, save on hardware
  analogWrite(LED, tremLevel);

  tremLevel += fadeAmount;

  // reverse the direction of the fading at the ends of the fade:
  if (tremLevel <= 0 || tremLevel >= 255)
  {
    fadeAmount = -fadeAmount;
  }
  delay(freq);
}
