# RadioMesh MIC Authentication Requirements

## Overview

This document defines the requirements for adding Message Integrity Check (MIC) authentication to RadioMesh packets using AES-CMAC. The MIC provides cryptographic authentication and integrity protection for all mesh communications, ensuring only devices with the network key can send valid messages.

## Design Goals

1. **Cryptographic Authentication**: Prove packets originate from devices possessing the network key
2. **Full Packet Integrity**: Protect both header and payload against tampering
3. **Mesh Compatibility**: Enable any device to verify packets for relay decisions
4. **Performance Efficiency**: Minimize computational overhead on constrained devices
5. **Backward Compatibility**: Support migration from non-MIC to MIC-enabled networks

## Packet Structure

### Current Structure (256 bytes total)
```
[Header (35 bytes)] + [Payload (up to 221 bytes)]
```

### New Structure with MIC (256 bytes total)
```
[Header (35 bytes)] + [Encrypted Payload (up to 217 bytes)] + [MIC (4 bytes)]
```

**Changes**:
- Maximum payload size reduced from 221 to 217 bytes
- 4-byte MIC appended after payload
- Total packet size remains 256 bytes

## MIC Computation

### Algorithm
- **Method**: AES-CMAC (RFC 4493)
- **Key**: 32-byte shared network key
- **Input**: Complete header (35 bytes) + encrypted payload (variable length)
- **Output**: 128-bit CMAC truncated to 32 bits (4 bytes)

### Computation Formula
```
Full_CMAC = AES_CMAC(NetworkKey, Header || EncryptedPayload)
MIC = Truncate_To_4_Bytes(Full_CMAC)
```

### Header Fields Included in MIC
All header fields are included in MIC computation:
- Protocol Version (1 byte)
- Source Device ID (4 bytes)
- Destination Device ID (4 bytes)
- Packet ID (4 bytes)
- Topic (1 byte)
- Device Type (1 byte)
- Hop Count (1 byte) - Note: Changes during relay
- Data CRC (4 bytes)
- Frame Counter (4 bytes)
- Last Hop ID (4 bytes) - Note: Changes during relay
- Next Hop ID (4 bytes) - Note: Changes during relay
- Reserved (3 bytes)

## Message Processing Flows

### Sending a Packet
1. Construct packet with header and data
2. Encrypt payload (if encryption is enabled)
3. Compute MIC over header + encrypted payload
4. Append 4-byte MIC to end of packet
5. Transmit complete packet (header + encrypted payload + MIC)

### Receiving a Packet
1. Receive complete packet
2. Extract last 4 bytes as received MIC
3. Compute expected MIC over header + payload (excluding extracted MIC)
4. Compare received MIC with computed MIC
5. **If MIC verification fails**:
   - Drop packet immediately
   - Log security violation
   - Do NOT decrypt or process packet
6. **If MIC verification succeeds**:
   - Remove MIC bytes from packet
   - Decrypt payload (if encrypted)
   - Process packet according to topic and routing

### Relaying a Packet
1. Perform standard receive verification (steps 1-5 above)
2. If packet needs forwarding:
   - Update mutable header fields:
     - Increment hop count
     - Set last hop ID to current device
     - Update next hop ID based on routing table
   - Recompute MIC with updated header
   - Replace old MIC with new MIC
   - Forward packet

## Security Considerations

### MIC Properties
- **Authentication**: Only devices with the network key can generate valid MICs
- **Integrity**: Any modification to header or payload invalidates the MIC
- **Non-repudiation**: Within the network, packets can be traced to key holders
- **Replay Protection**: Combined with frame counters, prevents replay attacks

### Early Verification Benefits
- MIC verification before decryption saves CPU cycles on invalid packets
- Prevents processing of malicious or corrupted packets
- Reduces attack surface by dropping unauthenticated traffic early

### Key Management
- MIC uses the same network key distributed during device inclusion
- Key rotation requires updating all devices with new network key
- Compromised key allows forgery of MICs (standard shared-key limitation)

## Implementation Requirements

### Crypto Layer
1. Implement AES-CMAC algorithm (RFC 4493)
2. Support 32-bit truncated output
3. Optimize for embedded platforms
4. Integrate with existing AES infrastructure

### Packet Layer
1. Update MAX_DATA_LENGTH constant from 221 to 217
2. Add MIC extraction and append methods to RadioMeshPacket
3. Maintain packet parsing compatibility

### Routing Layer
1. Add MIC computation to packet send path
2. Add MIC verification to packet receive path
3. Implement MIC recomputation for relay path
4. Handle MIC verification failures gracefully

### Protocol Layer
1. Increment protocol version to indicate MIC support
2. Add MIC-aware packet processing
3. Ensure backward compatibility during migration

## Performance Requirements

### Computational Overhead
- MIC computation: < 1ms on ESP32 platforms
- MIC verification: < 1ms on ESP32 platforms
- Total throughput impact: < 5% reduction

### Memory Requirements
- AES-CMAC implementation: < 2KB code space
- Runtime memory: Reuse existing AES buffers
- No additional heap allocation required

## Migration Strategy

### Protocol Version Handling
- Increment RM_PROTOCOL_VERSION from 3 to 4
- Version 3: No MIC support
- Version 4: MIC required on all packets

### Compatibility Modes
1. **Pure v3 Network**: All devices without MIC (existing deployments)
2. **Mixed Network**: Hub accepts both v3 and v4 during migration
3. **Pure v4 Network**: All devices require MIC (target state)

### Migration Process
1. Update hub firmware to support both v3 and v4
2. Gradually update device firmware to v4
3. After all devices updated, hub enforces v4 only
4. Document provides 6-month migration window

## Testing Requirements

### Unit Tests
- AES-CMAC implementation correctness
- MIC computation with test vectors
- Truncation and padding edge cases
- Performance benchmarks

### Integration Tests
- End-to-end packet flow with MIC
- Relay path MIC recomputation
- Invalid MIC rejection
- Mixed version compatibility

### Security Tests
- MIC forgery attempts
- Replay attack prevention
- Bit-flip detection
- Key mismatch handling

## Success Criteria

1. All v4 packets include valid 4-byte MIC
2. Invalid MICs cause immediate packet drops
3. Relay operations maintain MIC integrity
4. Performance impact within 5% threshold
5. Smooth migration from v3 to v4 networks

## Future Enhancements

1. **Extended MIC**: Option for 8-byte MIC for higher security
2. **Per-Message Keys**: Derive unique keys per message type
3. **Selective Authentication**: Skip MIC for specific low-value topics
4. **Hardware Acceleration**: Utilize crypto hardware when available

---

**Document Status**: Final  
**Version**: 1.0  
**Last Updated**: 2025-01-14  
**Author**: RadioMesh Development Team