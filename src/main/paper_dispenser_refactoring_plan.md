## PaperDispenser Refactoring Plan

### Current Analysis

The PaperDispenser class currently handles serial communication directly with several methods:

- `sendStatusJson()`
- `sendEventJson()`
- `sendErrorJson()`
- `sendAckJson()`

These methods serialize JSON objects and write directly to Serial. It also has a `handleSerial()` method that receives and processes commands from the serial line.

### Refactoring Goals

1. Remove all direct Serial writing from the PaperDispenser class
2. Add status tracking flags similar to other modules
3. Replace handleSerial with processCommand method
4. Update main.ino to handle all PaperDispenser serial communication

### Changes Required

#### 1. Update StepperMotor.h:

- Add status tracking flags:

  ```cpp
  // Status tracking flags
  bool m_bStatusUpdated;     // Indicates a status update is available
  bool m_bEventOccurred;     // Indicates an event has occurred
  const char* m_pLastEvent;  // The last event that occurred
  const char* m_pLastError;  // The last error that occurred
  const char* m_pLastErrorDetails; // Details about the last error
  ```

- Add methods for checking and clearing flags:

  ```cpp
  // Status flag methods
  bool hasStatusUpdate() const;
  bool hasEvent() const;
  bool hasError() const;
  void clearStatusUpdate();
  void clearEvent();
  void clearError();

  // Status accessors
  const char* getLastEvent() const;
  const char* getLastError() const;
  const char* getLastErrorDetails() const;
  DispenserState getState() const;
  ```

- Add processCommand method:

  ```cpp
  bool processCommand(const String& line, JsonDocument& doc);
  ```

- Remove the serial output methods (or repurpose them to set status flags instead)

#### 2. Update StepperMotor.cpp:

- Update the constructor to initialize new flags
- Modify all methods that call Serial to set status flags instead
- Implement the new status accessor methods
- Implement processCommand() method to replace handleSerial()
- Update code to set status flags instead of writing to Serial directly

#### 3. Update main.ino:

- Update processSerialLine() to use PaperDispenser's processCommand()
- Add status checking for PaperDispenser in the main loop
- Send appropriate messages when flags are set
- Add SerialComm methods for PaperDispenser messages if needed

### Implementation Strategy

1. First, add the status tracking flags and methods to StepperMotor.h
2. Implement new status methods in StepperMotor.cpp
3. Refactor all Serial-writing methods to set flags instead
4. Add support for PaperDispenser in SerialComm if not already present
5. Update main.ino to handle PaperDispenser serial communication
6. Test the changes thoroughly
