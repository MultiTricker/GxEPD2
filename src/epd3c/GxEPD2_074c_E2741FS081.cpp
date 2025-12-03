// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from LaskaKit: https://github.com/LaskaKit/laskakit_epaper
// Panel: E2741FS081 (Pervasive Displays 7.4" 3-color BWR)
// Controller: Pervasive Displays iTC driver
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_074c_E2741FS081.h"

GxEPD2_074c_E2741FS081::GxEPD2_074c_E2741FS081(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, HIGH, 50000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _paged = false;
}

void GxEPD2_074c_E2741FS081::clearScreen(uint8_t value)
{
  clearScreen(value, 0xFF);
}

void GxEPD2_074c_E2741FS081::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  refresh();
}

void GxEPD2_074c_E2741FS081::writeScreenBuffer(uint8_t value)
{
  writeScreenBuffer(value, 0xFF);
}

void GxEPD2_074c_E2741FS081::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  if (!_init_display_done) _InitDisplay();
  // Send configuration commands
  uint8_t data1[] = {0x00, 0x3b, 0x00, 0x00, 0x1f, 0x03};
  _sendIndexData(0x13, data1, 6); // DUW
  uint8_t data2[] = {0x00, 0x3b, 0x00, 0xc9};
  _sendIndexData(0x90, data2, 4); // DRFW
  uint8_t data3[] = {0x3b, 0x00, 0x14};
  _sendIndexData(0x12, data3, 3); // RAM_RW
  // Send black frame (frame1) - 0x00 for black, 0xFF for white
  _writeCommand(0x10);
  _startTransfer();
  for (uint32_t i = 0; i < _frame_size; i++)
  {
    _transfer(black_value);
  }
  _endTransfer();
  // Send RAM_RW again for second frame
  _sendIndexData(0x12, data3, 3);
  // Send color frame (frame2) - 0x00 for red, 0xFF for no-red
  _writeCommand(0x11);
  _startTransfer();
  for (uint32_t i = 0; i < _frame_size; i++)
  {
    _transfer(color_value);
  }
  _endTransfer();
  _initial_write = false;
}

void GxEPD2_074c_E2741FS081::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, (const uint8_t*)0, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_074c_E2741FS081::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!black && !color) return;
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  
  int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
  if ((w <= 0) || (h <= 0)) return;
  
  // Send configuration commands
  uint8_t data1[] = {0x00, 0x3b, 0x00, 0x00, 0x1f, 0x03};
  _sendIndexData(0x13, data1, 6); // DUW
  uint8_t data2[] = {0x00, 0x3b, 0x00, 0xc9};
  _sendIndexData(0x90, data2, 4); // DRFW
  uint8_t data3[] = {0x3b, 0x00, 0x14};
  _sendIndexData(0x12, data3, 3); // RAM_RW
  
  // Send black frame (frame1)
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t data = 0xFF; // white
      if (black)
      {
        if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
        {
          uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&black[idx]);
#else
            data = black[idx];
#endif
          }
          else
          {
            data = black[idx];
          }
          if (invert) data = ~data;
        }
      }
      _transfer(data);
    }
  }
  _endTransfer();
  
  // Send RAM_RW again for second frame
  _sendIndexData(0x12, data3, 3);
  
  // Send color frame (frame2)
  _writeCommand(0x11);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t data = 0xFF; // no red
      if (color)
      {
        if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
        {
          uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&color[idx]);
#else
            data = color[idx];
#endif
          }
          else
          {
            data = color[idx];
          }
          if (invert) data = ~data;
        }
      }
      _transfer(data);
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_074c_E2741FS081::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, (const uint8_t*)0, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_074c_E2741FS081::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!black && !color) return;
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  w += x % 8; // adjust for byte boundary of x
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  
  // Send configuration commands
  uint8_t data1[] = {0x00, 0x3b, 0x00, 0x00, 0x1f, 0x03};
  _sendIndexData(0x13, data1, 6); // DUW
  uint8_t data2[] = {0x00, 0x3b, 0x00, 0xc9};
  _sendIndexData(0x90, data2, 4); // DRFW
  uint8_t data3[] = {0x3b, 0x00, 0x14};
  _sendIndexData(0x12, data3, 3); // RAM_RW
  
  // Send black frame
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t data = 0xFF;
      if (black)
      {
        if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
        {
          uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&black[idx]);
#else
            data = black[idx];
#endif
          }
          else
          {
            data = black[idx];
          }
          if (invert) data = ~data;
        }
      }
      _transfer(data);
    }
  }
  _endTransfer();
  
  // Send RAM_RW again
  _sendIndexData(0x12, data3, 3);
  
  // Send color frame
  _writeCommand(0x11);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t data = 0xFF;
      if (color)
      {
        if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
        {
          uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&color[idx]);
#else
            data = color[idx];
#endif
          }
          else
          {
            data = color[idx];
          }
          if (invert) data = ~data;
        }
      }
      _transfer(data);
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_074c_E2741FS081::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  // For this display, native format is same as regular image format (1 bit per pixel per plane)
  writeImage(data1, data2, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_074c_E2741FS081::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_074c_E2741FS081::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_074c_E2741FS081::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_074c_E2741FS081::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_074c_E2741FS081::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_074c_E2741FS081::refresh(bool partial_update_mode)
{
  // COG initialization
  _cogInitialization();
  // DC/DC soft-start
  _dcDcSoftStart();
  // Display refresh and power down
  _displayRefreshAndPowerDown();
}

void GxEPD2_074c_E2741FS081::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  refresh(false); // no partial update support
}

