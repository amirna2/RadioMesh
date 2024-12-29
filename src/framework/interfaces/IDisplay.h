#pragma once

#include <string>
#include <vector>

#include <common/inc/Options.h>

/**
 * @class IDisplay
 * @brief Interface for display implementations in the RadioMesh framework
 *
 * This interface defines the contract for display implementations.
 * Custom display implementations must use the standard RM_E_* error codes
 * to maintain consistent error handling throughout the system.
 */
class IDisplay
{
public:
    virtual ~IDisplay()
    {
    }

    /**
     * @brief Initialize the display hardware
     *
     * Must be called after device configuration but before any display operations.
     * Performs actual hardware initialization.
     *
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int setup() = 0;

    /**
     * @brief Set the display into standby mode.
     * @param save If true, the display will be put into power save mode. If false, the display will
     * be woken up.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int powerSave(bool save) = 0;

    /**
     * @brief Draw a string on the display.
     * @param x The x coordinate of the string.
     * @param y The y coordinate of the string.
     * @param clearScreen If true, the screen will be cleared before drawing the string.
     * @param text The text to draw.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int drawString(uint8_t x, uint8_t y, const std::string text) = 0;

    /**
     * @brief Draw a string on the display.
     * @param x The x coordinate of the string.
     * @param y The y coordinate of the string.
     * @param clearScreen If true, the screen will be cleared before drawing the string.
     * @param text The text to draw.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int drawString(uint8_t x, uint8_t y, const char* text) = 0;

    /**
     * @brief Draw a number on the display.
     * @param x The x coordinate of the number.
     * @param y The y coordinate of the number.
     * @param number The number to draw.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int drawNumber(uint8_t x, uint8_t y, int number) = 0;
    /**
     * @brief Set the cursor position of the display.
     * @param x The x coordinate of the cursor.
     * @param y The y coordinate of the cursor.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int setCursor(uint8_t x, uint8_t y) = 0;

    /**
     * @brief Print a string to the display at the current cursor position.
     * @param text The text to print.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int print(const std::string text) = 0;

    /**
     * @brief Clear the display buffer.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int clear() = 0;
    /**
     * @brief Send the display buffer to the display.
     * @return RM_E_NONE Success. And error code otherwise.
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
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int showSplashScreen() = 0;

    /**
     * @brief Set the font of the display.
     * @param fontId The ID of the font to set (e.g RM_FONT_MEDIUM)
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int setFont(uint8_t fontId) = 0;

    /**
     * @brief Set the parameters of the display.
     * @param params The parameters to set.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int setBrightness(uint8_t level) = 0;
    /**
     * @brief Set the rotation of the display.
     * @param rotation The rotation to set.
     * @return RM_E_NONE Success. And error code otherwise.
     */
    virtual int setRotation(uint8_t rotation) = 0;
};
