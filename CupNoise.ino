#define LOG_OUT 1 // use the log output function
#define FHT_N 64 // set to 256 point fht
#include <FHT.h> // include the library
#include<FastLED.h>


#define pin1 8
#define pin2 9
#define pin3 10
#define pin4 11
#define pin5 12

#define strip1 26
#define strip2 39
#define strip3 23
#define strip4 41
#define strip5 17

#define MIC_PIN 0                                             // Analog port for microphone
#define DC_OFFSET  0                                         // DC offset in mic signal - if unusure, leave 0

const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;


const uint8_t kColumns = 23;
const uint8_t kRows = 9;
const uint8_t kMatrixWidth = kColumns;
const uint8_t kMatrixHeight = kRows;
byte inByte = 0;
boolean serialComplete = false;

#define NUM_LEDS (kColumns * kRows)
#define numLeds NUM_LEDS

//
// Mark's xy coordinate mapping code.  See the XYMatrix for more information on it.
//

// Params for width and height
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)

// This function will return the right 'led index number' for 
// a given set of X and Y coordinates on your RGB Shades. 
// This code, plus the supporting 80-byte table is much smaller 
// and much faster than trying to calculate the pixel ID with code.
// Params for width and height
uint16_t XY( uint8_t x, uint8_t y)
{
  //Serial.print("XY(");Serial.print(x);Serial.print(",");Serial.print(y);Serial.print(")");Serial.println();

  const uint16_t mappedTable[kRows * kColumns] = 
  {
//        0,   1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,   21,   22,    
/* 0 */  24,   24,   24,   24,   24,   24,   25,   25,   25,   25,   25,   25,   25,   25,   23,   23,   23,   23,   23,   23,   23,   25,   25,
/* 1 */  20,   19,   19,   18,   18,   18,   17,   17,   16,   16,   16,   15,   15,   14,   14,   14,   22,   22,   22,   21,   21,   21,   20,
/* 2 */  8,    7,    7,    6,    6,    5,    4,    4,    3,    3,    2,    1,    1,    0,    0,    13,   12,   12,   11,   10,   10,   9,    9,
/* 3 */  57,   56,   55,   54,   54,   53,   52,   52,   51,   50,   49,   48,   48,   47,   64,   63,   62,   62,   61,   60,   59,   59,   58,
/* 4 */  37,   36,   35,   34,   33,   32,   31,   31,   30,   29,   28,   27,   26,   46,   45,   44,   43,   42,   41,   40,   39,   39,   38,
/* 5 */  76,   75,   74,   73,   72,   71,   70,   69,   68,   67,   66,   65,   87,   86,   85,   84,   83,   82,   81,   80,   79,   78,   77,
/* 6 */  115,  114,  113,  112,  111,  110,  109,  108,  107,  106,  105,  126,  125,  124,  123,  122,  121,  121,  120,  119,  118,  117,  116,  
/* 7 */  94,   93,   92,   91,   90,   89,   88,   88,   145,  144,  104,  103,  102,  101,  100,  100,  99,   99,   98,   97,   96,   95,   94, 
/* 8 */  135,  134,  133,  132,  131,  130,  130,  130,  129,  143,  142,  141,  141,  140,  139,  139,  138,  138,  137,  137,  136,  135,  134,  
  };

  // lazy bounds checking
  if (x >= kColumns)
    x = 0;
  if (y >= kRows)
    y = 0;

  uint16_t i;
  i = (y * kColumns) + x;
  //Serial.print("i(");Serial.print(i);Serial.print(") = ");Serial.print(y);Serial.print(" * ");Serial.print(kColumns);Serial.print(" + ");Serial.print(x);Serial.print(" --- ");
  //Serial.flush();
  
  if( i >= sizeof(mappedTable) ) return 0;
 
  uint16_t j = mappedTable[i];
  //Serial.println(j);
  return j;

}

// The leds
CRGB leds[kMatrixWidth * kMatrixHeight];
bool drops[kMatrixWidth][kMatrixHeight] = {false};