void GxEPD2_074c_E2741FS081::powerOff()
{
  _PowerOff();
}

void GxEPD2_074c_E2741FS081::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    digitalWrite(_rst, LOW);
    _hibernating = true;
    _init_display_done = false;
  }
}

void GxEPD2_074c_E2741FS081::setPaged()
{
  _paged = true;
  _InitDisplay();
}

void GxEPD2_074c_E2741FS081::_sendIndexData(uint8_t index, const uint8_t* data, uint16_t len)
{
  _writeCommand(index);
  for (uint16_t i = 0; i < len; i++)
  {
    _writeData(data[i]);
  }
}

void GxEPD2_074c_E2741FS081::_PowerOn()
{
  if (!_power_is_on)
  {
    // Power on sequence is handled in _InitDisplay
  }
  _power_is_on = true;
}

void GxEPD2_074c_E2741FS081::_PowerOff()
{
  if (_power_is_on)
  {
    // DCDC off sequence
    uint8_t data1[] = {0x7f};
    _sendIndexData(0x09, data1, 1);
    uint8_t data2[] = {0x7d};
    _sendIndexData(0x05, data2, 1);
    uint8_t data3[] = {0x00};
    _sendIndexData(0x09, data3, 1);
    delay(200);
    _waitWhileBusy("_PowerOff", power_off_time);
    // Set pins low
    if (_dc >= 0) digitalWrite(_dc, LOW);
    if (_cs >= 0) digitalWrite(_cs, HIGH);
    if (_rst >= 0) digitalWrite(_rst, LOW);
  }
  _power_is_on = false;
  _init_display_done = false;
}

void GxEPD2_074c_E2741FS081::_InitDisplay()
{
  if (_hibernating || _initial_write)
  {
    if (_rst >= 0)
    {
      digitalWrite(_rst, HIGH);
      delay(20);
      digitalWrite(_rst, LOW);
      delay(200);
      digitalWrite(_rst, HIGH);
      delay(50);
    }
    _hibernating = false;
  }
  _power_is_on = true;
  _init_display_done = true;
}

