const int output = 1;     //pin6 - can be PB1 or PB4, PB4 works better for Tone because we can use hardware output compare
const int trigOut = 0;    //pin5
int noteIn = A3;          //pin2
int octaveIn = A2;        //pin3
int linearEnable = 2;     //pin7
bool linearScale = false;
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

//bool play = true;
//volatile bool interruptFlag = false;
//int noteLength = 1000;
//int playButton = 2;       //interrupt pin 7

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
  pinMode(trigOut, OUTPUT);
  digitalWrite(trigOut, HIGH);
}


void Trigger()    //this is used to trigger other blocks that perform a time-based function, for example to trigger an envelope or begin a low pass sweep function
{
    digitalWrite(trigOut, LOW);             //pulling the attached attiny's reset pin low will reset, we can use this as a simplified external interrupt to trigger or reset functions on following modules
    //delay(100);
    digitalWrite(trigOut, HIGH);
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
  
  
  //n = NoteFilter(noteInTemp);
  //n = Filter(noteInTemp);                    //feed the moving average filter with a new sample
  //n = noteInTemp;

  //if(n != prevN)
 // if(MAFCounter = 0)
  //  updateSound = true;
}

void ReadOctave()
{
  //delay(10);
  octaveRead = analogRead(octaveIn);      //read twice with a small delay because the ADC multiplexer needs switching time, and the voltage needs time to stabilize after switching
  //delay(10);
  octaveRead = analogRead(octaveIn);
  newO = map(octaveRead, 0, 1023, 0, 7);

  if((newO != o) && (abs(octaveRead-prevOctaveRead) > 50))    //hysteresis to help noisy potentiometers deal with threshold values
    updateSound = true;

}

