#include "RGBmatrixPanelDue.h"
#include "glcdfontDue.c"


#include <Arduino.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#endif

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

// Pointers are a peculiar case...typically 16-bit on AVR boards,
// 32 bits elsewhere.  Try to accommodate both...

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
#define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
#define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
#ifdef __AVR__
  return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
#else
  // expression in __AVR__ section may generate "dereferencing type-punned
  // pointer will break strict-aliasing rules" warning In fact, on other
  // platforms (such as STM32) there is no need to do this pointer magic as
  // program memory may be read in a usual way So expression may be simplified
  return gfxFont->glyph + c;
#endif //__AVR__
}

inline uint8_t *pgm_read_bitmap_ptr(const GFXfont *gfxFont) {
#ifdef __AVR__
  return (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);
#else
  // expression in __AVR__ section generates "dereferencing type-punned pointer
  // will break strict-aliasing rules" warning In fact, on other platforms (such
  // as STM32) there is no need to do this pointer magic as program memory may
  // be read in a usual way So expression may be simplified
  return gfxFont->bitmap;
#endif //__AVR__
}

uint8_t RGBmatrixPanelDue::width() {return WIDTH; }

uint8_t RGBmatrixPanelDue::height() {return HEIGHT; }

RGBmatrixPanelDue::RGBmatrixPanelDue(uint8_t matrix_type, uint8_t xpanels, uint8_t ypanels, uint8_t planes) {
  single_matrix_width = 32;
  single_matrix_height = matrix_type == MATRIX_32_32 ? 32 : 16;
  init(xpanels, ypanels, planes);
}

RGBmatrixPanelDue::RGBmatrixPanelDue(uint8_t xpanels, uint8_t ypanels, uint8_t planes) {
  // Keep old constructor for backwards compability, assuming 16x32 matrix
  single_matrix_width = 32;
  single_matrix_height = 16;
  init(xpanels, ypanels, planes);
}

void RGBmatrixPanelDue::init(uint8_t xpanels, uint8_t ypanels, uint8_t planes) {
  NX = xpanels;
  NY = ypanels;

  PWMBITS = planes;
  PWMMAX = ((1 << PWMBITS) - 1);

  // Save number of sections once because dividing by two is oh so expensive
  sections = single_matrix_height /2;

  WIDTH = single_matrix_width * xpanels;
  HEIGHT = single_matrix_height * ypanels;

  NUMBYTES = (WIDTH * HEIGHT / 2) * planes;
  matrixbuff = (uint8_t *)malloc(NUMBYTES);
  // set all of matrix buff to 0 to begin with
  memset(matrixbuff, 0, NUMBYTES);


  pwmcounter = 0;
  scansection = 0;

  cursor_x = cursor_y = 0;
  textsize = 1;
  textcolor = Color333(7,7,7); // white

}

void RGBmatrixPanelDue::begin(uint32_t frequency) {
  pinMode(APIN, OUTPUT);
  digitalWrite(APIN, LOW);
  pinMode(BPIN, OUTPUT);
  digitalWrite(BPIN, LOW);
  pinMode(CPIN, OUTPUT);
  digitalWrite(CPIN, LOW);
  pinMode(DPIN, OUTPUT); // Only applies to 32x32 matrix
  digitalWrite(DPIN, LOW);
  pinMode(LAT, OUTPUT);
  digitalWrite(LAT, LOW);
  pinMode(CLK, OUTPUT);
  digitalWrite(CLK, HIGH);

  pinMode(OE, OUTPUT);
  digitalWrite(OE, LOW);

  pinMode(R1,OUTPUT);
  pinMode(R2,OUTPUT);
  pinMode(G1,OUTPUT);
  pinMode(G2,OUTPUT);
  pinMode(B1,OUTPUT);
  pinMode(B2,OUTPUT);
  digitalWrite(R1,LOW);
  digitalWrite(R2,LOW);
  digitalWrite(G1,LOW);
  digitalWrite(G2,LOW);
  digitalWrite(B1,LOW);
  digitalWrite(B2,LOW);

  startTimer(TC1, 0, TC3_IRQn, frequency); //TC1 channel 0, the IRQ for that channel and the desired frequency

/*Serial.println(WIDTH);
  Serial.println(HEIGHT);
  Serial.println(PWMMAX);
  Serial.println(NUMBYTES);
  Serial.println("end of matrix.begin"); */

}