void setup() {
  // uncomment the following lines if you want to see FPS count information
  // Serial.begin(38400);
  // Serial.println("resetting!");
  pinMode(A0, INPUT);
  delay(200);
  LEDS.addLeds<WS2812B, pin1, GRB>(leds, strip1).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin2, GRB>(leds, strip1, strip2).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin3, GRB>(leds, strip1 + strip2, strip3).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin4, GRB>(leds, strip1 + strip2 + strip3, strip4).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin5, GRB>(leds, strip1 + strip2 + strip3 + strip4, strip5).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(255);
  set_max_power_in_volts_and_milliamps(5, 3000);
  setAllColor(CRGB::Black);

  Serial.begin(115200);

  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0
}

void simpleTest() 
{
  LEDS.show();
  delay(1000);
  static byte hue = 0;
  for(int row = 0;row < kRows;row++)
  {
    for(int col = 0;col < kColumns;col++)
    {

      leds[XY(col, row)] = CHSV(hue++,255,255);
      LEDS.show();
      delay(1);
    }
  }
}

void setAllColor(CRGB color)
{
  for(int col = 0;col<kColumns;col++)
  {
    for (int row = 0;row<kRows;row++)
    {
      leds[XY(col, row)] = color;
    }
  }
}

// make all lights in a row the same color
void setRowColor(byte row, CRGB color)
{
  for(int col = 0;col < kColumns;col++)
  {
    leds[XY(col, row)] = color;
  }  
}

// make all lights in a column the same color
void setColumnColor(byte column, CRGB color)
{
  for(int row = 0;row < kRows;row++)
  {
    leds[XY(column, row)] = color;
  }
}

// takes an 8-bit height and maps it to column
void setColumnHeight(byte column, byte height, CRGB color)
{
  for(int row = 0;row<kRows;row++)
  {
    if (row < height/9  )
      leds[XY(column,row)] = color;
    else
      leds[XY(column,row)].fadeToBlackBy( 64 );
  }

}

void columnWipe()
{
  static byte hue = 0;
  for(int col = 0;col < kColumns;col++)
  {
    hue += 20;
    setColumnColor(col, CHSV(hue,255,255));
    show_at_max_brightness_for_power();
    delay(50);
  }
}

void setLedFromSerial()
{
  if (serialComplete)
  {
    setAllColor(CRGB::Black);
    Serial.print("is ");Serial.println(inByte);
    leds[inByte] = CRGB::Fuchsia;
    inByte = 0;
    serialComplete = false;
    show_at_max_brightness_for_power();
      //setRowColor(i, CRGB::Black);
  }
}

void setColFromSerial()
{
  if (serialComplete)
  {

    setAllColor(CRGB::Black);
    Serial.print("is ");Serial.println(inByte);
    if (inByte < kColumns)
      setColumnColor(inByte, CRGB::Fuchsia);
    leds[inByte] = CRGB::Blue;
    inByte = 0;
    serialComplete = false;
    show_at_max_brightness_for_power();
    //setRowColor(i, CRGB::Black);  
  }
}

uint32_t getAmplitude()
{
   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level
 
   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;
 
   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(0);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
 
   Serial.println(peakToPeak);
   return peakToPeak;
}

byte hue = 0;

void loop() {
  static unsigned long nextChange = millis() + 50;

  
  // loop forever
  while(true)
  {
    getFHT();
    //Serial.write(255); // send a start byte
    //Serial.write(fht_log_out, FHT_N/2); // send out the data/*

    //waterDroplets();
      doLeds();
      LEDS.show(); 
      LEDS.delay(20);
  }
  LEDS.delay(20);
}

void getFHT()
{
  cli();  // UDRE interrupt slows this way down on arduino1.0
  for (int i = 0 ; i < FHT_N ; i++) { // save 256 samples
    while(!(ADCSRA & 0x10)); // wait for adc to be ready
    ADCSRA = 0xf5; // restart adc
    byte m = ADCL; // fetch adc data
    byte j = ADCH;
    int k = (j << 8) | m; // form into an int
    k -= 0x0200; // form into a signed int
    k <<= 6; // form into a 16b signed int
    fht_input[i] = k; // put real data into bins
  }
  fht_window(); // window the data for better frequency response
  fht_reorder(); // reorder the data before doing the fht
  fht_run(); // process the data in the fht
  fht_mag_log(); // take the output of the fht
  sei();
}

