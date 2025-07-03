/**
 * @file Errors.h
 * @brief This file contains error codes used in the RadioMesh project.
 */

#pragma once

/** @defgroup error_codes Error Codes
 *  @brief Error codes used throughout the RadioMesh project
 *
 *  This group contains all error code definitions used for error handling
 *  and status reporting in RadioMesh.
 */

/** @addtogroup error_codes
 *  @{
 */

/**
 * @brief No error occurred. All is well.
 */
#define RM_E_NONE (0)

/**
 * @brief Unknown error. Something went wrong...I mean, really wrong.
 */
#define RM_E_UNKNOWN (-1)

/**
 * @brief Invalid parameter passed to a function.
 */
#define RM_E_INVALID_PARAM (-2)

/**
 * @brief The packet is too long to send.
 *        Generally, this means your packet data section is too long.
 */
#define RM_E_PACKET_TOO_LONG (-3)

/**
 * @brief Some parameteter have invalid length.
 *        Device name, device id, topic, data, etc.
 */
#define RM_E_INVALID_LENGTH (-4)

/**
 * @brief The function is not implemented yet.
 */
#define RM_E_NOT_IMPLEMENTED (-5)

/**
 * @brief The function is not SUPPORTED.
 */
#define RM_E_NOT_SUPPORTED (-6)

/**
 * @brief The packet is corrupted. (Data section CRC mismatch)
 */
#define RM_E_PACKET_CORRUPTED (-7)

/**
 * @brief The packet has reached the maximum number of hops.
 */
#define RM_E_MAX_HOPS (-8)

/**
 * @brief The device type is invalid.
 */
#define RM_E_INVALID_DEVICE_TYPE (-9)

/**
 * @brief Device initialization failed.
 *        This error is returned when the device fails to initialize properly.
 */
#define RM_E_DEVICE_INITIALIZATION_FAILED (-10)

/**
 * @brief The radio setup failed.
 */
#define RM_E_RADIO_SETUP (-101)

/**
 * @brief The radio failed to transmit a packet.
 */
#define RM_E_RADIO_TX (-102)

/**
 * @brief The radio failed to receive a packet.
 */
#define RM_E_RADIO_RX (-103)

/**
 * @brief The radio transmit timed out.
 */
#define RM_E_RADIO_TX_TIMEOUT (-104)

/**
 * @brief The radio receive timed out.
 */
#define RM_E_RADIO_RX_TIMEOUT (-105)

/**
 * @brief The radio is not initialized.
 */
#define RM_E_RADIO_NOT_INITIALIZED (-106)

/**
 * @brief The radio is already setup.
 */
#define RM_E_RADIO_ALREADY_SETUP (-107)

/**
 * @brief The radio CRC mismatch.
 */
#define RM_E_RADIO_CRC_MISMATCH (-108)

/**
 * @brief The radio function failed.
 */
#define RM_E_RADIO_FAILURE (-109)

/**
 * @brief The radio parameters are invalid.

*/
#define RM_E_INVALID_RADIO_PARAMS (-110)

/**
 * @brief The radio CRC mismatch.
 */
#define RM_E_RADIO_HEADER_CRC_MISMATCH (-111)

/**
 * @brief The display setup failed.
 */
#define RM_E_DISPLAY_SETUP (-201)

/**
 * @brief The display operation failed to complete.
 */
#define RM_E_DISPLAY_FAILURE (-202)

/**
 * @brief The display is not setup. Call setup() first.
 */
#define RM_E_DISPLAY_NOT_SETUP (-203)

/**
 * @brief The display font is invalid.
 */
#define RM_E_DISPLAY_INVALID_FONT (-204)

/**
 * @brief The display failed to draw string.
 */
#define RM_E_DISPLAY_DRAW_STRING (-205)

/**
 * @brief The display failed to draw number.
 */
#define RM_E_DISPLAY_DRAW_NUMBER (-206)

/**
 * @brief The display coordinates are invalid/out of bounds.
 */
#define RM_E_DISPLAY_INVALID_COORDS (-207)

/**
 * @brief The display rotation is invalid.
 */
#define RM_E_DISPLAY_INVALID_ROTATION (-208)

/**
 * @brief The display brightness is invalid.
 */
#define RM_E_DISPLAY_INVALID_BRIGHTNESS (-209)

/**
 * @brief SSID in not available.
 */
#define RM_E_WIFI_SSID_NOT_AVAILABLE (-301)

/**
 * @brief WiFi connection failed.
 */
#define RM_E_WIFI_CONNECTION_FAILED (-302)

/**
 * @brief WiFi disconnection failed.
 */
#define RM_E_WIFI_DISCONNECT_FAILED (-303)

/**
 * @brief WiFi SSID not set.
 */
#define RM_E_WIFI_SSID_NOT_SET (-304)

/**
 * @brief WiFi AP already started.
 */
#define RM_E_WIFI_AP_ALREADY_STARTED (-305)

/**
 * @brief WiFi AP already stopped.
 */
#define RM_E_WIFI_AP_START_FAILED (-306)

/**
 * @brief WiFi AP failed to stop.
 */
#define RM_E_WIFI_AP_STOP_FAILED (-307)

/**
 * @brief WiFi AP setup failed.
 */
#define RM_E_WIFI_AP_SETUP (-308)

/**
 * @brief The wifi parameters are invalid.
 */
#define RM_E_INVALID_WIFI_PARAMS (-309)

/**
 * @brief The wifi parameters are invalid.
 */
#define RM_E_INVALID_AP_PARAMS (-310)

/**
 * @brief The display setup failed.
 */
#define RM_E_CRYPTO_SETUP (-401)

/**
 * @brief The crypto parameters are invalid.
 */
#define RM_E_INVALID_CRYPTO_PARAMS (-402)

/**
 * @brief The storage setup failed.
 */
#define RM_E_STORAGE_SETUP (-501)

/**
 * @brief The storage is not initialized.
 */
#define RM_E_STORAGE_NOT_INIT (-502)

/**
 * @brief The storage key is not found.
 */
#define RM_E_STORAGE_KEY_NOT_FOUND (-503)

/**
 * @brief The storage read failed.
 */
#define RM_E_STORAGE_READ_FAILED (-504)

/**
 * @brief The storage write failed.
 */
#define RM_E_STORAGE_WRITE_FAILED (-505)

/**
 * @brief The does not have enough space left.
 */
#define RM_E_STORAGE_NOT_ENOUGH_SPACE (-506)

/**
 * @brief The storage size is invalid.
 */
#define RM_E_STORAGE_INVALID_SIZE (-507)

/**
 * @brief invalid inclusion state
 */
#define RM_E_INVALID_STATE (-601)

/**
 * @brief inclusion failed
 */
#define RM_E_INCLUSION_FAILED (-602)

/**
 * @brief inclusion open failed
 */
#define RM_E_DEVICE_NOT_INCLUDED (-603)
