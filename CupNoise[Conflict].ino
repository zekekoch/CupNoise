#include<FastLED.h>

// How many leds are in the strip?
#define numLeds 144

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

const uint8_t kColumns = 23;
const uint8_t kRows = 9;
const uint8_t kMatrixWidth = kColumns;
const uint8_t kMatrixHeight = kRows;

#define NUM_LEDS (kColumns * kRows)

//
// Mark's xy coordinate mapping code.  See the XYMatrix for more information on it.
//

// Params for width and height
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)
// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;

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
    63, 63, 63, 63, 63, 63, 64, 64, 64, 64, 64, 64, 64, 62, 62, 62, 62, 62, 62, 62, 62, 63, 63,
    60, 59, 59, 58, 58, 58, 57, 57, 56, 56, 56, 55, 55, 54, 54, 54, 53, 61, 61, 61, 61, 60, 60,
    48, 47, 47, 46, 46, 45, 44, 44, 43, 43, 42, 41, 41, 40, 40, 39, 52, 51, 51, 50, 50, 49, 49,
    32, 31, 30, 30, 29, 28, 27, 27, 26, 25, 24, 23, 23, 22, 21, 38, 37, 36, 36, 35, 34, 34, 33,
    12, 11, 10, 9, 8, 7, 6, 6, 5, 4, 3, 2, 1, 0, 20, 19, 18, 17, 16, 15, 14, 14, 13,
    116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117,
    92, 91, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93,
    71, 71, 70, 69, 68, 67, 66, 65, 65, 143, 142, 81, 80, 79, 78, 78, 77, 76, 75, 74, 73, 73, 72,
    131, 131, 131, 130, 129, 128, 129, 127, 127, 127, 141, 140, 140, 138, 138, 137, 136, 135, 135, 134, 133, 133, 132
  };

  uint16_t i;
  i = (y * kColumns) + x;
  
  if( i >= sizeof(mappedTable) ) return 0;
 
  uint16_t j = mappedTable[i];
  //Serial.println(j);
  return j;

}

uint16_t oldXY( uint8_t x, uint8_t y)
{
  uint16_t i;

  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }

  return i;
}

// The leds
CRGB leds[kMatrixWidth * kMatrixHeight];

// The 32bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

// We're using the x/y dimensions to map to the x/y pixels on the matrix.  We'll
// use the z-axis for "time".  speed determines how fast time moves forward.  Try
// 1 for a very slow moving effect, or 60 for something that ends up looking like
// water.
// uint16_t speed = 1; // almost looks like a painting, moves very slowly
uint16_t speed = 20; // a nice starting speed, mixes well with a scale of 100
// uint16_t speed = 33;
// uint16_t speed = 100; // wicked fast!

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.

// uint16_t scale = 1; // mostly just solid colors
// uint16_t scale = 4011; // very zoomed out and shimmery
uint16_t scale = 311;

// This is the array that we keep our computed noise values in
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];

void setup() {
  // uncomment the following lines if you want to see FPS count information
  // Serial.begin(38400);
  // Serial.println("resetting!");
  delay(1000);
  LEDS.addLeds<WS2812B, pin1, GRB>(leds, strip1).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin2, GRB>(leds, strip1, strip2).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin3, GRB>(leds, strip1 + strip2, strip3).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin4, GRB>(leds, strip1 + strip2 + strip3, strip4).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin5, GRB>(leds, strip1 + strip2 + strip3 + strip4, strip5).setCorrection(TypicalSMD5050);

  LEDS.setBrightness(96);

  // Initialize our coordinates to some random values
  x = random16();
  y = random16();
  z = random16();
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  for(int i = 0; i < MAX_DIMENSION; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < MAX_DIMENSION; j++) {
      int joffset = scale * j;
      noise[i][j] = inoise8(x + ioffset,y + joffset,z);
    }
  }
  z += speed;
}

void loop() 
{
  static byte hue = 0;
  for(int row = 0;row < kRows;row++)
  {
    for(int col = 0;col < kColumns;col++)
    {

      leds[XY(col, row)] = CHSV(hue++,255,255);
      LEDS.show();
      delay(50);
    }
  }

}


void simpleloop() {
  static uint8_t ihue=0;
  fillnoise8();
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's hue.
      leds[XY(i,j)] = CHSV(noise[j][i],255,noise[i][j]);

      // You can also explore other ways to constrain the hue used, like below
      // leds[XY(i,j)] = CHSV(ihue + (noise[j][i]>>2),255,noise[i][j]);
    }
  }
  ihue+=1;

  LEDS.show();
  // delay(10);
}
