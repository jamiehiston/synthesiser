const int output = 1;     //pin6 - can be PB1 or PB4, PB4 works better for Tone because we can use hardware output compare
const int trigOut = 0;    //pin5
int noteIn = A3;          //pin2
int octaveIn = A2;        //pin3
int linearEnable = 2;     //pin7
bool linearScale = false; //switch between linear and exponential for the voltage input - useful for resistor ladder input
int n = 0;
int o = 0;
int frequency = 0;
int noteRead = 0;
int octaveRead = 0;

int noteInRead = 0;

int prevNoteRead = 0;
int prevOctaveRead = 0;
int prevN = 0;
int prevO = 0;

int newN = 0;
int newO = 0;

bool updateSound = true;

#define arrayLength 3                  //NB: a side effect of increasing this value too much (to around 5-10) is a portamento effect, which may be desireable. it definitely gives a unique sound to just the raw keyboard input!
int movingAverageArray[arrayLength];
int MAFCounter = 0;
int sum = 0;
int avg = 0;
bool startCounter = false;

//2D arrays to store exact frequency values for musical notes; X axis is for notes and Y axis is for octaves. first value in sequence must be zero to allow for an off state where no note is playing
int noteArray[ 8 ][ 13 ] = {    {  0,   16,    17,     18,     19,     21,     22,     23,     25,     26,     28,     29,     31    },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 0 
                                {  0,   33,    35,     37,     39,     41,     44,     46,     49,     52,     55,     58,     62    },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 1
                                {  0,   65,    69,     73,     78,     82,     87,     93,     98,     104,    110,    117,    123   },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 2
                                {  0,   131,   139,    147,    156,    165,    175,    185,    196,    208,    220,    233,    247   },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 3
                                {  0,   262,   277,    294,    311,    330,    349,    370,    392,    415,    440,    466,    494   },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 4
                                {  0,   523,   554,    587,    622,    659,    698,    740,    784,    831,    880,    932,    988   },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 5
                                {  0,   1047,  1109,   1175,   1245,   1319,   1397,   1480,   1568,   1661,   1760,   1865,   1976  },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 6
                                {  0,   1047,  1109,   1175,   1245,   1319,   1397,   1480,   1568,   1661,   1760,   1865,   1976  }   };  //at the high end of the inputs, interference occurs, if we duplicate this last octave then we can provide a buffer at that high end to make sure we hit it and don't get any audible noise
 

int noteArraybackup[ 8 ][ 13 ] = {    {  0,   15,    16,    17,     18,     19,     21,     22,     23,     25,     26,     28,     29    },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 0 
                                {  0,   31,    33,    35,     37,     39,     41,     44,     46,     49,     52,     55,     58    },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 1
                                {  0,   62,    65,    69,     73,     78,     82,     87,     93,     98,     104,    110,    117   },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 2
                                {  0,   123,   131,   139,    147,    156,    165,    175,    185,    196,    208,    220,    233   },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 3
                                {  0,   247,   262,   277,    294,    311,    330,    349,    370,    392,    415,    440,    466   },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 4
                                {  0,   494,   523,   554,    587,    622,    659,    698,    740,    784,    831,    880,    932   },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 5
                                {  0,   988,   1047,  1109,   1175,   1245,   1319,   1397,   1480,   1568,   1661,   1760,   1865  },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 6
                                {  0,   988,   1047,  1109,   1175,   1245,   1319,   1397,   1480,   1568,   1661,   1760,   1865  }   };  //at the high end of the inputs, interference occurs, if we duplicate this last octave then we can provide a buffer at that high end to make sure we hit it and don't get any audible noise
                                
                                //{  2093,  2217,   2349,   2489,   2637,   2794,   2960,   3136,   3322,   3520,   3729,   3951  },      //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 7
                                //{  4186,  4435,   4699,   4978,   5274,   5588,   5920,   6272,   6645,   7040,   7459,   7902  }   };  //null, c, c#, d, d#, e, f, f#, g, g#, a, a#, b octave 8