const byte changeDuration = 50;
void doLeds()
{
  static byte hue = 0;
  for(int col = 0;col < kColumns;col++)
  {
    if (fht_log_out[0] > 128)
    {
      hue += 10;
    }
  //Serial.print(col);Serial.print(".");Serial.print(fht_log_out[col]);Serial.print(", ");
    //hue += 10;
    setColumnHeight(col, fht_log_out[col+2], CHSV(hue,255,255));
  }
  //Serial.println();
}

void waterDroplets()
{
  // first seed the top row with a droplet
  byte firstDrop = random8(0,22);

  // i use a seperate drop array because the head is a globe
  // and so leds has a lot of duplicates in it
  for(int i = 0;i<kColumns;i++)
  {
    drops[i][0] =false;
  }
  drops[firstDrop][0] = true;


  // I'm just copying (from the bottom) a row down
  for(int row = kRows -1;row > 0;row--)
  {
    for (int col = kColumns-1;col >= 0;col--)
    {
      drops[col][row] = drops[col][row-1];
    }    
  }

  for(int row = kRows -1;row >= 0;row--)
  {
    for (int col = kColumns-1;col >= 0;col--)
    {
      if(drops[col][row] == true)
      {
        leds[XY(col,row)] = CRGB::Blue;
      } 
      else
      {
        leds[XY(col,row)] /= 2;        
      }
    }    
  }
  Serial.println("dropped");
}


/*
void looped()
{
  byte fhtresults[kColumns];
  byte hue = 0;

  cli();  // UDRE interrupt slows this way down on arduino1.0
  for (int i = 0 ; i < FHT_N ; i++) { // save 256 samples
    while(!(ADCSRA & 0x10)); // wait for adc to be ready
    ADCSRA = 0xf5; // restart adc
    byte m = ADCL; // fetch adc data
    byte j = ADCH;
    int k = (j << 8) | m; // form into an int
    k -= 0x0200; // form into a signed int
    k <<= 6; // form into a 16b signed int
    fht_input[i] = k; // put real data into bins
  }
  fht_window(); // window the data for better frequency response
  fht_reorder(); // reorder the data before doing the fht
  fht_run(); // process the data in the fht
  fht_mag_log(); // take the output of the fht
  for(int i = 0;i< kColumns;i++)
  {
    fhtresults[i] = fht_log_out[i]; // send out the data
  }  
  sei();

  for(int i = 0;i< kColumns;i++)
  {
    hue = fhtresults[i];
    setColumnColor(i, CHSV(hue,255,255));
  }
  //columnWipe();
  //soundmems();
  //Serial.println(LEDS.getFPS());

  //getAmplitude();
  //setAllColor(CRGB::White);
  //setLedFromSerial();
  //setColFromSerial();
  LEDS.show();
  //show_at_max_brightness_for_power();
  //delay_at_max_brightness_for_power(60);

  LEDS.showColor(CRGB::Black);
  for (byte col = 0;col<kColumns;col++)
  {
    hue += 20;
    setColumnColor(col, CHSV(hue,255,255));
  }

  //setSerial();
  LEDS.show();
  delay(10000);
}
*/


void soundmems() {
  int n;
  for (int i = 0; i<NUM_LEDS; i++) {
    n = analogRead(MIC_PIN);                                   // Raw reading from mic
    n = abs(n - 512 - DC_OFFSET);                              // Center on zero
    leds[i] = CHSV((n*2 % 255), 255, (n*2)% 255);
  }
} // soundmems()


void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char c = (char)Serial.read(); 
    Serial.print(" ");Serial.print(c);
    if (c >= '0' && c <= '9')
    {
      inByte = (inByte * 10) + (c - '0');
    } 
    else if('\n' == c) // character is newline     
    {
      serialComplete = true;
    }
  }
}