void GxEPD2_074c_E2741FS081::_cogInitialization()
{
  // COG initialization sequence from reference code
  uint8_t data5[] = {0x00};
  
  uint8_t data4[] = {0x7d};
  _sendIndexData(0x05, data4, 1);
  delay(1);
  
  _sendIndexData(0x05, data5, 1);
  delay(1);
  
  uint8_t data6[] = {0x3f};
  _sendIndexData(0xc2, data6, 1);
  delay(1);
  
  uint8_t data7[] = {0x00};
  _sendIndexData(0xd8, data7, 1); // MS_SYNC
  
  uint8_t data8[] = {0x00};
  _sendIndexData(0xd6, data8, 1); // BVSS
  
  uint8_t data9[] = {0x10};
  _sendIndexData(0xa7, data9, 1);
  delay(1);
  
  _sendIndexData(0xa7, data5, 1);
  delay(1);
  
  uint8_t data10[] = {0x00, 0x01};
  _sendIndexData(0x03, data10, 2); // OSC
  
  _sendIndexData(0x44, data5, 1);
  
  uint8_t data11[] = {0x80};
  _sendIndexData(0x45, data11, 1);
  
  _sendIndexData(0xa7, data9, 1);
  delay(1);
  
  _sendIndexData(0xa7, data7, 1);
  delay(1);
  
  uint8_t data12[] = {0x06};
  _sendIndexData(0x44, data12, 1);
  
  uint8_t data13[] = {0x82}; // Temperature 0x82@25C
  _sendIndexData(0x45, data13, 1);
  
  _sendIndexData(0xa7, data9, 1);
  delay(1);
  
  _sendIndexData(0xa7, data7, 1);
  delay(1);
  
  uint8_t data14[] = {0x25};
  _sendIndexData(0x60, data14, 1); // TCON
  
  uint8_t data15[] = {0x00};
  _sendIndexData(0x61, data15, 1); // STV_DIR
  
  uint8_t data16[] = {0x00};
  _sendIndexData(0x01, data16, 1); // DCTL
  
  uint8_t data17[] = {0x00};
  _sendIndexData(0x02, data17, 1); // VCOM
}

void GxEPD2_074c_E2741FS081::_dcDcSoftStart()
{
  // DC/DC soft-start sequence from reference code
  uint8_t Index51_data[] = {0x50, 0x01, 0x0a, 0x01};
  uint8_t Index09_data[] = {0x1f, 0x9f, 0x7f, 0xff};
  
  _sendIndexData(0x51, &Index51_data[0], 2); // 0x51-0x50,0x01
  
  // Stage 1
  for (int value = 1; value <= 4; value++)
  {
    _sendIndexData(0x09, &Index09_data[0], 1);
    Index51_data[1] = value;
    _sendIndexData(0x51, &Index51_data[0], 2);
    _sendIndexData(0x09, &Index09_data[1], 1);
    delay(1);
  }
  
  // Stage 2
  for (int value = 1; value <= 10; value++)
  {
    _sendIndexData(0x09, &Index09_data[0], 1);
    Index51_data[3] = value;
    _sendIndexData(0x51, &Index51_data[2], 2);
    _sendIndexData(0x09, &Index09_data[1], 1);
    delay(1);
  }
  
  // Stage 3
  for (int value = 3; value <= 10; value++)
  {
    _sendIndexData(0x09, &Index09_data[2], 1);
    Index51_data[3] = value;
    _sendIndexData(0x51, &Index51_data[2], 2);
    _sendIndexData(0x09, &Index09_data[3], 1);
    delay(1);
  }
  
  // Stage 4
  for (int value = 9; value >= 2; value--)
  {
    _sendIndexData(0x09, &Index09_data[2], 1);
    Index51_data[2] = value;
    Index51_data[3] = 0x0A;
    _sendIndexData(0x51, &Index51_data[2], 2);
    _sendIndexData(0x09, &Index09_data[3], 1);
    delay(1);
  }
  
  _sendIndexData(0x09, &Index09_data[3], 1);
  delay(1);
}

void GxEPD2_074c_E2741FS081::_displayRefreshAndPowerDown()
{
  // Wait for BUSY pin to go high
  _waitWhileBusy("_displayRefreshAndPowerDown wait1", power_on_time);
  
  // Send display refresh command
  uint8_t data18[] = {0x3c};
  _sendIndexData(0x15, data18, 1); // Display Refresh
  delay(1);
  
  // Wait for refresh to complete
  _waitWhileBusy("_displayRefreshAndPowerDown refresh", full_refresh_time);
  
  // DCDC off sequence
  uint8_t data19[] = {0x7f};
  _sendIndexData(0x09, data19, 1);
  uint8_t data20[] = {0x7d};
  _sendIndexData(0x05, data20, 1);
  uint8_t data21[] = {0x00};
  _sendIndexData(0x09, data21, 1);
  delay(200);
  
  _waitWhileBusy("_displayRefreshAndPowerDown powerdown", power_off_time);
  
  // Set pins to low power state
  if (_dc >= 0) digitalWrite(_dc, LOW);
  if (_cs >= 0) digitalWrite(_cs, HIGH);
  if (_rst >= 0) digitalWrite(_rst, LOW);
  
  _power_is_on = false;
  _init_display_done = false;
}