void setup()
{
  pinMode(noteIn, INPUT);             //potentiometer selects the note
  pinMode(octaveIn, INPUT);           //potentiometer selects the octave
  pinMode(output, OUTPUT);            //squarewave audio output
  pinMode(trigOut, OUTPUT);           //for triggering effects on subsequent modules
  digitalWrite(trigOut, HIGH);        //it's active low, so set it high by default, and pull low when you want to send a trigger
}


void Trigger()    //this is used to trigger other blocks that perform a time-based function, for example to trigger an envelope or begin a low pass sweep function
{
    digitalWrite(trigOut, LOW);             //pulling the attached attiny's reset pin low will reset, we can use this as a simplified external interrupt to trigger or reset functions on following modules
    digitalWrite(trigOut, HIGH);            //return it to high when done
}

void ReadNote()
{
  noteInRead = analogRead(noteIn); 
  noteInRead = analogRead(noteIn);                    //read twice with a small delay because the ADC multiplexer needs switching time, and the voltage needs time to stabilize after switching

  int noteInTemp = 0;
  
  if(!linearScale)                                      //specifically for 1k voltage divider piano input
  {
    noteInRead = constrain(noteInRead, 440, 1023);      //ignore values below 440, without this line 0v (no keypresses) will cause a floating value
    noteInRead = map(noteInRead, 440, 1023, 0, 1023);   //re-scaling to account for the reduced voltage range due to voltage divider circuit
    noteInTemp = ((sqrt(noteInRead))/2)-3;              //fixing non-linear output voltage from voltage divider piano
    noteInTemp = constrain(noteInTemp, 0, 12);          //stop it going into negative ranges at the low end
  }
  else                                                  //no scaling required
  {
    noteInTemp = map(noteInRead, 0, 1023, 0, 13);
    noteInTemp = constrain(noteInTemp, 0, 12);          //to help remove interference at high end
  }

  if(noteInTemp != 0)
  {
    n = Filter(noteInTemp);
    if(n != prevN)
    {
      //prevN = n;                  //store for comparison next time
      updateSound = true;         //update the sound even before its reached its destination, this allows for slight portamento effect

      if(MAFCounter == 0)
      {
          Trigger();                //pulse trigger in anticipation of the change
          startCounter = true;      //start cooldown counter
      }
    }
  }
  else        //no keypress
  {
    n = 0;
    updateSound = true;
  }

  if(startCounter)              //wait for filter to settle before allowing to trigger again (combats multi-trigger on portamento effect)
  {
    if(MAFCounter<arrayLength)
    {
      MAFCounter++;
    }
    else
    {
      MAFCounter=0;
      startCounter = false;
    }
  }
}

void ReadOctave()
{
  octaveRead = analogRead(octaveIn);      //read twice because the ADC multiplexer needs switching time, and the voltage needs time to stabilize after switching
  octaveRead = analogRead(octaveIn);
  newO = map(octaveRead, 0, 1023, 0, 7);

  if((newO != o) && (abs(octaveRead-prevOctaveRead) > 50))    //hysteresis to help noisy potentiometers deal with threshold values
    updateSound = true;

}

void PlayNote()
{
  prevN = n;
  prevOctaveRead = octaveRead;                  //store once to compare next time
 
  o = map(prevOctaveRead, 0, 1023, 0, 7);       //7 octaves, with an extra at the high end to account for interference at potentiometer limits

  frequency = noteArray[o][n];
  
  if(n != 0)                                    //if a note should be played
  {
    tone(output, frequency);                    //update the tone function with the note we want
  }
  else
    noTone();
}

int Filter(int input)    //moving average filter to reduce noise
{
  int sum = 0;

    for(int i=0; i<arrayLength; i++)
    {
      if(i+1<arrayLength)    //as long as we're in the range of the array
      {
        movingAverageArray[i] = movingAverageArray[i+1];    //shift each element one position to the left         
      }
      movingAverageArray[arrayLength-1] = input;
      sum += movingAverageArray[i];    
    }
    avg = (sum/arrayLength);
    return avg;
}


void loop()
{
  linearScale = digitalRead(linearEnable);
  
  if(updateSound)
  {
    PlayNote();
    updateSound = false;  //make sure it only does it once, whenever updateSound is set true
  }
  else
  {
    ReadOctave();
    ReadNote();  
  }
}
