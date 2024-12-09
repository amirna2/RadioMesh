#pragma once

#include <vector>
#include <string>

#include <common/inc/Definitions.h>
#include <framework/interfaces/IDisplay.h>
#include <common/inc/Errors.h>

#ifndef RM_NO_DISPLAY
#include <U8g2lib.h>
#endif

class OledDisplay : public IDisplay
{
public:
   /**
    * @brief Get the instance of the OledDisplay class.
    * @returns A pointer to the instance of the OledDisplay class.
   */
   static OledDisplay* getInstance() {
   #ifdef RM_NO_DISPLAY
      return nullptr;
   #endif

      if (!instance) {
         instance = new OledDisplay();
      }
      return instance;
   }

   virtual ~OledDisplay() {}

   // IDisplay interface
   #ifdef RM_NO_DISPLAY
   int setup() override { return RM_E_NOT_SUPPORTED; }
   int powerSave(bool save) override { return RM_E_NOT_SUPPORTED; }
   int drawString(uint8_t x, uint8_t y, const std::string text) override { return RM_E_NOT_SUPPORTED; }
   int drawString(uint8_t x, uint8_t y, const char* text) override { return RM_E_NOT_SUPPORTED; }
   int setCursor(uint8_t x, uint8_t y) override { return RM_E_NOT_SUPPORTED; }
   int print(const std::string text) override { return RM_E_NOT_SUPPORTED; }
   int clear() override { return RM_E_NOT_SUPPORTED; }
   int flush() override { return RM_E_NOT_SUPPORTED; }
   int showSplashScreen() override { return RM_E_NOT_SUPPORTED; }
   uint8_t getWidth() override { return 0; }
   uint8_t getHeight() override { return 0; }
   int setFont(uint8_t fontId) override { return RM_E_NOT_SUPPORTED; }
   int drawNumber(uint8_t x, uint8_t y, int number) override { return RM_E_NOT_SUPPORTED; }

   int setParams(const OledDisplayParams& params) { return RM_E_NOT_SUPPORTED; }
   int setBrightness(uint8_t brightness) override { return RM_E_NOT_SUPPORTED; }
   int setRotation(uint8_t rotation) override { return RM_E_NOT_SUPPORTED; }

#else
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

   int setBrightness(uint8_t brightness) override { return RM_E_NOT_SUPPORTED; }
   int setRotation(uint8_t rotation) override { return RM_E_NOT_SUPPORTED; }

   int setParams(const OledDisplayParams& params);
#endif

private:

   OledDisplay() {};
   OledDisplay(OledDisplay const&) = delete;
   OledDisplay& operator=(OledDisplay const&) = delete;

   static OledDisplay* instance;

   OledDisplayParams displayParams;

#ifndef RM_NO_DISPLAY
   U8G2_SSD1306_128X64_NONAME_F_HW_I2C* u8g2 = nullptr;
#endif

   uint8_t width;
   uint8_t height;
};