void PlayNote()
{
  //prevNoteRead = noteRead;                      //store once to compare next time
  prevN = n;
  prevOctaveRead = octaveRead;                  //store once to compare next time
  
  //n = map(prevNoteRead, 0, 1023, 0, 12);        //scale 0 to 12, for 11 notes and one 0 condition which stops the note
 
  o = map(prevOctaveRead, 0, 1023, 0, 7);       //7 octaves, with an extra at the high end to account for interference at potentiometer limits

  frequency = noteArray[o][n];
  //frequency = noteArray[o][n-1];                //update the frequency (the n-1 is because of the 0 condition)
  
  if(n != 0)
  {
    tone(output, frequency);                    //update the tone function
    //Trigger();                                  //pulse trigger pin whenever a note is played
  }
  else
    noTone();
  /*
  else
  {
    for(int i=0; i<500; i++)                    //sustain on key-up for envelope generator, this also fixes debouncing issues on low quality keyboard keys
    {
      delay(1);
    }
    noTone();                                   //use 0v to stop the oscillator
    //MAFCounter = 0;                             //reset the filter
  }
  */
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


/*
int NewFilter(int newSample)
{
  for(int i = arrayLength-1; i < 0; i--)
  {
    movingAverageArray[i+1] = movingAverageArray[i];
  }

  movingAverageArray[0] = newSample;

  for(int i=0; i<arrayLength; i++)
  {
    sum += movingAverageArray[i];
  }
  avg = (sum / arrayLength);
  return avg;

  
  if(MAFCounter<arrayLength)
  {
    MAFCounter++;
  }
  else
  {
    MAFCounter = 0;
  }
  
}
*/

void loop()
{
  linearScale = digitalRead(linearEnable);
  
  if(updateSound)
  {
    PlayNote();
    //MAFCounter = 0;
    updateSound = false;
  }
  else
  {
    //delay(5);
    ReadOctave();
    //delay(5);
    ReadNote();  
  }
}

/*
int FilterNote()    //moving average filter to reduce noise
{
  int sum = 0;
  
  //for(int i=0; i<arrayLength; i++)      //this nested loop ensures that the array is fully repopulated before returning the result
  //{
  
    for(int i=0; i<arrayLength; i++)
    {
      if(i+1<arrayLength)    //as long as we're in the range of the array
        movingAverageArray[i] = movingAverageArray[i+1];    //shift each element one position to the left 
    }
    
    movingAverageArray[arrayLength-1] = analogRead(noteIn);   //last position is the newest addition
      
    for(int i=0; i<arrayLength; i++)
    { 
      sum += movingAverageArray[i];
    }    
  //}

  //n = (sum/arrayLength);
  return(sum / arrayLength);                   //find the average and output it
}



/*
void ReadNote()     //deprecated hysteresis for note input
{
  delay(10);
  noteRead = analogRead(noteIn);
  delay(10);
  noteRead = analogRead(noteIn);        
  newN = map(noteRead, 0, 1023, 0, 12); //scale to 12, for 11 notes and one 0 condition which stops the note
  delay(10);
   
  if((newN != n) && (abs(noteRead-prevNoteRead) > 25))
    updateSound = true;

}
*/

/* INTERRUPT SETUP
void setup()
{
  cli();                              //disable interrupts during setup

  MCUCR |= B00000011;                 //watch for rising edge, B00000010 is falling edge
  GIMSK |= B01000000;                 //enable external interrupt on pin 7
  SREG  |= B10000000;                 //global interrupt enable
  
  pinMode(trig, INPUT);         //interrupt button to trigger an update on the note
  pinMode(noteIn, INPUT);             //potentiometer selects the note
  pinMode(octaveIn, INPUT);           //potentiometer selects the octave
  pinMode(output, OUTPUT);            //for the speaker

  sei();                              //last line of setup - enable interrupts after setup
}

ISR (INT0_vect)        // Interrupt service routine 
{

  
}
*/


/*
// Cater for 16MHz, 8MHz, or 1MHz clock:
const int Clock = ((F_CPU/1000000UL) == 16) ? 4 : ((F_CPU/1000000UL) == 8) ? 3 : 0;
const uint8_t scale[] PROGMEM = {239,226,213,201,190,179,169,160,151,142,134,127};


void note (int n, int octave) {
  int prescaler = 8 + Clock - (octave + n/12);
  if (prescaler<1 || prescaler>15 || octave==0) prescaler = 0;
  DDRB = (DDRB & ~(1<<Output)) | (prescaler != 0)<<Output;
  OCR1C = pgm_read_byte(&scale[n % 12]) - 1;
  GTCCR = (Output == 4)<<COM1B0;
  TCCR1 = 1<<CTC1 | (Output == 1)<<COM1A0 | prescaler<<CS10;
}
*/





/*
void PlayNoteOld()
{
  //o = map(analogRead(octaveIn), 0, 970, 0, 6);
  //n = map(analogRead(noteIn), 0, 1023, 0, 12); //scale 0 to 12, for 11 notes and one 0 condition which stops the note

  
  if(noteRead <= 80)
    n = 0;
  else if(noteRead > 80 && noteRead <= 157)
    n = 1;
  else if(noteRead > 157 && noteRead <= 236)
    n = 2;
  else if(noteRead > 236 && noteRead <= 315)
    n = 3;
  else if(noteRead > 315 && noteRead <= 393)
    n = 4;
  else if(noteRead > 393 && noteRead <= 472)
    n = 5;
  else if(noteRead > 472 && noteRead <= 551)
    n = 6;
  else if(noteRead > 551 && noteRead <= 630)
    n = 7;
  else if(noteRead > 630 && noteRead <= 708)
    n = 8;
  else if(noteRead > 708 && noteRead <= 787)
    n = 9;
  else if(noteRead > 787 && noteRead <= 866)
    n = 10;
  else if(noteRead > 866 && noteRead <= 944)
    n = 11;
  else if(noteRead > 944 && noteRead <= 1023)
    n = 12;


  if(octaveRead <= 146)
    o = 0;
  else if(octaveRead > 146 && octaveRead <= 292)
    o = 1;
  else if(octaveRead > 292 && octaveRead <= 438)
    o = 2;
  else if(octaveRead > 438 && octaveRead <= 584)
    o = 3;
  else if(octaveRead > 584 && octaveRead <= 730)
    o = 4;
  else if(octaveRead > 730 && octaveRead <= 876)
    o = 5;
  else if(octaveRead > 876 && octaveRead <= 1023)
    o = 6;
  
  frequency = noteArray[o][n-1]; //if its changed then update the frequency. n-1 because of the 0 condition.
  
  if(n != 0)
  {
    tone(output, frequency); 
  }
  else
    noTone();

  prevNoteRead = noteRead; //analogRead(noteIn);                      //store to compare next time
  prevOctaveRead = octaveRead; //analogRead(octaveIn);                  //store to compare next time
  //prevN = n; //map(analogRead(noteIn), 0, 1023, 0, 12);        //store to compare next time
  //prevO = o;    //map(analogRead(octaveIn), 0, 1023, 0, 6);       //store to compare next time
}
*/



  /*
  noteRead = analogRead(noteIn); 
  octaveRead = analogRead(octaveIn);

  if(abs(noteRead-prevNoteRead) > 25)
  {
    PlayNote();
  }
  //else
  //  n = prevN;

  if(abs(octaveRead-prevOctaveRead) > 50) 
  {
    PlayNote();
  }
  //else
  //  o = prevO;

  
  //delay(10);


  //if((n != prevN) || (o != prevO))  //if its changed since last time
  //{
  //  if(((analogRead(noteIn) > prevNoteRead+3) || (analogRead(noteIn) < prevNoteRead-3)) || ((analogRead(octaveIn) > prevOctaveRead+5) || (analogRead(octaveIn) < prevOctaveRead-5)))
    //hysteresis logic to stop artifacts caused by noisy potentiometers
  //  {
  //    frequency = noteArray[o][n-1]; //if its changed then update the frequency. n-1 because of the 0 condition.
  //  }
  //}
  //playNote();
}


    //interruptFlag = false;
    //play = false;
    //playNote();
    //for(int i=0; i<noteLength && interruptFlag==0; i++)
    //{
    //  delay(1); 
    //}
    //noTone();
  
 
  
  //if(play)
  //{
    //play = false;
    //interruptFlag = false;

    

    
    //tone(1, map(analogRead(noteIn), 0, 1023, 16, 8000));

    //for(int i=0; i<noteLength && interruptFlag==0; i++)
    //{
    //  delay(1); 
    //}

    
    
    //note(n,o);
    //for(int i=0; i<noteLength && interruptFlag==0; i++)
    //{
      //note(map(analogRead(noteIn), 0, 1023, 0, 11), map(analogRead(octaveIn), 0, 1023, 1, 7));    //note(note, octave)
      
     // delay(1); 
    //}
    //for(int i=0; i<noteLength && interruptFlag==0; i++)
    //{
    //  note(map(analogRead(noteIn), 0, 1023, 0, 11), map(analogRead(octaveIn), 0, 1023, 1, 7));    //note(note, octave)
    //  delay(1); 
    //}
    
 // }
 // else
    
    //noTone();
    //note(0,0);
//} */
