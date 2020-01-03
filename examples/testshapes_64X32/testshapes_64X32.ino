// demo for all the abilities of the RGBmatrixPanelDue library. public domain!

#include "RGBmatrixPanelDue.h"
//#include "Arduino.h"

// xpanels, ypanels, nplanes (tested w/3)
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

void setup() {
 
  //nYpanels = 1;
  Serial.begin(9600);

  matrix.begin(3000);
  
  // draw a pixel in solid white
  matrix.drawPixel(0, 0, matrix.Color333(7, 7, 7)); 
  delay(2000);

  // fix the screen with green
  matrix.fillRect(0, 0, matrix.width(), matrix.height(), matrix.Color333(0, 7, 0));
  delay(2000);

  // draw a box in yellow
  matrix.drawRect(0, 0, matrix.width(), matrix.height(), matrix.Color333(7, 7, 0));
  delay(2000);
  
  // draw an 'X' in red
  matrix.drawLine(0, 0, matrix.width()-1, matrix.height()-1, matrix.Color333(7, 0, 0));
  matrix.drawLine(matrix.width()-1, 0, 0, matrix.height()-1, matrix.Color333(7, 0, 0));
  delay(2000);
  
  // draw a blue circle
  matrix.drawCircle(10, 10, 10, matrix.Color333(0, 0, 7));
  delay(2000);
  
  // fill a violet circle
  matrix.fillCircle(40, 21, 10, matrix.Color333(7, 0, 7));
  delay(2000);
  
  // fill the screen with 'black'
  matrix.fillScreen(matrix.Color333(0, 0, 0));
  

  matrix.setCursor(8, 0);   // start at top left, with one pixel of spacing
  uint8_t w = 0;
  char *str = "AdafruitIndustries";
  for (w=0; w<8; w++) {
    matrix.setTextColor(Wheel(w));
    matrix.print(str[w]);
  }
  matrix.setCursor(2, 8);    // next line
  for (w=8; w<18; w++) {
    matrix.setTextColor(Wheel(w));
    matrix.print(str[w]);
  }
  matrix.println();
  matrix.setTextColor(matrix.Color333(4,4,4));
  matrix.println("Industries");
  matrix.setTextColor(matrix.Color333(7,7,7));
  matrix.println("LED MATRIX!");
  delay(2000);
  clearScreen();

  // print each letter with a rainbow color
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

  // whew!
}

void loop() {
  // do nothing
}

void clearScreen(){
  matrix.fillScreen(matrix.Color333(0, 0, 0));
  matrix.setCursor(0,0);
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
