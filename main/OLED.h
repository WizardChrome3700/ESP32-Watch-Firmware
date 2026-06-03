#ifndef OLED_H
#define OLED_H

#include <SPI.h>
#include <pgmspace.h>

class OLED {
  public:
  OLED(uint8_t sck, uint8_t din, uint8_t cs, uint8_t dc, uint8_t res) {
    this->sck = sck;
    this->din = din;
    this->cs = cs;
    this->dc = dc;
    this->res = res;
  }

  void sh1106_init() {
    // Initialize pins
    pinMode(cs, OUTPUT);      // CS pin
    pinMode(dc, OUTPUT);      // DC pin
    pinMode(res, OUTPUT);      // RES pin
    
    // Set initial states
    digitalWrite(cs, HIGH);   // CS HIGH (inactive)
    digitalWrite(dc, HIGH);   // DC HIGH
    digitalWrite(res, HIGH);   // RES HIGH
    
    // Reset OLED
    digitalWrite(res, LOW);
    delay(50);
    digitalWrite(res, HIGH);
    delay(50);
    
    // Initialize SPI
    SPI.begin(sck, -1, din, cs);  // SCK=18, MOSI=19, CS=22
    SPI.setFrequency(4000000);  // 4MHz for stability
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    
    // SH1106 Initialization Sequence
    this->sh1106_command(0xAE);       // Display OFF
    this->sh1106_command(0xD5);       // Set display clock divide
    this->sh1106_command(0x80);
    this->sh1106_command(0xA8);       // Set multiplex ratio
    this->sh1106_command(0x3F);       // 64 lines
    this->sh1106_command(0xD3);       // Set display offset
    this->sh1106_command(0x00);       // No offset
    this->sh1106_command(0x40);       // Set display start line 0
    this->sh1106_command(0x8D);       // Charge pump setting
    this->sh1106_command(0x14);       // Enable charge pump
    this->sh1106_command(0xC0);       // COM scan direction (normal)
    this->sh1106_command(0xDA);       // COM pins config
    this->sh1106_command(0x12);
    this->sh1106_command(0x81);       // Set contrast
    this->sh1106_command(0x7F);
    this->sh1106_command(0xD9);       // Pre-charge period
    this->sh1106_command(0xF1);
    this->sh1106_command(0xDB);       // VCOMH deselect level
    this->sh1106_command(0x40);
    this->sh1106_command(0xA4);       // Entire display ON
    this->sh1106_command(0xA6);       // Normal display
    this->sh1106_command(0xA0);       // Segment re-map (normal)
    this->sh1106_command(0xAF);       // Display ON
  }

  void updateDisplay() {
    // Update all 8 pages (0-7)
    for (uint8_t page = 0; page < 8; page++) {
      sh1106_command(0xB0 + page);     // Set page address
      sh1106_command(0x02);            // Lower column address = 2
      sh1106_command(0x10);            // Higher column address = 0
      
      // Send 128 bytes for this page
      for (uint8_t col = 0; col < 128; col++) {
        uint16_t bufferIndex = col + 2 + page * 132;
        sh1106_data(displayBuffer[bufferIndex]);
      }
    }
  }

  // ==================== GRAPHICS FUNCTIONS ====================

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (true) {
      drawPixel(x0, y0, color);
      if (x0 == x1 && y0 == y1) break;
      int16_t e2 = 2 * err;
      if (e2 > -dy) {
        err -= dy;
        x0 += sx;
      }
      if (e2 < dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  void drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color) {
    drawLine(x, y, x + w, y, color);
    drawLine(x, y + h, x + w, y + h, color);
    drawLine(x, y, x, y + h, color);
    drawLine(x + w, y, x + w, y + h, color);
  }

  void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color) {
    for (int16_t i = x; i < x + w; i++) {
      for (int16_t j = y; j < y + h; j++) {
        drawPixel(i, j, color);
      }
    }
  }

  void drawCircle(int16_t x0, int16_t y0, uint8_t r, uint8_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);
    