uint16_t RGBmatrixPanelDue::Color333(uint8_t r, uint8_t g, uint8_t b) {
  return Color444(r,g,b);
}

uint16_t RGBmatrixPanelDue::Color444(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t c;

  c = r;
  c <<= 4;
  c |= g & 0xF;
  c <<= 4;
  c |= b & 0xF;
  return c;
}

uint16_t RGBmatrixPanelDue::Color888(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t c;

  c = (r >> 5);
  c <<= 4;
  c |= (g >> 5) & 0xF;
  c <<= 4;
  c |= (b >> 5) & 0xF;

  /*
  Serial.print(r, HEX); Serial.print(", ");
  Serial.print(g, HEX); Serial.print(", ");
  Serial.print(b, HEX); Serial.print("->");
  Serial.println(c, HEX);
  */

  return c;
}



// draw a pixel at the x & y coords with a specific color
void  RGBmatrixPanelDue::drawPixel(uint8_t xin, uint8_t yin, uint16_t c) {
  uint16_t index = 0;
  uint8_t old, x, y;
  uint8_t red, green, blue, panel, ysave, ii;

  // extract the 12 bits of color
  red = (c >> 8) & 0xF;
  green = (c >> 4) & 0xF;
  blue = c & 0xF;

  // change to right coords
  //x = (yin - yin%single_matrix_height)/single_matrix_height*single_matrix_width*NX + xin;
  x = xin % WIDTH;
  y = yin % HEIGHT;
  //Serial.print("x = "); Serial.println(x);
  //Serial.print("y = "); Serial.println(y);

  // both top and bottom are stored in same byte
  if (y%single_matrix_height < sections)
    index = y%single_matrix_height;
  else
	index = y%single_matrix_height-sections;
  // now multiply this y by the # of pixels in a row
  index *= single_matrix_width*NX*NY;
  // now, add the x value of the row
  index += x;
  // then multiply by 3 bytes per color (12 bit * High and Low = 24 bit = 3 byte)
  index *= PWMBITS;


  old = matrixbuff[index];
  if (y%single_matrix_height < sections) {
    // we're going to replace the high nybbles only
    // red first!
    matrixbuff[index] &= ~0xF0;  // mask off top 4 bits
    matrixbuff[index] |= (red << 4);
    index++;
    // then green
    matrixbuff[index] &= ~0xF0;  // mask off top 4 bits
    matrixbuff[index] |= (green << 4);
    index++;
    // finally blue
    matrixbuff[index] &= ~0xF0;  // mask off top 4 bits
    matrixbuff[index] |= (blue << 4);
  } else {
    // we're going to replace the low nybbles only
    // red first!
    matrixbuff[index] &= ~0x0F;  // mask off bottom 4 bits
    matrixbuff[index] |= red;
    index++;
    // then green
    matrixbuff[index] &= ~0x0F;  // mask off bottom 4 bits
    matrixbuff[index] |= green;
    index++;
    // finally blue
    matrixbuff[index] &= ~0x0F;  // mask off bottom 4 bits
    matrixbuff[index] |= blue;
  }
}



// bresenham's algorithm - thx wikpedia
void RGBmatrixPanelDue::drawLine(int8_t x0, int8_t y0, int8_t x1, int8_t y1,
		      uint16_t color) {
  //int8_t y1 = y11;
  //int8_t y0 = y00;
  //int8_t x1 = x11;
  //int8_t x0 = x00;
  uint16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  uint16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;}

  for (; x0<=x1; x0++) {
    if (steep) {
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

// draw a rectangle
void RGBmatrixPanelDue::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
		      uint16_t color) {
  drawLine(x, y, x+w-1, y, color);
  drawLine(x, y+h-1, x+w-1, y+h-1, color);

  drawLine(x, y, x, y+h-1, color);
  drawLine(x+w-1, y, x+w-1, y+h-1, color);
}

// Draw a perfectly horizontal line
void RGBmatrixPanelDue::drawHLine(int16_t x, int16_t y,
        int16_t w, uint16_t color) {
    drawLine(x, y, x + w-1, y, color);
}

// Draw a perfectly vertictal line
void RGBmatrixPanelDue::drawVLine(int16_t x, int16_t y,
        int16_t h, uint16_t color) {
    drawLine(x, y, x, y+h-1, color);
}

// fill a rectangle
void RGBmatrixPanelDue::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
		      uint16_t color) {
    for (uint8_t j=y; j<y+h; j++) {
      drawHLine(x, j, w, color);
    }
}

