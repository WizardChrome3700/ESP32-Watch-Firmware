#ifndef SSD1306_H
#define SSD1306_H

#include "SPI.h"
#include <cstring>
// #include <pgmspace.h>

/**
 * @class SSD1306
 * @brief Driver application header for SSD1306 128x64 OLED display controller.
 * @details It is used to control the OLED display via 4-Wire SPI.
 */
class SSD1306 {
	public:
	/**
	 * Constructor for SSD1306 class
	 */
	SSD1306(uint8_t sck, uint8_t din, uint8_t cs, uint8_t dc, uint8_t res) {
		this->sck = sck;
		this->din = din;
		this->cs = cs;
		this->dc = dc;
		this->res = res;
		// SSD1306 supports high-speed SPI, 4MHz is extremely safe
		_spiSettings = SPISettings(4000000, MSBFIRST, SPI_MODE0);
		_spiSettings.spics_io_num = cs;
		this->displayBuffer = nullptr;
	}

  ~SSD1306() {
    if (this->displayBuffer != nullptr) {
      free(this->displayBuffer); // Safely returns the 1024 bytes back to the OS heap!
      this->displayBuffer = nullptr;
    }
  }

  /**
   * Initialises the OLED in the following order:
   * - Reset the OLED controller
   * - Allocate 1024 bytes of DMA-capable RAM
   * - Configure charge pumps, multiplexing, and memory addressing modes
   */
  void ssd1306_init() {
    // Initialize pins
    pinMode(cs, OUTPUT);      
    pinMode(dc, OUTPUT);      
    pinMode(res, OUTPUT);     
    
    // Set initial states
    digitalWrite(cs, HIGH);   
    digitalWrite(dc, HIGH);   
    digitalWrite(res, HIGH);  
    
    // Hardware Reset Sequence
    digitalWrite(res, LOW);
    vTaskDelay(pdMS_TO_TICKS(50));
    digitalWrite(res, HIGH);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Initialize SPI
    SPI.begin(sck, -1, din, cs);  
    
    if (this->displayBuffer == nullptr) {
      // Changed from 132x8 (SH1106) to exactly 128x8 (SSD1306)
      this->displayBuffer = (uint8_t*) heap_caps_malloc(128 * 8, MALLOC_CAP_DMA);
      memset(this->displayBuffer, 0, 128 * 8); 
    }
    
    // SSD1306 Initialization Sequence
    this->ssd1306_command(0xAE);       // Display OFF
    this->ssd1306_command(0xD5);       // Set display clock divide
    this->ssd1306_command(0x80);       // Recommended ratio
    this->ssd1306_command(0xA8);       // Set multiplex ratio
    this->ssd1306_command(0x3F);       // 64 lines
    this->ssd1306_command(0xD3);       // Set display offset
    this->ssd1306_command(0x00);       // No offset
    this->ssd1306_command(0x40 | 0x0); // Set display start line to 0
    
    this->ssd1306_command(0x8D);       // Charge pump setting
    this->ssd1306_command(0x14);       // Enable charge pump
    
    this->ssd1306_command(0x20);       // Memory Addressing Mode
    this->ssd1306_command(0x02);       // Set Page Addressing Mode (to match original loop)
    
    this->ssd1306_command(0xA1);       // Segment re-map (mirrors X axis)
    this->ssd1306_command(0xC8);       // COM scan direction (mirrors Y axis)
    
    this->ssd1306_command(0xDA);       // COM pins config
    this->ssd1306_command(0x12);
    this->ssd1306_command(0x81);       // Set contrast
    this->ssd1306_command(0x7F);       // Default contrast
    this->ssd1306_command(0xD9);       // Pre-charge period
    this->ssd1306_command(0xF1);
    this->ssd1306_command(0xDB);       // VCOMH deselect level
    this->ssd1306_command(0x40);
    
    this->ssd1306_command(0xA4);       // Entire display ON (Resume RAM content)
    this->ssd1306_command(0xA6);       // Normal display (not inverted)
    this->ssd1306_command(0xAF);       // Display ON
  }

  void ssd1306_shutdown() {
    this->ssd1306_command(0xAE);       // Display OFF
  }

