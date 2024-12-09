// This module is a mock of the IDisplay interface.
// It is used to test custom display integration with the DeviceBuilder.

#pragma once

#include <RadioMesh.h>

#define CUSTOM_DISPLAY_WIDTH 128
#define CUSTOM_DISPLAY_HEIGHT 64

#define CUSTOM_DISPLAY_FONT_SMALL 0
#define CUSTOM_DISPLAY_FONT_MEDIUM 1
#define CUSTOM_DISPLAY_FONT_LARGE 2

class CustomDisplay : public IDisplay
{
public:
   CustomDisplay() {}
   virtual ~CustomDisplay() {}

   int setup() override;
   int powerSave(bool save) override;
   int drawString(uint8_t x, uint8_t y, const std::string text) override;
   int drawString(uint8_t x, uint8_t y, const char* text) override;
   int setCursor(uint8_t x, uint8_t y) override;
   int print(const std::string text) override;
   int clear() override;
   int flush() override;
   int showSplashScreen() override;
   uint8_t getWidth() override;
   uint8_t getHeight() override;
   int setFont(uint8_t fontId) override;
   int drawNumber(uint8_t x, uint8_t y, int number) override;

   int setBrightness(uint8_t brightness) override;
   int setRotation(uint8_t rotation) override;

private:
   uint8_t width = CUSTOM_DISPLAY_WIDTH;
   uint8_t height = CUSTOM_DISPLAY_HEIGHT;
   uint8_t fontId = CUSTOM_DISPLAY_FONT_MEDIUM;
   uint8_t brightness = 50;
   uint8_t rotation = 0;
   bool inPowerSave = false;
   bool isSetup = false;

   uint8_t cursorX = 0;
   uint8_t cursorY = 0;
};
