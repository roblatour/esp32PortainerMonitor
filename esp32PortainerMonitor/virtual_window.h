// esp32 TFT Virtual Window
//
// Copyright Rob Latour, 2024
// License: MIT
//
// https://github.com/roblatour/esp32PortainerMonitor  

#ifndef VIRTUAL_WINDOW_H
#define VIRTUAL_WINDOW_H

#include <TFT_eSPI.h>
#include <bb_spi_lcd.h>
#include <vector>
#include <String>

struct LineAttributes {
  String text;
  uint16_t textColor;
  uint16_t backgroundColor;

  LineAttributes(const String& t, uint16_t tc, uint16_t bc)
    : text(t), textColor(tc), backgroundColor(bc) {}
};

class VirtualWindow {
private:
  BB_SPI_LCD* touch;
  TFT_eSPI* display;
  int16_t maxRows;
  int16_t maxColumns;
  int16_t verticalScrollPosition;
  int16_t horizontalScrollPosition;  
  int16_t lineHeight;
  int16_t visibleRows;
  int16_t visibleColumns;   
  uint16_t currentTextColor;
  uint16_t currentBackgroundColor;
  String currentLine;
  std::vector<LineAttributes> buffer;

public:
  VirtualWindow(BB_SPI_LCD* touchPanel, TFT_eSPI* display, int16_t maxBufferRows = 200, int16_t maxBufferColumns = 200);
  void clear();
  void println(const String& text);
  void print(const String& text);
  void scrollUp();
  void scrollDown();
  void scrollLeft();
  void scrollRight();
  void render();
  void renderLine(int index);
  void setColors(uint16_t text, uint16_t background);
  void handleTouch();
};

#endif