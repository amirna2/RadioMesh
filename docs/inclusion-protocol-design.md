# RadioMesh Device Inclusion Protocol Design

## Overview

The RadioMesh inclusion protocol provides a secure, automatic mechanism for new devices to join an existing mesh network. The protocol operates entirely at the protocol layer, requiring no application intervention while maintaining security through cryptographic key exchange.

## Design Goals

1. **Automatic Operation**: Inclusion should happen automatically when a hub is in inclusion mode and a new device powers on
2. **Security**: Use proper cryptographic techniques (ECDH key exchange) to establish secure communication
3. **Protocol Integration**: Inclusion is part of the core protocol, not an application-layer concern
4. **State Management**: Devices track their inclusion state persistently
5. **Error Resilience**: Handle timeouts, retries, and error conditions gracefully

## Protocol Flow

### Message Sequence

```
Actor          HUB                                 NEW DEVICE
  |             |                                      |
  |--switch to->|                                      |<--Enter Discovery
  | inclusion   |                                      |    Mode
  |   mode      |----Broadcast INCLUDE_OPEN----------->|
  |             |                                      |
  |             |<---INCLUDE_REQUEST-------------------|
  |             |    (DeviceID, PublicKey,             |
  |             |     InitialCounter)                  |
  |             |                                      |
  |             |--Verify DeviceID                     |
  |             |--Generate Session Key                |
  |             |--Generate Nonce                      |
  |             |                                      |
  |             |----INCLUDE_RESPONSE----------------->|
  |             |    (HubPublicKey,                    |
  |             |     EncryptedSessionKey,             |
  |             |     EncryptedNonce,                  |
  |             |     HubInitialCounter)               |
  |             |                                      |
  |             |                              Decrypt Session Key
  |             |                              Store Hub Public Key
  |             |                              Decrypt & Verify Nonce
  |             |                              Increment Nonce
  |             |                                      |
  |             |<---INCLUDE_CONFIRM-------------------|
  |             |    (IncrementedEncryptedNonce)       |
  |             |                                      |
  |             |--Decrypt & Verify Nonce              |
  |             |                                      |
  |             |----INCLUDE_SUCCESS------------------>|
  |             |                                      |
  |             |                                      |
  |             |<========Encrypted Communication=====>|
```

### Message Topics

- `INCLUDE_OPEN` (0x08): Broadcast by hub when in inclusion mode
- `INCLUDE_REQUEST` (0x06): Sent by device wanting to join
- `INCLUDE_RESPONSE` (0x07): Hub's response with encrypted session key
- `INCLUDE_CONFIRM` (0x09): Device's confirmation with incremented nonce
- `INCLUDE_SUCCESS` (0x0A): Hub's final acknowledgment

### Device States

```
enum DeviceInclusionState {
    NOT_INCLUDED = 0x01,      // Can only send inclusion messages
    INCLUSION_PENDING = 0x02, // Inclusion in progress
    INCLUDED = 0x03          // Normal operation
}
```

## Implementation Architecture

### Protocol Layer Integration

The inclusion protocol is integrated directly into the `RadioMeshDevice::handleReceivedData()` method:

```cpp
int RadioMeshDevice::handleReceivedData(RadioMeshPacket& packet) {
    // ... existing validation ...

    // Check if this is an inclusion message
    if (isInclusionMessage(packet.topic)) {
        // Handle automatically via InclusionController
        int result = inclusionController->handleInclusionMessage(packet);

        // Optionally still notify application for monitoring
        if (rxCallback && (inclusionCallbackMode == NOTIFY_ALL)) {
            rxCallback(packet);
        }

        return result;
    }

    // ... normal packet processing ...
}
```

### InclusionController State Machine

The InclusionController implements a state machine for both hub and device roles:

#### Hub State Machine
```
IDLE -> INCLUSION_MODE (enableInclusionMode)
    -> Broadcast INCLUDE_OPEN periodically

INCLUSION_MODE + INCLUDE_REQUEST received
    -> Validate device
    -> Generate session key
    -> Send INCLUDE_RESPONSE
    -> Move to WAITING_CONFIRMATION

WAITING_CONFIRMATION + INCLUDE_CONFIRM received
    -> Validate incremented nonce
    -> Send INCLUDE_SUCCESS
    -> Store device info
    -> Return to INCLUSION_MODE or IDLE
```

#### Device State Machine
```
NOT_INCLUDED + INCLUDE_OPEN received
    -> Send INCLUDE_REQUEST with public key
    -> Move to WAITING_RESPONSE

WAITING_RESPONSE + INCLUDE_RESPONSE received
    -> Derive shared secret (ECDH)
    -> Decrypt session key
    -> Validate and increment nonce
    -> Send INCLUDE_CONFIRM
    -> Move to WAITING_SUCCESS

WAITING_SUCCESS + INCLUDE_SUCCESS received
    -> Store session key
    -> Update inclusion state to INCLUDED
    -> Enable normal operation
```

### Security Implementation

#### Key Exchange
- Use Elliptic Curve Diffie-Hellman (ECDH) with Curve25519
- Each device generates a key pair during initialization
- Public keys exchanged during inclusion
- Shared secret derived using ECDH

#### Session Key Distribution
- Hub generates random 32-byte session key
- Encrypts using ECIES with device's public key
- Device decrypts using its private key

