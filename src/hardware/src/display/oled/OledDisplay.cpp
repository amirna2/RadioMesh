
#include <hardware/inc/display/oled/OledDisplay.h>
#include <common/inc/Logger.h>
#include <common/inc/Errors.h>
#include <common//utils/Utils.h>

OledDisplay* OledDisplay::instance = nullptr;

#ifndef RM_NO_DISPLAY

int OledDisplay::setParams(const OledDisplayParams& params) {
   // no need to check the parameters as they are just pin numbers
   // actual call to setup the display will check if the pins are valid
   displayParams = params;
   return RM_E_NONE;
}

int OledDisplay::setup() {
   u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, displayParams.resetPin, displayParams.clockPin, displayParams.dataPin);
   if (u8g2 == nullptr) {
      logerr_ln("ERROR  failed to create display object");
      return RM_E_UNKNOWN;
   }

   if (!u8g2->begin()) {
      logerr_ln("ERROR  failed to initialize display");
      return RM_E_DISPLAY_SETUP;
   }
   if (setFont(displayParams.fontId) != RM_E_NONE) {
      logerr_ln("ERROR  failed to set font");
      return RM_E_DISPLAY_INVALID_FONT;
   }

   u8g2->clearDisplay();
   drawString(5, 40, "RadioMesh");
   drawString(5, 54, "v" + RadioMeshUtils::getVersion());

   width = u8g2->getCols();
   height = u8g2->getRows();

   return RM_E_NONE;
}

int OledDisplay::powerSave(bool save) {
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   if (save) {
      u8g2->clear();
      u8g2->setPowerSave(1);
   } else {
      u8g2->setPowerSave(0);
      u8g2->initDisplay();
   }
   return RM_E_NONE;
}

int OledDisplay::drawString(uint8_t x, uint8_t y, const char* text)
{
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   int res = u8g2->drawStr(x, y, text);
   if (res == 0) {
      logerr_ln("ERROR  failed to draw string");
      return RM_E_DISPLAY_DRAW_STRING;
   }
   // TODO: should we have a deffered sendBuffer()?
   // Maybe a flag to indicate that the buffer should be sent after a drawString() call?
   u8g2->sendBuffer();
   return RM_E_NONE;
}

int OledDisplay::drawString(uint8_t x, uint8_t y, const std::string text) {
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   return drawString(x, y, text.c_str());

}

int OledDisplay::drawNumber(uint8_t x, uint8_t y, int number) {
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   char buffer[10] = {0};
   snprintf(buffer, 10, "%d", number);
   return drawString(x, y, buffer);
}

int OledDisplay::setCursor(uint8_t x, uint8_t y) {
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   u8g2->setCursor(x, y);
   return RM_E_NONE;
}

int OledDisplay::print(const std::string text) {
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   u8g2->print(text.c_str());
   return RM_E_NONE;
}

int OledDisplay::clear() {
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   u8g2->clear();
   return RM_E_NONE;
}

int OledDisplay::flush() {
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }
   u8g2->sendBuffer();
   return RM_E_NONE;
}

int OledDisplay::showSplashScreen() {
   // show the splash screen
   // ...
   return RM_E_NOT_IMPLEMENTED;
}

uint8_t OledDisplay::getWidth() {
   if (u8g2 == nullptr) {
      return 0;
   }
   return u8g2->getCols();
}

uint8_t OledDisplay::getHeight() {
   if (u8g2 == nullptr) {
      return 0;
   }
   return u8g2->getRows();
}

int OledDisplay::setFont(uint8_t fontId) {
   if (u8g2 == nullptr) {
      return RM_E_DISPLAY_NOT_SETUP;
   }

   if (fontId == RM_FONT_TINY) {
      u8g2->setFont(u8g2_font_5x7_tf);
   } else if (fontId == RM_FONT_SMALL) {
      u8g2->setFont(u8g2_font_6x13_tf);
   } else if (fontId == RM_FONT_MEDIUM) {
      u8g2->setFont(u8g2_font_8x13_tf);
   } else if (fontId == RM_FONT_LARGE) {
      u8g2->setFont(u8g2_font_10x20_tf);
   } else if (fontId == RM_FONT_BATTERY) {
      u8g2->setFont(u8g2_font_battery19_tn);
   } else {
      return RM_E_DISPLAY_INVALID_FONT;
   }
   return RM_E_NONE;
}

#endif // ! RM_NO_DISPLAY


