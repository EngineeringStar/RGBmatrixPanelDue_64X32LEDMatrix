
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
  // put your setup code here, to run once:
  Serial.begin(9600);

  matrix.begin(3000);

  matrix.println(3);
  matrix.println('3');
  matrix.println("3");
  matrix.println("Henry");
}

void loop() {
  // put your main code here, to run repeatedly:

}
