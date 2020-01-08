// Demonstrates the drawing abilities of the RGBmatrixPanel library.
// Also for the Adafruit GFX Library
// For 32x64 RGB LED matrix.


// COMMENT OUT THIS #DEFINE TO COMPILE FOR THE DUE
//#define USE_MEGA
#ifdef USE_MEGA
#include <RGBmatrixPanel.h>
#include <Adafruit_GFX.h>
#include <fonts/FreeMonoBold18pt7b.h> 
#include <fonts/FreeMonoBold12pt7b.h>

//pin setup for the MEGA using my custom shield
#define CLK 11
#define OE   9
#define LAT  10
#define A   A12
#define B   A15
#define C   A14
#define D   A13
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false, 64);

#else

#include "RGBmatrixPanelDue.h"
#include <gfxfont.h>
#include <fonts/FreeMonoBold18pt7b.h> 
#include <fonts/FreeMonoBold12pt7b.h>
#define Height16 0
#define Height32 1
// xpanels, ypanels, nplanes (tested w/3)
uint8_t nXpanels = 2; // Width = nXpanel * 32 = 64
uint8_t nYpanels = 1; // Height = nYpanel * 32 = 32

RGBmatrixPanelDue matrix(Height32, nXpanels, nYpanels, 3);

// wrapper for the redrawing code, this gets called by the interrupt
//TIMER: TC1 ch 0
void TC3_Handler()
{
  TC_GetStatus(TC1, 0);
  matrix.updateDisplay();
}

#endif

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
#define COOL_BLUE  0x6B6D


void setup() {

  Serial.begin(115200);
  Serial.println("Starting...");

#ifdef USE_MEGA
  matrix.begin();
#else
  matrix.begin(10000);
#endif

  // draw a pixel in solid white
  matrix.drawPixel(0, 0, matrix.Color333(7, 7, 7));
  delay(500);

  // fix the screen with green
  matrix.fillRect(0, 0, matrix.width(), matrix.height(), matrix.Color333(0, 7, 0));
  delay(500);

  // draw a box in yellow
  matrix.drawRect(0, 0, matrix.width(), matrix.height(), matrix.Color333(7, 7, 0));
  delay(500);

  // draw an 'X' in red
  matrix.drawLine(0, 0, matrix.width()-1, matrix.height()-1, matrix.Color333(7, 0, 0));
  matrix.drawLine(matrix.width()-1, 0, 0, matrix.height()-1, matrix.Color333(7, 0, 0));
  delay(500);

  // draw a blue circle
  matrix.drawCircle(10, 10, 10, matrix.Color333(0, 0, 7));
  delay(500);

  // fill a violet circle
  matrix.fillCircle(40, 21, 10, matrix.Color333(7, 0, 7));
  delay(500);

  // fill the screen with 'black'
  matrix.fillScreen(matrix.Color333(0, 0, 0));

  // draw some text!
  matrix.setTextSize(1);     // size 1 == 8 pixels high
  //matrix.setTextWrap(false); // Don't wrap at end of line - will do ourselves

  matrix.setCursor(8, 0);    // start at top left, with 8 pixel of spacing
  uint8_t w = 0;
  char *str = "AdafruitIndustries";
  for (w=0; w<8; w++) {
    matrix.setTextColor(Wheel(w));
    matrix.print(str[w]);
    delay(100);
  }
  matrix.setCursor(2, 8);    // next line
  for (w=8; w<18; w++) {
    matrix.setTextColor(Wheel(w));
    matrix.print(str[w]);
    delay(100);
  }
  matrix.println();
  matrix.setTextColor(matrix.Color333(4,4,4));
  matrix.println("Industries");
  matrix.setTextColor(matrix.Color333(7,7,7));
  matrix.println("LED MATRIX!");

  delay(2500);

  // print each letter with a rainbow color
  matrix.fillScreen(BLACK);
  matrix.setCursor(4, 4);
  matrix.setTextColor(matrix.Color333(7,0,0));
  matrix.print('3');
  matrix.setTextColor(matrix.Color333(7,4,0));
  matrix.print('2');
  matrix.setTextColor(matrix.Color333(7,7,0));
  matrix.print('x');
  matrix.setTextColor(matrix.Color333(4,7,0));
  matrix.print('6');
  matrix.setTextColor(matrix.Color333(0,7,0));
  matrix.print('4');
  matrix.setCursor(34, 24);
  matrix.setTextColor(matrix.Color333(0,7,7));
  matrix.print('*');
  matrix.setTextColor(matrix.Color333(0,4,7));
  matrix.print('R');
  matrix.setTextColor(matrix.Color333(0,0,7));
  matrix.print('G');
  matrix.setTextColor(matrix.Color333(4,0,7));
  matrix.print('B');
  matrix.setTextColor(matrix.Color333(7,0,4));
  matrix.print('*');
  
  delay(2500);

unsigned long sTime = millis();
  matrix.fillScreen(BLACK);
  matrix.fillRect(0, 10, 64, 11, BLUE);
  matrix.setFont(); 
  matrix.setTextSize(1);
  //matrix.setTextWrap(false);
  matrix.setTextColor(WHITE);
  matrix.setCursor(1, 12);
  matrix.print("LEVEL");
  matrix.setCursor(31, 12);
  matrix.print("UP");
  matrix.setCursor(43, 12);
  matrix.print("l");
  matrix.setCursor(47, 12);
  matrix.print("i");
  matrix.setCursor(52, 12);
  matrix.print("v");
  matrix.setCursor(58, 12);
  matrix.print("e");
Serial.println("Display time: " + String(millis() - sTime));

  delay(500);

  for (int i = 20; i > 0; i--) {
    matrix.fillScreen(BLACK);
    matrix.fillCircle(32, 16, 15, BLUE);
    matrix.setTextColor(WHITE);
    
    if (i < 10) {
      matrix.setFont(&FreeMonoBold18pt7b);
      matrix.setTextSize(1);
      matrix.setCursor(22, 27); 
      String tempString = String(i);
      matrix.print(tempString.c_str());
    } else {
      matrix.setFont(&FreeMonoBold12pt7b);
      matrix.setTextSize(1);
      matrix.setCursor(19, 22); 
      String tempString = String(i);
      matrix.print(tempString.c_str());
      //matrix.print(String(i));
    }
    delay(250);
  } 

  matrix.fillScreen(BLACK);
  matrix.fillRect(0, 6, 64, 18, GREEN);
  matrix.setFont(&FreeMonoBold12pt7b);
  matrix.setTextColor(WHITE);
  matrix.setCursor(4, 21); 
  matrix.setTextSize(1);
  matrix.print("DONE");

  // whew!
    Serial.println("...done!");

}

void loop() {
  // Do nothing -- image doesn't change
}


// Input a value 0 to 24 to get a color value.
// The colours are a transition r - g - b - back to r.
uint16_t Wheel(byte WheelPos) {
  if(WheelPos < 8) {
   return matrix.Color333(7 - WheelPos, WheelPos, 0);
  } else if(WheelPos < 16) {
   WheelPos -= 8;
   return matrix.Color333(0, 7-WheelPos, WheelPos);
  } else {
   WheelPos -= 16;
   return matrix.Color333(WheelPos, 0, 7 - WheelPos);
  }
}
