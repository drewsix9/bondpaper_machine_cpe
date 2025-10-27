# Paper Out Detection Documentation

## Overview

This document describes the implementation of paper out detection in the bond paper vending machine system.

## Arduino Implementation (`PaperDispenser.cpp`)

### Error Conditions

The Arduino firmware detects three possible paper out conditions:

1. **Initial Paper Out** (`ERR PAPER_OUT`)

   - Occurs during the `homeToLimit()` function
   - When the limit switch is not triggered within 10 seconds
   - Indicates that the paper tray is empty before dispensing begins

2. **Cannot Start Dispensing** (`ERR PAPER_OUT_CANNOT_DISPENSE`)

   - Occurs at the start of `dispenseSheets()`
   - When `homeToLimit()` fails to find paper
   - Indicates that no paper is available to dispense

3. **Paper Out During Cycle** (`ERR PAPER_OUT_DURING_CYCLE`)
   - Occurs during the dispensing cycle
   - When nudging forward fails to trigger the limit switch
   - Indicates that paper ran out in the middle of a dispensing operation
   - Also reports how many sheets were successfully dispensed: `SHEETS_DISPENSED: N`

### Implementation Details

1. `homeToLimit()` has been modified to:

   - Return a boolean (true = success, false = paper out)
   - Send an error message via Serial when paper out is detected

2. `dispenseSheets()` has been modified to:
   - Check the return value from `homeToLimit()`
   - Detect paper out conditions during the dispensing cycle
   - Send appropriate error messages and sheet count information

## Raspberry Pi API Implementation (`main.py`)

The `/paper/{paper_type}/{count}` endpoint has been enhanced to:

1. Detect paper out error messages in the Arduino response
2. Return detailed information about the error condition
3. Include a `paper_out` boolean flag in the response
4. Report how many sheets were successfully dispensed before failure

### API Response Format

#### Normal Success

```json
{
  "success": true,
  "paper_out": false,
  "sheets_dispensed": 10,
  "response": ["DONE PAPER"]
}
```

#### Paper Out Error

```json
{
  "error": "Paper tray is empty",
  "paper_out": true,
  "sheets_dispensed": 0,
  "sheets_requested": 10,
  "response": ["ERR PAPER_OUT"]
}
```

#### Partial Dispensing Error

```json
{
  "error": "Paper ran out during dispensing",
  "paper_out": true,
  "sheets_dispensed": 3,
  "sheets_requested": 10,
  "response": ["ERR PAPER_OUT_DURING_CYCLE", "SHEETS_DISPENSED: 3"]
}
```

## Frontend Usage

The frontend application should check for the `paper_out` field in the response and show an appropriate message to the user when it is `true`.
