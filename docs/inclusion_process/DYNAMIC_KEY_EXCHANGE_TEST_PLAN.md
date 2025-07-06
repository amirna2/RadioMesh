# Dynamic Key Exchange Test Plan

## Overview
This document outlines the test cases for the dynamic key exchange feature implemented in the RadioMesh project.

## Test Categories

### 1. Unit Tests for updateSecurityParams

#### test_updateSecurityParams_success
- **Purpose**: Verify that security parameters can be updated successfully
- **Setup**: 
  - Create a device with initial security parameters
  - Initialize crypto system
- **Test**:
  - Call updateSecurityParams with new parameters
  - Verify return code is RM_E_NONE
- **Expected**: Security parameters updated successfully

#### test_updateSecurityParams_noCrypto
- **Purpose**: Verify error handling when crypto is not initialized
- **Setup**: 
  - Create a device without initializing crypto
- **Test**:
  - Call updateSecurityParams
  - Verify return code is RM_E_INVALID_STATE
- **Expected**: Function returns error when crypto not initialized

### 2. Integration Tests for Inclusion with Key Exchange

#### test_inclusion_sessionKeyApplied_device
- **Purpose**: Verify session key is applied on device after successful inclusion
- **Setup**:
  - Create hub and standard device
  - Put hub in inclusion mode
  - Mock key exchange during inclusion
- **Test**:
  - Complete inclusion protocol
  - Verify device's crypto is updated with session key
- **Expected**: Device uses session key after inclusion

#### test_inclusion_sessionKeyApplied_hub
- **Purpose**: Verify hub applies session key after generating it
- **Setup**:
  - Create hub device
  - Put hub in inclusion mode
- **Test**:
  - Hub receives INCLUDE_REQUEST
  - Hub generates and sends session key
  - Verify hub's crypto is updated
- **Expected**: Hub uses same session key as device

### 3. Persistence Tests

#### test_sessionKey_persistsAcrossReboot
- **Purpose**: Verify session key is loaded on device startup
- **Setup**:
  - Create device that was previously included
  - Mock storage with saved session key
- **Test**:
  - Initialize device
  - Verify loadAndApplySessionKey is called
  - Verify crypto is updated with stored key
- **Expected**: Device loads and applies session key on startup

#### test_sessionKey_loadFailure
- **Purpose**: Verify graceful handling when session key loading fails
- **Setup**:
  - Create device marked as included
  - Mock storage to return error on key load
- **Test**:
  - Initialize device
  - Verify device continues initialization
  - Verify warning is logged
- **Expected**: Device initializes but warns about key load failure

### 4. DeviceBuilder Tests

#### test_deviceBuilder_withoutSecurity
- **Purpose**: Verify device can be built without initial security
- **Setup**:
  - Use DeviceBuilder without calling withSecureMessaging
- **Test**:
  - Build device
  - Verify device is created successfully
  - Verify crypto can be initialized later
- **Expected**: Device builds without crypto, ready for inclusion

#### test_deviceBuilder_optionalSecurity
- **Purpose**: Verify backwards compatibility with existing security setup
- **Setup**:
  - Use DeviceBuilder with withSecureMessaging
- **Test**:
  - Build device with hardcoded keys
  - Verify crypto is initialized
  - Verify updateSecurityParams can still update keys
- **Expected**: Existing security setup still works

### 5. End-to-End Tests

#### test_e2e_dynamicKeyExchange
- **Purpose**: Full inclusion flow with dynamic key exchange
- **Setup**:
  - Create hub without hardcoded keys
  - Create standard device without hardcoded keys
- **Test**:
  1. Hub enters inclusion mode
  2. Hub sends INCLUDE_OPEN
  3. Device sends INCLUDE_REQUEST with public key
  4. Hub generates session key and sends INCLUDE_RESPONSE
  5. Device decrypts session key and sends INCLUDE_CONFIRM
  6. Hub sends INCLUDE_SUCCESS
  7. Both devices update crypto with session key
  8. Verify encrypted communication works
- **Expected**: Devices establish secure communication without hardcoded keys

### 6. Error Handling Tests

#### test_inclusion_invalidPublicKey
- **Purpose**: Verify handling of invalid public key during inclusion
- **Setup**:
  - Create hub and device
  - Mock invalid public key in INCLUDE_REQUEST
- **Test**:
  - Send INCLUDE_REQUEST with wrong key size
  - Verify hub rejects request
- **Expected**: Hub returns RM_E_INVALID_LENGTH

#### test_inclusion_decryptionFailure
- **Purpose**: Verify handling when session key decryption fails
- **Setup**:
  - Create hub and device
  - Mock decryption failure
- **Test**:
  - Device receives INCLUDE_RESPONSE
  - Decryption of session key fails
  - Verify device handles error gracefully
- **Expected**: Device logs error and remains in pending state

## Test Implementation Guidelines

1. Use Unity test framework (as seen in existing tests)
2. Mock storage and crypto operations where needed
3. Test both positive and negative scenarios
4. Verify log messages for important operations
5. Ensure tests are independent and repeatable

## Running Tests

```bash
# Run all dynamic key exchange tests
./tools/builder.py test -t test_heltec_wifi_lora_32_V3 -s test_DynamicKeyExchange

# Run specific test category
./tools/builder.py test -t test_heltec_wifi_lora_32_V3 -s test_Device
```

## Coverage Goals

- Line coverage: >90% for new code
- Branch coverage: Test all error paths
- Integration coverage: Test full inclusion flow