  /**
   * Pushes the DMA-allocated RAM buffer to the screen using bulk hardware bursts.
   * This is drastically faster than single-byte loop transfers.
   */
	void updateDisplay() {
	// Update all 8 pages (0-7)
		for (uint8_t page = 0; page < 8; page++) {
			
			// 1. Send the positioning commands
			ssd1306_command(0xB0 + page);    
			ssd1306_command(0x00);         
			ssd1306_command(0x10);           
			
			// 2. Lock the bus into Data Mode
			SPI.beginTransaction(_spiSettings);
			digitalWrite(dc, HIGH);   
			digitalWrite(cs, LOW); 
			SPI.transfer((displayBuffer + 128*page), 128);
			
			// 4. Release the bus
			digitalWrite(cs, HIGH);   
			SPI.endTransaction();
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

  void drawFrame(uint8_t x0, uint8_t y0, uint8_t* frame, uint16_t width, uint16_t height) {
    for(uint16_t col = 0; col < (width)*(height)/8; col++) {
      uint16_t deltaX = col % (width);
      uint16_t deltaY = (col / (width)) * 8;
      uint8_t colData = frame[col];
      for(uint8_t bit = 0; bit < 8; bit++) {
        if(colData & (1 << bit)) {
          drawPixel(x0 + deltaX, y0 + deltaY + bit, 1);
        }
      }
    }
  }

  // ==================== TEXT FUNCTIONS ====================

  /**
   * It is uesed to obtain the byte from the 8x6 memory unit table.
   */
  uint8_t getFontByte(char c, uint8_t col) {
    if (c < 32 || c > 126) return 0;
    return SSD1306::font5x7[c - 32][col];
  }

  /**
   * It is used to draw the byte in the 8x6 memory unit format.
   */
  void drawChar(uint8_t x, uint8_t y, char c, int8_t color) {
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

  /**
   * It writes strings in left aligned format.
   */
  void drawString(uint8_t x, uint8_t y, const char* text, uint8_t color) {
    uint8_t cursorX = x;
    while (*text) {
      drawChar(cursorX, y, *text, color);
      cursorX += 6;
      text++;
    }
  }

  /**
   * It writes strings in centered format.
   */
  void drawStringCentered(uint8_t y, const char* text, uint8_t color) {
    uint8_t textLength = strlen(text);
    uint8_t textWidth = textLength * 6 - 1;
    uint8_t x = (128 - textWidth) / 2;
    drawString(x, y, text, color);
  }

  /**
   * It writes string in right aligned format.
   */
  void drawStringRight(uint8_t y, const char* text, uint8_t color) {
    uint8_t textLength = strlen(text);
    uint8_t textWidth = textLength * 6 - 1;
    uint8_t x = 128 - textWidth - 2;
    drawString(x, y, text, color);
  }

  /**
   * It writes string in screen wrapped format.
   */
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

  /**
   * It clears the display buffer stored in RAM.
   */
  void clearBuffer() {
    memset(this->displayBuffer, 0, 128 * 8); // Clear strictly 1024 bytes
  }

  private:
  uint8_t cs;
  uint8_t dc;
  uint8_t sck;
  uint8_t din;
  uint8_t res;
  uint8_t* displayBuffer;
  SPISettings _spiSettings;
  inline static constexpr uint8_t font5x7[95][5] = {
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

  void ssd1306_command(uint8_t cmd) {
    SPI.beginTransaction(_spiSettings);
    digitalWrite(dc, LOW);    // DC LOW for command
    digitalWrite(cs, LOW);    // CS LOW
    SPI.transfer(cmd);
    digitalWrite(cs, HIGH);   // CS HIGH
    SPI.endTransaction();
    delayMicroseconds(2);
  }

  void ssd1306_data(uint8_t data) {
    SPI.beginTransaction(_spiSettings);
    digitalWrite(dc, HIGH);   // DC HIGH for data
    digitalWrite(cs, LOW);    // CS LOW
    SPI.transfer(data);
    digitalWrite(cs, HIGH);   // CS HIGH
    SPI.endTransaction();
    delayMicroseconds(2);
  }

  void drawPixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    
    uint8_t page = y / 8;
    uint8_t bitPosition = y % 8;
    uint16_t byteIndex = x + (page * 128); // Removed SH1106's +2 offset
    
    if (color) {
      displayBuffer[byteIndex] |= (1 << bitPosition);
    } else {
      displayBuffer[byteIndex] &= ~(1 << bitPosition);
    }
  }
};

#endif