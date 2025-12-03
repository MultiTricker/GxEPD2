// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from LaskaKit: https://github.com/LaskaKit/Testcode_examples/tree/main/Displays/E-Paper/7-50/GDEM075F52_ESP32
// Panel: GDEM075F52 : https://www.good-display.com/product/465.html
// Controller: similar to HX8717, see panel specs for command list subset
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_750c_GDEM075F52.h"

GxEPD2_750c_GDEM075F52::GxEPD2_750c_GDEM075F52(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 50000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _paged = false;
}

void GxEPD2_750c_GDEM075F52::clearScreen(uint8_t value)
{
  clearScreen(value, 0xFF);
}

void GxEPD2_750c_GDEM075F52::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  refresh();
}

void GxEPD2_750c_GDEM075F52::writeScreenBuffer(uint8_t value)
{
  writeScreenBuffer(value, 0xFF);
}

void GxEPD2_750c_GDEM075F52::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  //Serial.println("writeScreenBuffer");
  if (!_init_display_done) _InitDisplay();
  _writeCommand(0x10);
  _startTransfer();
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 4; i++)
  {
    _transfer(0xFF == black_value ? 0x55 : 0x00);
  }
  _endTransfer();
  _initial_write = false; // initial full screen buffer clean done
}

void GxEPD2_750c_GDEM075F52::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
  {
    //Serial.println("paged");
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 8; i++)
    {
      uint8_t data = bitmap[i];
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                        (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
        data <<= 4;
        _transfer(data2);
      }
    }
    _endTransfer();
    if (y + h == HEIGHT) // last page
    {
      //Serial.println("paged ended");
      _paged = false;
    }
  }
  else
  {
    _paged = false;
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    if ((w <= 0) || (h <= 0)) return;
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
      {
        uint8_t data = 0xFF;
        if ((j >= x) && (j <= x + w) && (i >= y) && (i < y + h))
        {
          uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&bitmap[idx]);
#else
            data = bitmap[idx];
#endif
          }
          else
          {
            data = bitmap[idx];
          }
          if (invert) data = ~data;
        }
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                          (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
          data <<= 4;
          _transfer(data2);
        }
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750c_GDEM075F52::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!black && !color) return;
  if (!color) return writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  //Serial.print("writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
  {
    //Serial.println("paged");
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 8; i++)
    {
      uint8_t black_data = black[i];
      uint8_t color_data = color[i];
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t out_data = 0x00;
        for (int16_t l = 0; l < 4; l++)
        {
          out_data <<= 2;
          if (!(color_data & 0x80)) out_data |= 0x02; // yellow
          else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
          black_data <<= 1;
          color_data <<= 1;
        }
        _transfer(out_data);
      }
    }
    _endTransfer();
    if (y + h == HEIGHT) // last page
    {
      //Serial.println("paged ended");
      _paged = false;
    }
  }
  else
  {
    _paged = false;
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    if ((w <= 0) || (h <= 0)) return;
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
      {
        uint8_t black_data = 0xFF, color_data = 0xFF;
        if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
        {
          uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            black_data = pgm_read_byte(&black[idx]);
            color_data = pgm_read_byte(&color[idx]);
#else
            black_data = black[idx];
            color_data = color[idx];
#endif
          }
          else
          {
            black_data = black[idx];
            color_data = color[idx];
          }
          if (invert)
          {
            black_data = ~black_data;
            color_data = ~color_data;
          }
        }
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t out_data = 0x00;
          for (int16_t l = 0; l < 4; l++)
          {
            out_data <<= 2;
            if (!(color_data & 0x80)) out_data |= 0x02; // yellow
            else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
            black_data <<= 1;
            color_data <<= 1;
          }
          _transfer(out_data);
        }
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750c_GDEM075F52::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImagePart("); Serial.print(x_part); Serial.print(", "); Serial.print(y_part); Serial.print(", ");
  //Serial.print(w_bitmap); Serial.print(", "); Serial.print(h_bitmap); Serial.print(", ");
  //Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
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
  // make x, w multiple of 8
  w += x % 8; // adjust for byte boundary of x
  x -= x % 8; // byte boundary ox
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
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t data = 0xFF;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
#else
          data = bitmap[idx];
#endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
      }
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                        (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
        data <<= 4;
        _transfer(data2);
      }
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750c_GDEM075F52::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImagePart("); Serial.print(x_part); Serial.print(", "); Serial.print(y_part); Serial.print(", ");
  //Serial.print(w_bitmap); Serial.print(", "); Serial.print(h_bitmap); Serial.print(", ");
  //Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  if (!black && !color) return;
  if (!color) return writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  // make x, w multiple of 8
  w += x % 8; // adjust for byte boundary of x
  x -= x % 8; // byte boundary ox
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
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t black_data = 0xFF, color_data = 0xFF;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          black_data = pgm_read_byte(&black[idx]);
          color_data = pgm_read_byte(&color[idx]);
#else
          black_data = black[idx];
          color_data = color[idx];
#endif
        }
        else
        {
          black_data = black[idx];
          color_data = color[idx];
        }
        if (invert)
        {
          black_data = ~black_data;
          color_data = ~color_data;
        }
      }
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t out_data = 0x00;
        for (int16_t l = 0; l < 4; l++)
        {
          out_data <<= 2;
          if (!(color_data & 0x80)) out_data |= 0x02; // yellow
          else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
          black_data <<= 1;
          color_data <<= 1;
        }
        _transfer(out_data);
      }
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750c_GDEM075F52::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!data1) return;
  //Serial.print("writeNative("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
  {
    //Serial.println("paged");
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 4; i++)
    {
      uint8_t data = data1[i];
      _transfer(data);
    }
    _endTransfer();
    if (y + h == HEIGHT) // last page
    {
      //Serial.println("paged ended");
      _paged = false;
    }
  }
  else
  {
    //Serial.println("not paged");
    _paged = false;
    int16_t wb = (w + 3) / 4; // width bytes, bitmaps are padded
    x -= x % 4; // byte boundary
    w = wb * 4; // byte boundary
    if ((w <= 0) || (h <= 0)) return;
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 4)
      {
        uint8_t data = 0x55;
        if (data1)
        {
          if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
          {
            uint32_t idx = mirror_y ? (j - x) / 4 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 4 + uint32_t(i - y) * wb;
            if (pgm)
            {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
              data = pgm_read_byte(&data1[idx]);
#else
              data = data1[idx];
#endif
            }
            else
            {
              data = data1[idx];
            }
            if (invert) data = ~data;
          }
        }
        _transfer(data);
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750c_GDEM075F52::writeNativePart(const uint8_t* data1, const uint8_t* data2, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!data1) return;
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 3) / 4; // width bytes, bitmaps are padded
  x_part -= x_part % 4; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  w += x % 4; // adjust for byte boundary of x
  x -= x % 4; // byte boundary
  w = 4 * ((w + 3) / 4); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 4)
    {
      uint8_t data = 0x55;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 4 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 4 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&data1[idx]);