// draw a circle outline
void RGBmatrixPanelDue::drawCircle(uint8_t x0, uint8_t y0, uint8_t r,
			uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0, y0+r, color);
  drawPixel(x0, y0-r, color);
  drawPixel(x0+r, y0, color);
  drawPixel(x0-r, y0, color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);

    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);

  }
}

//Quarter-circle drawer, used to do circles and roundrects
void RGBmatrixPanelDue::drawCircleHelper( int16_t x0, int16_t y0,
        int16_t r, uint8_t cornername, uint16_t color) {
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4) {
            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 - x, y0 - y, color);
        }
    }
}

//Quarter-circle drawer with fill, used for circles and roundrects
void RGBmatrixPanelDue::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
  uint8_t corners, int16_t delta, uint16_t color) {

    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;
    int16_t px    = x;
    int16_t py    = y;

    delta++; // Avoid some +1's in the loop

    while(x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if(x < (y + 1)) {
            if(corners & 1) drawVLine(x0+x, y0-y, 2*y+delta, color);
            if(corners & 2) drawVLine(x0-x, y0-y, 2*y+delta, color);
        }
        if(y != py) {
            if(corners & 1) drawVLine(x0+py, y0-px, 2*px+delta, color);
            if(corners & 2) drawVLine(x0-py, y0-px, 2*px+delta, color);
            py = y;
        }
        px = x;
    }
}

