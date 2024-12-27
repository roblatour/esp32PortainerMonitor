// esp32 TFT Virtual Window
//
// Copyright Rob Latour, 2024
// License: MIT
//
// https://github.com/roblatour/esp32PortainerMonitor

#include "virtual_window.h"
#include <bb_spi_lcd.h>

VirtualWindow::VirtualWindow(BB_SPI_LCD* touchPanel, TFT_eSPI* displayPanel, int16_t maxBufferRows, int16_t maxBufferColumns) {
  touch = touchPanel;
  display = displayPanel;
  maxRows = maxBufferRows;
  maxColumns = maxBufferColumns;
  verticalScrollPosition = 0;
  horizontalScrollPosition = 0;
  lineHeight = 8;
  visibleRows = display->height() / lineHeight;
  visibleColumns = display->width() / 6;  // Assuming each character is 6 pixels wide
  currentTextColor = TFT_GREEN;
  currentBackgroundColor = TFT_BLACK;
  currentLine = "";
  display->begin();
  display->setRotation(1);
  display->fillScreen(currentBackgroundColor);
  display->setTextSize(1);
  display->setTextColor(currentTextColor, currentBackgroundColor);
  Serial.println("Virtual Window initialized");
}

void VirtualWindow::clear() {
  buffer.clear();
  verticalScrollPosition = 0;
  horizontalScrollPosition = 0;
  display->fillScreen(currentBackgroundColor);
}

void VirtualWindow::render() {
  display->setTextSize(1);
  display->fillScreen(currentBackgroundColor);

  int16_t y = 0;

  int16_t limit = min(verticalScrollPosition + visibleRows, (int)buffer.size());
  for (int i = verticalScrollPosition; i < limit; i++) {
    display->setTextColor(buffer[i].textColor, buffer[i].backgroundColor);
    display->drawString(buffer[i].text.substring(horizontalScrollPosition, horizontalScrollPosition + visibleColumns), 0, y);
    y += lineHeight;
  }
}

void VirtualWindow::renderLine(int index) {
  if (index >= verticalScrollPosition && index < verticalScrollPosition + visibleRows) {
    int16_t y = (index - verticalScrollPosition) * lineHeight;
    display->setTextColor(buffer[index].textColor, buffer[index].backgroundColor);
    display->drawString(buffer[index].text.substring(horizontalScrollPosition, horizontalScrollPosition + visibleColumns), 0, y);
  }
}

void VirtualWindow::print(const String& text) {
  for (char c : text) {
    if (c == '\r') {
      // Ignore carriage return
      continue;
    } else if (c == '\n') {
      // New line character
      buffer.push_back(LineAttributes(currentLine, currentTextColor, currentBackgroundColor));
      currentLine = "";
      renderLine(buffer.size() - 1);
    } else {
      // Print character immediately
      display->drawChar(c, currentLine.length() * 6, buffer.size() * lineHeight, 1);
      currentLine += c;
    }
  }

  while (buffer.size() > maxRows) {
    buffer.erase(buffer.begin());
    if (verticalScrollPosition > 0) verticalScrollPosition--;
  }
}

void VirtualWindow::println(const String& text) {
  print(text + "\n");
}

void VirtualWindow::scrollUp() {
  if (verticalScrollPosition > 0) {
    verticalScrollPosition--;
    render();
  }
}

void VirtualWindow::scrollDown() {
  if (verticalScrollPosition + visibleRows < buffer.size()) {
    verticalScrollPosition++;
    render();
  }
}

void VirtualWindow::scrollLeft() {
  if (horizontalScrollPosition > 0) {
    horizontalScrollPosition--;
    render();
  }
}

void VirtualWindow::scrollRight() {
  if (horizontalScrollPosition + visibleColumns < maxColumns) {
    horizontalScrollPosition++;
    render();
  }
}

void VirtualWindow::setColors(uint16_t text, uint16_t background) {
  currentTextColor = text;
  currentBackgroundColor = background;
}

void VirtualWindow::handleTouch() {
  const int16_t bottomOfTopSection = (display->height() / 3) * 2;
  const int16_t topOfTheBottomSection = display->height() / 3;
  const int16_t midPoint = display->width() / 2;

  TOUCHINFO ti;
  touch->rtReadTouch(&ti);

  // if the screen was touched
  if (ti.count > 0) {

    // get the x and y coordinates of the point that was touched
    // note: 0,0 is in bottom right

    uint16_t x = ti.x[0];
    uint16_t y = ti.y[0];

    if (y < topOfTheBottomSection) {
      scrollDown();
    } else if (y > bottomOfTopSection) {
      scrollUp();
    } else if (x < midPoint) {
      scrollRight();
    } else {
      scrollLeft();
    }
  }
}