#else
          data = data1[idx];
#endif
        }
        else
        {
          data = data1[idx];
        }
        if (invert) data = ~data;
      }
      _transfer(data);
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750c_GDEM075F52::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_750c_GDEM075F52::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_750c_GDEM075F52::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_750c_GDEM075F52::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_750c_GDEM075F52::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_750c_GDEM075F52::refresh(bool partial_update_mode)
{
  _refresh(partial_update_mode);
}

void GxEPD2_750c_GDEM075F52::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  _refresh(false);
}

void GxEPD2_750c_GDEM075F52::powerOff()
{
  _PowerOff();
}

void GxEPD2_750c_GDEM075F52::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData(0xA5);    // control code
    _hibernating = true;
    _init_display_done = false;
  }
}

void GxEPD2_750c_GDEM075F52::setPaged()
{
  if (!hasPartialUpdate)
  {
    _paged = true;
    _InitDisplay();
    _writeCommand(0x10);
  }
}

void GxEPD2_750c_GDEM075F52::_refresh(bool partial_update_mode)
{
  _writeCommand(0x12); // Display Refresh
  _writeData(0x00);
  delay(1);
  _waitWhileBusy("_refresh", full_refresh_time);
  _init_display_done = false; // needed
}

void GxEPD2_750c_GDEM075F52::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_750c_GDEM075F52::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x02);
    _writeData(0x00);
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
}

void GxEPD2_750c_GDEM075F52::_InitDisplay()
{
  //Serial.println("_InitDisplay");
  // initcode from LaskaKit example for GDEM075F52
  if ((_rst >= 0) && (_hibernating || _initial_write))
  {
    digitalWrite(_rst, HIGH);
    delay(20); // At least 20ms delay
    digitalWrite(_rst, LOW); // Module reset
    delay(2);  // At least 40ms delay, 2ms for WS "clever" reset
    digitalWrite(_rst, HIGH);
    delay(2); // At least 50ms delay (32ms measured)
    _waitWhileBusy("_InitDisplay reset", power_on_time);
    _hibernating = false;
    _power_is_on = false;
  }
  _writeCommand(0x00); // PSR Panel Setting Register
  _writeData(0x0F);    // default res, scan up, shift left, booster on
  _writeData(0x29);    // LUT from OTP
  _writeCommand(0x06); // BTST Booster Soft Start
  _writeData(0x0F);
  _writeData(0x8B);
  _writeData(0x93);
  _writeData(0xA1);
  _writeCommand(0x41); // TSE
  _writeData(0x00);
  _writeCommand(0x50); // CDI VCOM and Data Interval Setting
  _writeData(0x37);    // (VBD 1, DDX 1) white border, 10hsync
  _writeCommand(0x60); // TCON
  _writeData(0x02);
  _writeData(0x02);
  _writeCommand(0x61); // TRES Resolution Setting
  _writeData(WIDTH / 256);   // Source_BITS_H
  _writeData(WIDTH % 256);   // Source_BITS_L
  _writeData(HEIGHT / 256);  // Gate_BITS_H
  _writeData(HEIGHT % 256);  // Gate_BITS_L
  _writeCommand(0x62); // Additional setting
  _writeData(0x98);
  _writeData(0x98);
  _writeData(0x98);
  _writeData(0x75);
  _writeData(0xCA);
  _writeData(0xB2);
  _writeData(0x98);
  _writeData(0x7E);
  _writeCommand(0x65); // Flash control
  _writeData(0x00);
  _writeData(0x00);
  _writeData(0x00);
  _writeData(0x00);
  _writeCommand(0xE7); // Unknown command
  _writeData(0x1C);
  _writeCommand(0xE3); // PWS Power Saving
  _writeData(0x00);
  _writeCommand(0xE9); // Unknown command
  _writeData(0x01);
  _writeCommand(0x30); // PLL Frame rate
  _writeData(0x08);
  _PowerOn();
  _init_display_done = true;
}
