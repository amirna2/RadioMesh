#include "CustomDisplay.h"

// This module is a mock of the IDisplay interface.
// It is used to test custom display integration with the DeviceBuilder.

CustomDisplay::CustomDisplay()
{

}

int CustomDisplay::setup()
{
   this->isSetup = true;
   return RM_E_NONE;
}

int CustomDisplay::powerSave(bool save)
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   this->inPowerSave = save;
   return RM_E_NONE;
}

int CustomDisplay::drawString(uint8_t x, uint8_t y, const std::string text)
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   if (x >= width || y >= height)
   {
      return RM_E_DISPLAY_INVALID_COORDS;
   }

   return RM_E_NONE;
}

int CustomDisplay::drawString(uint8_t x, uint8_t y, const char* text)
{

   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   if (x >= width || y >= height)
   {
      return RM_E_DISPLAY_INVALID_COORDS;
   }

   return RM_E_NONE;
}

int CustomDisplay::setCursor(uint8_t x, uint8_t y)
{

   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   if (x >= width || y >= height)
   {
      return RM_E_DISPLAY_INVALID_COORDS;
   }

   this->cursorX = x;
   this->cursorY = y;
   return RM_E_NONE;
}

int CustomDisplay::print(const std::string text)
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   return RM_E_NONE;
}

int CustomDisplay::clear()
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   return RM_E_NONE;
}

int CustomDisplay::flush()
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   return RM_E_NONE;
}

int CustomDisplay::showSplashScreen()
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   return RM_E_NONE;
}

uint8_t CustomDisplay::getWidth()
{
   if (!this->isSetup)
   {
      return 0;
   }

   return width;
}

uint8_t CustomDisplay::getHeight()
{
   if (!this->isSetup)
   {
      return 0;
   }
   return height;
}

int CustomDisplay::setFont(uint8_t fontId)
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   if (fontId != CUSTOM_DISPLAY_FONT_SMALL && fontId != CUSTOM_DISPLAY_FONT_MEDIUM && fontId != CUSTOM_DISPLAY_FONT_LARGE)
   {
      return RM_E_DISPLAY_INVALID_FONT;
   }
   this->fontId = fontId;
   return RM_E_NONE;
}

int CustomDisplay::drawNumber(uint8_t x, uint8_t y, int number)
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   if (x >= width || y >= height)
   {
      return RM_E_DISPLAY_INVALID_COORDS;
   }
   return RM_E_NONE;
}

int CustomDisplay::setBrightness(uint8_t brightness)
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   if (brightness > 100)
   {
      return RM_E_DISPLAY_INVALID_BRIGHTNESS;
   }
   return RM_E_NONE;
}

int CustomDisplay::setRotation(uint8_t rotation)
{
   if (!this->isSetup)
   {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   if (rotation > 3)
   {
      return RM_E_DISPLAY_INVALID_ROTATION;
   }
   return RM_E_NONE;
}