    while (x < y) {
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

  // ==================== TEXT FUNCTIONS ====================

  uint8_t getFontByte(char c, uint8_t col) {
    if (c < 32 || c > 126) return 0;
    return pgm_read_byte(&font5x7[c - 32][col]);
  }

  void drawChar(uint8_t x, uint8_t y, char c, uint8_t color) {
    if (c < 32 || c > 126) return;
    
    for (uint8_t col = 0; col < 5; col++) {
      uint8_t colData = getFontByte(c, col);
      for (uint8_t bit = 0; bit < 7; bit++) {
        if (colData & (1 << bit)) {
          drawPixel(x + col, y + bit, color);
        }
      }
    }
  }

  void drawString(uint8_t x, uint8_t y, const char* text, uint8_t color) {
    uint8_t cursorX = x;
    while (*text) {
      drawChar(cursorX, y, *text, color);
      cursorX += 6;
      text++;
    }
  }

  void drawStringCentered(uint8_t y, const char* text, uint8_t color) {
    uint8_t textLength = strlen(text);
    uint8_t textWidth = textLength * 6 - 1;
    uint8_t x = (128 - textWidth) / 2;
    drawString(x, y, text, color);
  }

  void drawStringRight(uint8_t y, const char* text, uint8_t color) {
    uint8_t textLength = strlen(text);
    uint8_t textWidth = textLength * 6 - 1;
    uint8_t x = 128 - textWidth - 2;
    drawString(x, y, text, color);
  }

  void drawStringWrapped(uint8_t x, uint8_t y, const char* text, uint8_t color) {
    uint8_t cursorX = x;
    uint8_t cursorY = y;
    while (*text) {
      if (*text == '\n') {
        cursorX = x;
        cursorY += 8;
      } else {
        if (cursorX > 122) {
          cursorX = x;
          cursorY += 8;
          if (cursorY > 56) break;
        }
        drawChar(cursorX, cursorY, *text, color);
        cursorX += 6;
      }
      text++;
    }
  }

  void clearBuffer() {
    memset(displayBuffer, 0, sizeof(displayBuffer));
  }

  private:
  uint8_t cs;
  uint8_t dc;
  uint8_t sck;
  uint8_t din;
  uint8_t res;
  uint8_t displayBuffer[1056]; // 132 columns × 8 pages
  static const uint8_t PROGMEM font5x7[95][5];

  void sh1106_command(uint8_t cmd) {
    digitalWrite(dc, LOW);    // DC LOW for command
    digitalWrite(cs, LOW);    // CS LOW
    SPI.transfer(cmd);
    digitalWrite(cs, HIGH);   // CS HIGH
  }

  void sh1106_data(uint8_t data) {
    digitalWrite(dc, HIGH);   // DC HIGH for data
    digitalWrite(cs, LOW);    // CS LOW
    SPI.transfer(data);
    digitalWrite(cs, HIGH);   // CS HIGH
  }

  void drawPixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    
    uint8_t page = y / 8;
    uint8_t bitPosition = y % 8;
    uint16_t byteIndex = (x + 2) + page * 132;
    
    if (color) {
      displayBuffer[byteIndex] |= (1 << bitPosition);
    } else {
      displayBuffer[byteIndex] &= ~(1 << bitPosition);
    }
  }
};

// 5x7 Font in PROGMEM (Flash memory)
const uint8_t PROGMEM OLED::font5x7[95][5] = {
  {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
  {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
  {0x00, 0x07, 0x00, 0x07, 0x00}, // "
  {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
  {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
  {0x23, 0x13, 0x08, 0x64, 0x62}, // %
  {0x36, 0x49, 0x55, 0x22, 0x50}, // &
  {0x00, 0x05, 0x03, 0x00, 0x00}, // '
  {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
  {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
  {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
  {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
  {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
  {0x08, 0x08, 0x08, 0x08, 0x08}, // -
  {0x00, 0x60, 0x60, 0x00, 0x00}, // .
  {0x20, 0x10, 0x08, 0x04, 0x02}, // /
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
  {0x00, 0x36, 0x36, 0x00, 0x00}, // :
  {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
  {0x08, 0x14, 0x22, 0x41, 0x00}, // <
  {0x14, 0x14, 0x14, 0x14, 0x14}, // =
  {0x00, 0x41, 0x22, 0x14, 0x08}, // >
  {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
  {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
  {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
  {0x46, 0x49, 0x49, 0x49, 0x31}, // S
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
  {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
  {0x63, 0x14, 0x08, 0x14, 0x63}, // X
  {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
  {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
  {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
  {0x02, 0x04, 0x08, 0x10, 0x20}, // Backslash
  {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
  {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
  {0x40, 0x40, 0x40, 0x40, 0x40}, // _
  {0x00, 0x01, 0x02, 0x04, 0x00}, // `
  {0x20, 0x54, 0x54, 0x54, 0x78}, // a
  {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
  {0x38, 0x44, 0x44, 0x44, 0x20}, // c
  {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
  {0x38, 0x54, 0x54, 0x54, 0x18}, // e
  {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
  {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
  {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
  {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
  {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
  {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
  {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
  {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
  {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
  {0x38, 0x44, 0x44, 0x44, 0x38}, // o
  {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
  {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
  {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
  {0x48, 0x54, 0x54, 0x54, 0x20}, // s
  {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
  {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
  {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
  {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
  {0x44, 0x28, 0x10, 0x28, 0x44}, // x
  {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
  {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
  {0x00, 0x08, 0x36, 0x41, 0x00}, // {
  {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
  {0x00, 0x41, 0x36, 0x08, 0x00}, // }
  {0x10, 0x08, 0x08, 0x10, 0x08}, // ~
};

#endif