// fill a circle
void RGBmatrixPanelDue::fillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color) {
  drawVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

void RGBmatrixPanelDue::fillScreen(uint16_t c) {
  fillRect(0,0,WIDTH,HEIGHT,c);
}

void RGBmatrixPanelDue::setCursor(uint8_t x, uint8_t y) {
  cursor_x = x;
  cursor_y = y;
}

void RGBmatrixPanelDue::setTextSize(uint8_t s) {
  textsize = s;
}

void RGBmatrixPanelDue::setTextColor(uint16_t c) {
  textcolor = c;
}

// Print unsigned int 8
void RGBmatrixPanelDue::print(char c) {

	if (!gfxFont) { // 'Classic' built-in font	
	  if (c == '\n') {
		cursor_y += textsize * 8;
		cursor_x = 0;
	  } else if (c != '\r') {
		drawChar(cursor_x, cursor_y, c, textcolor, textsize, textsize);
		cursor_x += textsize * 6;
	  }
	}else{
	  if (c == '\n') {
		cursor_x = 0;
		cursor_y += (int16_t)textsize * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
      } else if (c != '\r') {
      uint8_t first = pgm_read_byte(&gfxFont->first);
      if ((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
        GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
        uint8_t w = pgm_read_byte(&glyph->width),
                h = pgm_read_byte(&glyph->height);
        if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
          int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
          drawChar(cursor_x, cursor_y, c, textcolor, textsize, textsize);
        }
        cursor_x +=
            (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize;
      }
    }
	}
}
// Print char array
void RGBmatrixPanelDue::print(const char str[]){
	for (int i=0; i<strlen(str); i++) {
		print(str[i]);
	}
}

// Println without string
void RGBmatrixPanelDue::println(){
  cursor_x = 0;
  if (!gfxFont){
	cursor_y += 8;
  }else{
	cursor_y += setOffset + 2;
  }
}

// Println with char
void RGBmatrixPanelDue::println(char c){
	print((uint8_t) c);
	println();
}

// Println with string
void RGBmatrixPanelDue::println(const char str[]){
  print(str);
  println();
}

// draw a character
void RGBmatrixPanelDue::drawChar(uint8_t x, uint8_t y, unsigned char c,
			      uint16_t color, uint8_t size_x, uint8_t size_y) {
	if (!gfxFont){
	  if ((x >= WIDTH) ||              // Clip right
        (y >= HEIGHT) ||             // Clip bottom
        ((x + 6 * size_x - 1) < 0) || // Clip left
        ((y + 8 * size_y - 1) < 0))   // Clip top
      return;

	  for (uint8_t i =0; i<5; i++ ) {
		uint8_t line = pgm_read_byte(&font[(c*5)+i]);
	//  uint8_t line = font[c*5+i];
		for (uint8_t j = 0; j<8; j++) {
		  if (line & 0x1) {
			if (size_x == 1 && size_y == 1) // default size
				drawPixel(x+i, y+j, color);
			else   // big size
				fillRect(x+i*size_x, y+j*size_y, size_x, size_y, color);	
			}
		  line >>= 1;
		}
	  }
	}else{
  // Character is assumed previously filtered by write() to eliminate
  // newlines, returns, non-printable characters, etc.  Calling
  // drawChar() directly with 'bad' characters of font may cause mayhem!
    c -= (uint8_t)pgm_read_byte(&gfxFont->first);
    GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c);
    uint8_t *bitmap = pgm_read_bitmap_ptr(gfxFont);

    uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
    uint8_t w = pgm_read_byte(&glyph->width), h = pgm_read_byte(&glyph->height);
Serial.print("h = "); Serial.println(h);
//	Serial.print("xstep = "); Serial.println(xstep);	Serial.print("ystep = "); Serial.println(ystep);
    int8_t xo = pgm_read_byte(&glyph->xOffset),
           yo = pgm_read_byte(&glyph->yOffset);
    uint8_t xx, yy, bits = 0, bit = 0;
    int16_t xo16 = 0, yo16 = 0;
  
    if (size_x > 1 || size_y > 1) {
      xo16 = xo;
      yo16 = yo;
    }

    // Todo: Add character clipping here

    // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
    // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
    // has typically been used with the 'classic' font to overwrite old
    // screen contents with new data.  This ONLY works because the
    // characters are a uniform size; it's not a sensible thing to do with
    // proportionally-spaced fonts with glyphs of varying sizes (and that
    // may overlap).  To replace previously-drawn text when using a custom
    // font, use the getTextBounds() function to determine the smallest
    // rectangle encompassing a string, erase the area with fillRect(),
    // then draw new text.  This WILL infortunately 'blink' the text, but
    // is unavoidable.  Drawing 'background' pixels will NOT fix this,
    // only creates a new set of problems.  Have an idea to work around
    // this (a canvas object type for MCUs that can afford the RAM and
    // displays supporting setAddrWindow() and pushColors()), but haven't
    // implemented this yet.
	  if ((x >= WIDTH) ||              // Clip right
        (y >= HEIGHT) ||             // Clip bottom
        ((x + w * size_x - 1) < 0) || // Clip left
        ((y + h * size_y - 1) < 0))   // Clip top
      return;

    for (yy = 0; yy < h; yy++) {
      for (xx = 0; xx < w; xx++) {
        if (!(bit++ & 7)) {
          bits = pgm_read_byte(&bitmap[bo++]);
        }
        if (bits & 0x80) {
          if (size_x == 1 && size_y == 1) {
            drawPixel(x + xo + xx, y + yo + yy, color);
          } else {
            fillRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y,
                          size_x, size_y, color);
          }
        }
        bits <<= 1;
      }
    }
	}
}

/**************************************************************************/
/*!
    @brief Set the font to display when print()ing, either custom or default
    @param  f  The GFXfont object, if NULL use built in 6x8 font
*/
/**************************************************************************/
void RGBmatrixPanelDue::setFont(const GFXfont *f) {
  if (f) {          // Font struct pointer passed in?

	  unsigned char c = (uint8_t)pgm_read_byte(&f->first);
	  GFXglyph *glyph = pgm_read_glyph_ptr(f, c);
	  setOffset = pgm_read_byte(&glyph->height);
	  Serial.print("Offset = "); Serial.println(setOffset);
	
    if (!gfxFont) { // And no current font struct?
      // Switching from classic to new font behavior.
      // Move cursor pos down 6 pixels so it's on baseline.
     cursor_y += setOffset;
    }
  } else if (gfxFont) { // NULL passed.  Current font struct defined?
    // Switching from new to classic font behavior.
    // Move cursor pos up 6 pixels so it's at top-left of char.
     cursor_y -= setOffset;
  }
  gfxFont = (GFXfont *)f;
}


void RGBmatrixPanelDue::dumpMatrix(void) {
  uint8_t i=0;

  do {
    Serial.print("0x");
    if (matrixbuff[i] < 0xF)  Serial.print('0');
    Serial.print(matrixbuff[i], HEX);
    Serial.print(" ");
    i++;
    if (! (i %32) ) Serial.println();
  } while (i != 0);


}



