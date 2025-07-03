# Device Inclusion Example - Automatic Protocol

This example demonstrates the **new automatic inclusion protocol** implemented in RadioMesh. The inclusion process is now handled automatically by the `InclusionController` at the protocol layer, requiring minimal application code.

## Overview

This example shows how devices automatically join a mesh network using the secure inclusion protocol:

1. **Hub Device** enables inclusion mode and broadcasts `INCLUDE_OPEN` messages
2. **Standard Devices** automatically respond when they hear the broadcast
3. **Automatic Key Exchange** occurs between hub and device using ECDH
4. **Timeout and Retry** logic ensures reliable inclusion
5. **State Persistence** allows devices to remember their inclusion status

## Key Improvements from Previous Implementation

### Before (Application-Level)
- Manual state machine in application code
- Manual handling of all inclusion messages
- Complex timeout and retry logic in application
- Error-prone message sequencing

### Now (Protocol-Level)
- Automatic inclusion protocol handling
- Application simply enables/disables inclusion mode
- Built-in timeout and retry with exponential backoff
- Robust state machine with proper error recovery

## Files

- **`MiniHub/`** - Hub device that manages network inclusion
- **`Standard/`** - Standard device that joins the network automatically
- **`device_info.h`** - Device configuration (in each subfolder)

## Hardware Requirements

### Hub Device (MiniHub)
- Heltec WiFi LoRa 32 V3 (ESP32S3 with LoRa and OLED)
- LoRa radio on 868MHz or 915MHz
- OLED display for status

### Standard Device
- Seeed XIAO ESP32S3 with LoRa module
- LoRa radio on same frequency as hub
- No display required

## How to Use

### 1. Setup Hub Device

```bash
cd examples/DeviceInclusion/MiniHub
pio run -e heltec_wifi_lora_32_V3 -t upload
```

The hub will:
- Initialize and show "Hub Ready" on display
- Automatically enter inclusion mode for 60 seconds
- Display inclusion statistics (requests received, successful inclusions)
- Handle all inclusion protocol messages automatically

### 2. Setup Standard Device

```bash
cd examples/DeviceInclusion/Standard
pio run -e seeed_xiao_esp32s3 -t upload
```

The standard device will:
- Initialize and wait for hub inclusion broadcast
- Automatically join when hub enables inclusion mode
- Start sending sensor data once included
- Handle all inclusion protocol messages automatically

### 3. Monitor the Process

Watch the serial output from both devices to see the automatic inclusion process:

**Hub Output:**
```
=== RadioMesh Hub Device - Automatic Inclusion Example ===
[INFO] Hub device initialized successfully
[INFO] Starting inclusion mode for 60 seconds
[INFO] Inclusion mode active - devices can now join the network
[INFO] Received inclusion request from device (total: 1)
[INFO] Received inclusion confirmation from device
[INFO] Device inclusion completed successfully (total: 1)
```

**Device Output:**
```
=== RadioMesh Standard Device - Automatic Inclusion Example ===
[INFO] Standard device initialized successfully
[INFO] Device ready - waiting for hub inclusion broadcast
[INFO] Received INCLUDE_OPEN from hub - automatic inclusion will start
[INFO] Received INCLUDE_RESPONSE from hub - processing keys automatically
[INFO] Inclusion completed successfully! Device is now part of the network
[INFO] Device state: INCLUDED - successfully joined network!
[INFO] Sent sensor data: 25°C
```

## Application Code Comparison

### Old Implementation (Manual)
```cpp
// Application had to handle all inclusion messages manually
void handleInclusionMessage(uint8_t topic, const RadioMeshPacket& packet) {
    switch (topic) {
        case INCLUDE_OPEN:
            if (state == IDLE) {
                sendInclusionRequest();
                state = WAITING_RESPONSE;
                startTimer();
            }
            break;
        case INCLUDE_RESPONSE:
            if (state == WAITING_RESPONSE) {
                processKeys(packet);
                sendInclusionConfirm();
                state = WAITING_SUCCESS;
            }
            break;
        // ... more manual handling
    }
}
```

### New Implementation (Automatic)
```cpp
// Application only monitors inclusion for UI updates (optional)
void onPacketReceived(const RadioMeshPacket* packet, int err) {
    switch (packet->topic) {
        case MessageTopic::INCLUDE_OPEN:
            loginfo_ln("Inclusion starting automatically");
            break;
        case MessageTopic::INCLUDE_SUCCESS:
            loginfo_ln("Inclusion completed successfully!");
            break;
        // InclusionController handles all protocol logic automatically
    }
}
```

## Protocol Features Demonstrated

### Automatic State Machine
- Protocol states: IDLE → WAITING_FOR_REQUEST → WAITING_FOR_CONFIRMATION → IDLE
- State transitions handled automatically
- Proper state logging for debugging

### Timeout and Retry Logic
- Exponential backoff: 5s, 10s, 20s
- Maximum 3 retries per message
- Automatic state reset on failure
- 35-second total timeout

### Security Features
- ECDH key exchange (placeholder implementation)
- Session key encryption
- Nonce validation
- Replay protection

### Error Recovery
- Automatic retry on timeout
- Clean state reset on max retries
- Device state rollback on inclusion failure
- Robust error logging

## Configuration

### Radio Parameters
Both devices must use the same radio configuration:
```cpp
// In device_info.h
const LoraRadioParams radioParams = {
    .frequency = 868.0,      // EU band (use 915.0 for US)
    .bandwidth = 125.0,
    .spreadingFactor = 7,
    .codingRate = 5,
    .outputPower = 14
};
```

### Inclusion Timing
Configurable in the examples:
```cpp
const unsigned long INCLUSION_MODE_DURATION = 60000; // Hub inclusion window
const unsigned long STATUS_UPDATE_INTERVAL = 3000;   // UI update frequency
```

## Troubleshooting

### Device Not Joining
1. Check radio frequencies match between hub and device
2. Verify devices are within radio range
3. Check inclusion mode timeout (default 60 seconds)
4. Monitor serial output for error messages

### Protocol Timeouts
1. Check for radio interference
2. Verify signal strength (RSSI values in logs)
3. Increase inclusion mode duration if needed
4. Check for hardware issues (antenna connections)

### Build Issues
1. Ensure PlatformIO environment matches your hardware
2. Check device_info.h configuration
3. Verify RadioMesh library is properly installed

## Next Steps

1. **Test Real Hardware**: Deploy to actual LoRa devices
2. **Add Security**: Replace placeholder crypto with real ECDH
3. **Extended Testing**: Test with multiple devices joining simultaneously
4. **UI Enhancement**: Add button controls for inclusion mode
5. **Range Testing**: Test inclusion at various distances

## Notes

- Inclusion process is fully automatic once hub enables inclusion mode
- Application callbacks are optional for monitoring/UI updates
- State persistence ensures devices remember inclusion status after reboot
- Protocol is designed to be robust against radio interference and timing issues