#### Nonce Verification
- Hub generates random 4-byte nonce
- Device must increment and return encrypted
- Prevents replay attacks

### Message Handling Details

#### INCLUDE_OPEN (Hub → Broadcast)
```cpp
struct IncludeOpenMessage {
    // Empty payload, presence indicates hub is accepting inclusions
};
```

#### INCLUDE_REQUEST (Device → Hub)
```cpp
struct IncludeRequestMessage {
    uint8_t publicKey[32];     // Device's public key
    uint32_t initialCounter;   // Device's initial frame counter
};
```

#### INCLUDE_RESPONSE (Hub → Device)
```cpp
struct IncludeResponseMessage {
    uint8_t hubPublicKey[32];          // Hub's public key
    uint8_t encryptedSessionKey[48];   // ECIES encrypted session key
    uint8_t encryptedNonce[16];        // AES encrypted nonce
    uint32_t hubInitialCounter;        // Hub's initial frame counter
};
```

#### INCLUDE_CONFIRM (Device → Hub)
```cpp
struct IncludeConfirmMessage {
    uint8_t encryptedNonce[16];  // AES encrypted incremented nonce
};
```

#### INCLUDE_SUCCESS (Hub → Device)
```cpp
struct IncludeSuccessMessage {
    // Empty payload, presence confirms successful inclusion
};
```

## Error Handling

### Timeouts
- Device waits 5 seconds for INCLUDE_RESPONSE after sending REQUEST
- Hub waits 5 seconds for INCLUDE_CONFIRM after sending RESPONSE
- Automatic retry with exponential backoff (max 3 attempts)

### Validation Failures
- Invalid device ID format
- Cryptographic verification failures
- Duplicate device detection
- All failures result in silent drop (security consideration)

### State Recovery
- Persistent storage of inclusion state
- Resume from last known good state on restart
- Clear pending states after timeout

## Storage Requirements

### Device Storage
- `"is"`: Inclusion state (1 byte)
- `"sk"`: Session key (32 bytes)
- `"pk"`: Device private key (32 bytes)
- `"hk"`: Hub public key (32 bytes)
- `"mc"`: Message counter (4 bytes)

### Hub Storage
- Per-device records containing:
  - Device ID
  - Device public key
  - Last seen timestamp
  - Message counter

## Configuration Options

### DeviceBuilder Integration
```cpp
device = DeviceBuilder()
    .start()
    .withLoraRadio(radioParams)
    .withSecureMessaging(securityParams)
    .withInclusionCallbacks(onInclusionEvent)  // Optional
    .withAutoInclusion(true)                   // Default: true
    .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::SENSOR);
```

### Inclusion Callback Modes
```cpp
enum InclusionCallbackMode {
    NOTIFY_NONE,     // No callbacks for inclusion messages
    NOTIFY_EVENTS,   // Callbacks for state changes only
    NOTIFY_ALL       // Callbacks for all inclusion messages
};
```

## Testing Strategy

### Unit Tests
1. State machine transitions for both hub and device
2. Cryptographic operations (key generation, ECDH, encryption)
3. Message serialization/deserialization
4. Timeout and retry logic
5. Error condition handling

### Integration Tests
1. Full inclusion sequence simulation
2. Multiple devices joining simultaneously
3. Network disruption scenarios
4. Power loss recovery
5. Security attack scenarios (replay, MITM)

### Example Test Case
```cpp
TEST(InclusionProtocol, BasicInclusionFlow) {
    // Setup hub in inclusion mode
    auto hub = createHub();
    hub->enableInclusionMode(true);

    // Create new device
    auto device = createDevice();
    EXPECT_EQ(device->getInclusionState(), NOT_INCLUDED);

    // Simulate inclusion sequence
    simulateRadioEnvironment(hub, device);

    // Wait for completion
    waitForInclusion(5000);

    // Verify success
    EXPECT_EQ(device->getInclusionState(), INCLUDED);
    EXPECT_TRUE(device->hasSessionKey());
}
```

## Migration Path

### Phase 1: Core Implementation
- Implement automatic message handling in Device class
- Add state machine to InclusionController
- Maintain backward compatibility with rxCallback

### Phase 2: Cryptographic Upgrade
- Replace XOR placeholder with real ECDH
- Implement proper ECIES for key encryption
- Add secure random number generation

### Phase 3: Enhanced Features
- Multiple simultaneous inclusions
- Inclusion history/audit log
- Remote inclusion via gateway
- QR code based pairing

## Security Considerations

1. **No Plain Text Keys**: All keys encrypted during transmission
2. **Forward Secrecy**: New session keys for each inclusion
3. **Replay Protection**: Nonce verification prevents replay attacks
4. **Timing Attacks**: Constant-time crypto operations
5. **DoS Prevention**: Rate limiting on inclusion attempts
6. **Device Authentication**: Validate device IDs against whitelist (optional)

## Future Enhancements

1. **Batch Inclusion**: Include multiple devices simultaneously
2. **Proxy Inclusion**: Allow already-included devices to help with inclusion
3. **Out-of-Band Verification**: QR codes or NFC for initial trust
4. **Revocation**: Remove compromised devices from network
5. **Key Rotation**: Periodic session key updates
6. **Inclusion Profiles**: Different security levels for different device types