void RGBmatrixPanelDue::writeSection(uint8_t secn, uint8_t *buffptr) {



  //Serial.println('here!');
   //digitalWrite(OE, HIGH);
  uint16_t portCstatus_nonclk = 0x0010; // CLK = low
  uint16_t portCstatus = 0x0010; // OE = HIGH
  uint16_t oeLow = 0x0020;
  portCstatus |= 0x0020; // clk is high here too
  REG_PIOC_ODSR = portCstatus; // set OE, CLK to high

  // set A, B, C pins
  if (secn & 0x1){  // Apin
    portCstatus |= 0x0002;
    portCstatus_nonclk |= 0x0002;
    oeLow |= 0x0002;
  }
  if (secn & 0x2){ // Bpin
    portCstatus |= 0x0004;
    portCstatus_nonclk |= 0x0004;
    oeLow |= 0x0004;
  }
  if (secn & 0x4){ // Cpin
    portCstatus |= 0x0008;
    portCstatus_nonclk |= 0x0008;
    oeLow |= 0x0008;
  }
  if (secn & 0x8){ // Dpin
    portCstatus |= 0x0080;
    portCstatus_nonclk |= 0x0080;
    oeLow |= 0x0080;
  }
  REG_PIOC_ODSR = portCstatus; // set A, B, C pins

  uint8_t  low, high;
  uint16_t out = 0x0000;

  uint8_t i;
  for ( i=0; i<single_matrix_width*NX*NY; i++) {

    out = 0x0000;

    // red
   low = *buffptr++;
   high = low >> 4;
   low &= 0x0F;
   if (low > pwmcounter) out |= 0x0200; // R2, pin 30, PD9
   if (high > pwmcounter) out |= 0x0400; // R1, pin 32, PD10

   // green
   low = *buffptr++;
   high = low >> 4;
   low &= 0x0F;
   if (low > pwmcounter) out |= 0x0008; // G2, pin 28, PD3
   if (high > pwmcounter) out |= 0x0040; // G1, pin 29, PD6

   // blue
   low = *buffptr++;
   high = low >> 4;
   low &= 0x0F;
   if (low > pwmcounter) out |= 0x0002; // B2, pin 26, PD1
   if (high > pwmcounter) out |= 0x0004; // B1, pin 27, PD2


   //digitalWrite(CLK, LOW);
   REG_PIOC_ODSR = portCstatus_nonclk; // set clock to low, OE, A, B, C stay the same

   REG_PIOD_ODSR = out;

   //digitalWrite(CLK, HIGH);
   REG_PIOC_ODSR = portCstatus; // set clock to high, OE, A, B, C stay the same

  }

  // latch it!

  //digitalWrite(LAT, HIGH);
  REG_PIOC_ODSR = (portCstatus |= 0x0040);

  //digitalWrite(LAT, LOW);
  REG_PIOC_ODSR = portCstatus;

  //digitalWrite(OE, LOW);
  REG_PIOC_ODSR = oeLow; //portCstatus; //<< portCstatus;
}



void  RGBmatrixPanelDue::updateDisplay(void) {
  writeSection(scansection, matrixbuff + (PWMBITS*single_matrix_width*NX*NY*scansection));
  scansection++;
  if (scansection == sections) {
    scansection = 0;
    pwmcounter++;
    if (pwmcounter == PWMMAX) { pwmcounter = 0; }
  }
  //Serial.println(scansection);
}

void  RGBmatrixPanelDue::startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency) {
        pmc_set_writeprotect(false);
        pmc_enable_periph_clk((uint32_t)irq);
        TC_Configure(tc, channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1);
        uint32_t rc = VARIANT_MCK/2/frequency; //2 because we selected TIMER_CLOCK1 above
        //TC_Configure(tc, channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4);
        //// VARIANT_MCK = 84000000, I guess timer4 is every 1/4 or something?
        //uint32_t rc = VARIANT_MCK/128/frequency; //128 because we selected TIMER_CLOCK4 above
        TC_SetRA(tc, channel, rc/2); //50% high, 50% low
        TC_SetRC(tc, channel, rc);
        TC_Start(tc, channel);
        tc->TC_CHANNEL[channel].TC_IER=TC_IER_CPCS;
        tc->TC_CHANNEL[channel].TC_IDR=~TC_IER_CPCS;
        NVIC_EnableIRQ(irq);
}
