#pragma once

#include <vector>
#include <string>

#include <common/inc/Options.h>

/**
 * @class IDisplay
 * @brief This class is an interface for a display.
 *
 * It provides the structure for setting up the display component of a device.
 * Use this interface to create specialized display components such as OLED, E-Ink, etc.
 */
class IDisplay
{
public:
   virtual ~IDisplay() {}

   /**
    * @brief Setup the display with the currently stored parameters.
    *
    * @returns RM_E_NONE if the display was successfully setup, an error code otherwise.
    */
   virtual int setup() = 0;

   /**
    * @brief Set the display into standby mode.
    * @param save If true, the display will be put into power save mode. If false, the display will be woken up.
    * @returns RM_E_NONE if the display was successfully put into standby mode, an error code otherwise.
    */
   virtual int powerSave(bool save) = 0;

   /**
    * @brief Draw a string on the display.
    * @param x The x coordinate of the string.
    * @param y The y coordinate of the string.
    * @param clearScreen If true, the screen will be cleared before drawing the string.
    * @param text The text to draw.
    * @returns RM_E_NONE if the string was successfully drawn, an error code otherwise.
   */
   virtual int drawString(uint8_t x, uint8_t y, const std::string text) = 0;

   /**
    * @brief Draw a string on the display.
    * @param x The x coordinate of the string.
    * @param y The y coordinate of the string.
    * @param clearScreen If true, the screen will be cleared before drawing the string.
    * @param text The text to draw.
    * @returns RM_E_NONE if the string was successfully drawn, an error code otherwise.
   */
   virtual int drawString(uint8_t x, uint8_t y, const char* text) = 0;

   /**
    * @brief Draw a number on the display.
    * @param x The x coordinate of the number.
    * @param y The y coordinate of the number.
    * @param number The number to draw.
    * @returns RM_E_NONE if the number was successfully drawn, an error code otherwise.
   */
   virtual int drawNumber(uint8_t x, uint8_t y, int number) = 0;
   /**
    * @brief Set the cursor position of the display.
    * @param x The x coordinate of the cursor.
    * @param y The y coordinate of the cursor.
    * @returns RM_E_NONE if the cursor was successfully set, an error code otherwise.
   */
   virtual int setCursor(uint8_t x, uint8_t y) = 0;

   /**
    * @brief Print a string to the display at the current cursor position.
    * @param text The text to print.
    * @returns RM_E_NONE if the text was successfully printed, an error code otherwise.
   */
   virtual int print(const std::string text) = 0;

   /**
    * @brief Clear the display buffer.
    * @returns RM_E_NONE if the display buffer was successfully cleared, an error code otherwise.
   */
   virtual int clear() = 0;
   /**
    * @brief Send the display buffer to the display.
    * @returns RM_E_NONE if the display buffer was successfully sent, an error code otherwise.
   */
   virtual int flush() = 0;

   /**
    * @brief Get the width of the display.
    * @returns The width of the display.
   */
   virtual uint8_t getWidth() = 0;

   /**
    * @brief Get the height of the display.
    * @returns The height of the display.
   */
   virtual uint8_t getHeight() = 0;

   /**
    * @brief Show the splash screen on the display.
    * @returns RM_E_NONE if the splash screen was successfully shown, an error code otherwise.
   */
   virtual int showSplashScreen() = 0;

   /**
    * @brief Set the font of the display.
    * @param fontId The ID of the font to set (e.g RM_FONT_MEDIUM)
    * @returns RM_E_NONE if the font was successfully set, an error code otherwise.
   */
   virtual int setFont(uint8_t fontId) = 0;
};
