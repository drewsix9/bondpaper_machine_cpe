# Serial Communication Refactoring Plan

## Current Analysis

The codebase currently has:

- Each module contains its own `handleSerial()` function that both processes commands and sends responses
- JSON serialization/deserialization is distributed across modules
- Modules directly write to Serial when they need to send status updates, events, errors, etc.
- The main.ino file routes commands to modules but doesn't centralize response handling

This is causing delayed responses during API testing and potentially mixing up message sequences.

## Refactoring Goals

1. Centralize all Serial reading in main.ino (already done)
2. Centralize all Serial writing in main.ino (main change needed)
3. Create a message passing system between main and modules
4. Maintain the same functionality while improving response timing

## Implementation Plan

### 1. Create Message Structure

Define a centralized message structure for modules to communicate with main.ino:

```cpp
struct SerialMessage {
  String source;        // Module name (e.g., "CoinCounter", "System")
  String type;          // Message type (e.g., "status", "event", "ack", "error")
  String data;          // JSON string containing the message data
  unsigned long timestamp;  // Timestamp when the message was created
};
```

### 2. Modify Module Headers

For each module class (CoinCounter, CoinSlotISR, RelayHopper, PaperDispenser):

1. Remove functions that directly write to Serial: sendStatusJson(), sendEventJson(), sendAckJson(), etc.
2. Add new functions that return message objects instead

For example, convert:

```cpp
void sendStatusJson();
```

To:

```cpp
SerialMessage getStatusMessage();
```

### 3. Update Module CPP Files

1. Replace all Serial.print/Serial.println calls with message generation
2. Change command handling to return response objects rather than directly outputting to Serial
3. Return appropriate message objects from different events (status updates, errors, etc.)

### 4. Implement Central Serial Handler in main.ino

1. Create a queue or buffer for outgoing messages
2. Implement a sendSerialMessage() function to handle all serial output
3. Implement priority handling for different message types
4. Update the main loop to process and send messages from the queue

### 5. Modify Command Routing in main.ino

1. Update processSerialLine() to route commands as before
2. Collect response messages from modules instead of letting them write directly
3. Queue response messages for sending

### 6. Additional Improvements

1. Implement message deduplication to avoid flooding with similar messages
2. Add timing mechanisms to ensure timely response to commands
3. Add error handling for malformed messages

## Testing Approach

1. Test command-by-command to ensure each command works with the new architecture
2. Test rapid command sequences to verify correct order of responses
3. Test error conditions and recovery
4. Measure response times to verify improvement over the original implementation
