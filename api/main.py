import time
import threading
import serial
from typing import Optional, List, Dict, Any

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import uvicorn

# Serial port configuration
SERIAL_PORT = '/dev/ttyACM0'  # Default Arduino port on Raspberry Pi
BAUDRATE = 115200  # As specified in cmd.txt
TIMEOUT = 1.0

# Initialize FastAPI
app = FastAPI(title="Bondpaper Machine API", version="1.0")

# Configure CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Serial connection manager
class SerialManager:
    def __init__(self, port: str, baud: int, timeout: float = 0.5):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.ser = None
        self.lock = threading.Lock()
        self.connected = False
        self.last_error = None
        self.log_file = "serial_communication.log"
        self.connect()
    
    def connect(self) -> bool:
        with self.lock:
            try:
                # Close if already open
                if self.ser and self.ser.is_open:
                    self.ser.close()
                    self.log_message("SYSTEM", f"Closed existing connection to {self.port}")
                    time.sleep(0.3)
                
                # Create new connection
                self.ser = serial.Serial(
                    port=self.port,
                    baudrate=self.baud,
                    timeout=self.timeout
                )
                time.sleep(0.5)  # Give Arduino time to initialize
                
                # Send a newline to reset Arduino buffer
                self.ser.write(b"\n")
                self.ser.flush()
                
                self.connected = True
                self.last_error = None
                self.log_message("SYSTEM", f"Successfully connected to {self.port} at {self.baud} baud")
                return True
            
            except Exception as e:
                self.last_error = f"Serial connection error: {e}"
                self.log_message("ERROR", f"Connection failed: {e}")
                print(self.last_error)
                self.connected = False
                return False
    
    def send_command(self, cmd: str, wait_for_response: bool = True, timeout: float = 1.0) -> Dict[str, Any]:
        """
        Send a command to Arduino and optionally wait for response
        
        Args:
            cmd: Command to send
            wait_for_response: Whether to wait for a response
            timeout: How long to wait for complete response
            
        Returns:
            Dictionary with response lines or error
        """
        if not self.connected and not self.connect():
            return {"error": f"Not connected to {self.port}"}
        
        with self.lock:
            try:
                # Clear input buffer
                if self.ser.in_waiting > 0:
                    cleared_data = self.ser.read(self.ser.in_waiting).decode('utf-8', 'ignore').strip()
                    if cleared_data:
                        self.log_message("CLEARED", cleared_data)
                
                # Send command with newline if needed
                if not cmd.endswith('\n'):
                    cmd += '\n'
                self.ser.write(cmd.encode('utf-8'))
                self.ser.flush()
                
                # Log the sent command
                self.log_message("SEND", cmd)
                
                if not wait_for_response:
                    return {"success": True}
                
                # Read response with timeout
                response_lines = []
                end_time = time.time() + timeout
                
                while time.time() < end_time:
                    if self.ser.in_waiting > 0:
                        line = self.ser.readline().decode('utf-8', 'ignore').strip()
                        if line:
                            # Log the received response
                            self.log_message("RECV", line)
                            response_lines.append(line)
                            # If we got an error response, we can stop reading
                            if line.startswith("ERR"):
                                break
                            # For commands that return a specific value and don't need more lines
                            if cmd.strip() in ["COINS?", "STATUS?"] and not line.startswith("ERR"):
                                break
                            # For paper command, look for "DONE PAPER" response
                            if cmd.strip().startswith("PAPER") and line == "DONE PAPER":
                                break
                            # For change command, look for "DONE CHANGE" response
                            if cmd.strip().startswith("CHANGE") and line.startswith("DONE CHANGE"):
                                break
                            if cmd.strip().startswith("HOPPER") and line.startswith("DONE HOPPER"):
                                break
                    else:
                        time.sleep(0.01)  # Short sleep to avoid CPU spinning
                        # If we have at least one response and no data for a while, consider done
                        if response_lines and (time.time() - end_time) > -0.5:
                            break
                
                # Process response
                if not response_lines:
                    self.log_message("ERROR", "No response from device")
                    return {"error": "No response from device"}
                
                # Return all response lines
                return {"response": response_lines}
                
            except serial.SerialException as e:
                self.connected = False
                error_msg = f"Serial error: {e}"
                print(error_msg)
                return {"error": error_msg}
    
    def log_message(self, direction: str, message: str) -> None:
        """
        Log a message to the log file
        
        Args:
            direction: "SEND" or "RECV" to indicate direction
            message: The message content
        """
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        try:
            with open(self.log_file, "a") as f:
                log_entry = f"[{timestamp}] {direction}: {message.strip()}\n"
                f.write(log_entry)
        except Exception as e:
            print(f"Error writing to log file: {e}")
                
    def close(self) -> None:
        with self.lock:
            if self.ser and self.ser.is_open:
                self.ser.close()
                self.connected = False
                
    def send_command_with_predicate(self, cmd: str, predicate, timeout: float = 5.0) -> Dict[str, Any]:
        """
        Send a command and wait for a response that satisfies a specific predicate function
        
        Args:
            cmd: Command to send
            predicate: Function that takes a line of text and returns True if it matches the expected response
            timeout: Maximum time to wait in seconds
            
        Returns:
            Dictionary with success status and response or error message
        """
        if not self.connected and not self.connect():
            return {"error": f"Not connected to {self.port}"}
        
        with self.lock:
            try:
                # Clear input buffer
                if self.ser.in_waiting > 0:
                    cleared_data = self.ser.read(self.ser.in_waiting).decode('utf-8', 'ignore').strip()
                    if cleared_data:
                        self.log_message("CLEARED", cleared_data)
                
                # Send command with newline if needed
                if not cmd.endswith('\n'):
                    cmd += '\n'
                self.ser.write(cmd.encode('utf-8'))
                self.ser.flush()
                
                # Log the sent command
                self.log_message("SEND", cmd)
                
                # Now wait for the predicate match
                return self.wait_for_predicate(predicate, timeout)
                
            except serial.SerialException as e:
                self.connected = False
                error_msg = f"Serial error: {e}"
                print(error_msg)
                return {"error": error_msg}
    
    def wait_for_predicate(self, predicate, timeout: float = 5.0) -> Dict[str, Any]:
        """
        Wait for a response that satisfies a specific predicate function
        
        Args:
            predicate: Function that takes a line of text and returns True if it matches the expected response
            timeout: Maximum time to wait in seconds
            
        Returns:
            Dictionary with success status and response or error message
        """
        if not self.connected and not self.connect():
            return {"error": f"Not connected to {self.port}"}
            
        with self.lock:
            try:
                response_lines = []
                end_time = time.time() + timeout
                
                while time.time() < end_time:
                    if self.ser.in_waiting > 0:
                        line = self.ser.readline().decode('utf-8', 'ignore').strip()
                        if line:
                            # Log each received line
                            self.log_message("RECV", line)
                            response_lines.append(line)
                            # Check if this line satisfies our predicate
                            if predicate(line):
                                self.log_message("MATCH", f"Predicate matched on: {line}")
                                return {"success": True, "response": response_lines}
                    else:
                        time.sleep(0.01)  # Short sleep to avoid CPU spinning
                        
                # If we get here, the timeout expired without finding a match
                if response_lines:
                    self.log_message("ERROR", "Expected response not received within timeout")
                    return {"error": "Expected response not received within timeout", "partial_response": response_lines}
                else:
                    self.log_message("ERROR", "No response from device within timeout")
                    return {"error": "No response from device within timeout"}
                    
            except serial.SerialException as e:
                self.connected = False
                error_msg = f"Serial error: {e}"
                print(error_msg)
                return {"error": error_msg}

# Initialize SerialManager
serial_manager = SerialManager(SERIAL_PORT, BAUDRATE, TIMEOUT)

# API Routes
@app.get("/status")
def get_status():
    """Get system status"""
    result = serial_manager.send_command("STATUS?")
    
    if "error" in result:
        return {"error": result["error"]}
    
    # Return the raw status response
    return {"status": result.get("response", [""])[0]}

@app.get("/coins")
def get_coin_count():
    """Get current inserted coins"""
    result = serial_manager.send_command("COINS?")
    
    if "error" in result:
        return {"error": result["error"]}
    
    try:
        # Try to convert the response to an integer
        coins = int(result.get("response", ["0"])[0])
        return {"coins": coins}
    except ValueError:
        return {"error": "Invalid response", "raw": result.get("response", [])}

@app.post("/coins/reset")
def reset_coin_count():
    """Reset coin counter to zero"""
    result = serial_manager.send_command("COINS=RST", wait_for_response=False)
    
    if "error" in result:
        return {"error": result["error"]}
    
    return {"success": True, "message": "Coin counter reset"}

@app.post("/hopper/{denomination}/{count}")
def dispense_hopper(denomination: int, count: int):
    """
    Dispense specific coins from hopper
    
    Args:
        denomination: Coin value (1, 5, or 10)
        count: Number of coins to dispense
    """

    if denomination not in [1, 5, 10]:
        raise HTTPException(status_code=400, detail="Denomination must be 1, 5, or 10")
    
    if count <= 0:
        raise HTTPException(status_code=400, detail="Count must be positive")

    serial_manager.send_command("RESET", wait_for_response=False)
    time.sleep(1.0)  # Give the device time to reset
    
    command = f"HOPPER {denomination} {count}"
    result = serial_manager.send_command(command, wait_for_response=True, timeout=20.0)  # Longer timeout for dispensing
    
    if "error" in result:
        return {"error": result["error"]}
    
    # Return all response lines as they indicate progress and completion
    return {"response": result.get("response", [])}

def calculate_change_breakdown(amount: int) -> Dict[int, int]:
    """
    Calculate how many coins of each denomination to dispense
    
    Args:
        amount: Total amount to dispense
    
    Returns:
        Dictionary with denomination as key and count as value
    """
    result = {}
    
    # Start with largest denomination
    denominations = [10, 5, 1]
    remaining = amount
    
    for denom in denominations:
        if remaining >= denom:
            count = remaining // denom
            result[denom] = count
            remaining -= count * denom
    
    return result

@app.post("/change/{amount}")
async def dispense_change(amount: int):
    """
    Calculate and dispense change using coin hoppers
    
    This endpoint:
    1. Validates the amount
    2. Resets the device 
    3. Calculates how many coins of each type are needed
    4. Dispenses coins from largest to smallest denomination
    5. Falls back to smaller denominations if larger denominations fail
    6. Recursively handles remaining amounts until fully dispensed or all options exhausted
    7. Returns the results or an error
    
    Args:
        amount: Total change amount to dispense in coins
    """
    # Validate input
    if amount <= 0:
        raise HTTPException(status_code=400, detail="Amount must be positive")
    
    # Step 1: Reset the device to ensure clean state
    serial_manager.send_command("RESET", wait_for_response=False)
    time.sleep(1.0)  # Give the device time to reset
    
    # Track how much we've successfully dispensed
    amount_dispensed = 0
    remaining_amount = amount
    all_responses = []
    
    # Keep track of which denominations are available/working
    available_denominations = [10, 5, 1]
    
    # Continue dispensing until there's nothing left or we've tried all options
    while remaining_amount > 0 and available_denominations:
        # Calculate how many of each available coin we need
        coin_counts = {}
        temp_remaining = remaining_amount
        
        for denom in sorted(available_denominations, reverse=True):
            if temp_remaining >= denom:
                count = temp_remaining // denom
                coin_counts[denom] = count
                temp_remaining -= count * denom
        
        # If we couldn't create any valid breakdown with available denominations, break
        if not coin_counts:
            all_responses.append(f"Cannot make change for remaining {remaining_amount}₱ with available denominations")
            break
        
        # Try to dispense the calculated coins
        dispensed_this_round = False
        
        # *** Keep track of which denominations have been tried in this round
        tried_denominations = []
        
        for coin_value in sorted(coin_counts.keys(), reverse=True):
            num_coins = coin_counts[coin_value]
            
            if num_coins > 0:
                command = f"HOPPER {coin_value} {num_coins}"
                result = serial_manager.send_command(command, wait_for_response=True, timeout=10.0)
                
                # Add this denomination to tried list
                tried_denominations.append(coin_value)
                
                coins_dispensed = 0
                if "error" not in result and "response" in result:
                    responses = result.get("response", [])
                    all_responses.extend(responses)
                    
                    # Count actual coins dispensed
                    for line in responses:
                        if f"OUT {coin_value}" in line:
                            coins_dispensed += 1
                    
                    # Update amounts
                    amount_dispensed += coins_dispensed * coin_value
                    remaining_amount -= coins_dispensed * coin_value
                    
                    if coins_dispensed > 0:
                        dispensed_this_round = True
                    
                    # If we dispensed fewer coins than requested, this denomination may be empty/faulty
                    if coins_dispensed < num_coins:
                        all_responses.append(f"Warning: Only dispensed {coins_dispensed}/{num_coins} coins of {coin_value}₱")
                        if coins_dispensed == 0:
                            # Remove this denomination from available options
                            all_responses.append(f"Removing {coin_value}₱ from available denominations")
                            available_denominations.remove(coin_value)
                else:
                    error_msg = f"Error dispensing {num_coins} x {coin_value}₱ coins: {result.get('error', 'Unknown error')}"
                    all_responses.append(error_msg)
                    # Remove this denomination from available options
                    all_responses.append(f"Removing {coin_value}₱ from available denominations")
                    if coin_value in available_denominations:
                        available_denominations.remove(coin_value)
                
                time.sleep(1.0)  # Wait before next dispense operation
        
        # *** Change this check to verify if we've tried all available denominations
        # If we tried all available denominations but couldn't dispense anything, we're stuck
        if not dispensed_this_round and set(tried_denominations) == set(available_denominations):
            all_responses.append(f"Failed to dispense any coins for remaining {remaining_amount}₱")
            break
    
    # Step 5: Check if we got any response at all
    if not all_responses:
        raise HTTPException(
            status_code=500, 
            detail="No responses received from device while dispensing change"
        )
    
    # Add a completion message
    if remaining_amount == 0:
        all_responses.append("DONE CHANGE")
        serial_manager.send_command("DONE CHANGE", wait_for_response=False)
    
    # Step 6: Return the results with actual amount dispensed
    return {
        "requested_amount": amount,
        "dispensed_amount": amount_dispensed,  # What we actually dispensed
        "remaining_amount": remaining_amount,  # What we failed to dispense
        "responses": all_responses
    }

@app.get("/paper/{paper_type}")
def check_paper(paper_type: str):
    """
    Check if paper is present
    
    Args:
        paper_type: SHORT or LONG
    
    Returns:
        Dictionary with has_paper boolean and status message
    """
    paper_type = paper_type.upper()
    if paper_type not in ["SHORT", "LONG"]:
        raise HTTPException(status_code=400, detail="Paper type must be SHORT or LONG")
    
    command = f"PAPER? {paper_type}"
    result = serial_manager.send_command(command, timeout=2.0)
    
    if "error" in result:
        return {"error": result["error"]}
    
    responses = result.get("response", [])
    
    # Look for the 1 or 0 response
    has_paper = None
    for line in responses:
        if line == "1":
            has_paper = True
        elif line == "0":
            has_paper = False
    
    return {
        "paper_type": paper_type,
        "has_paper": has_paper,
        "responses": responses
    }

@app.post("/paper/{paper_type}/{count}")
def dispense_paper(paper_type: str, count: int):
    """
    Dispense paper sheets
    
    Args:
        paper_type: SHORT or LONG
        count: Number of sheets to dispense
    """
    paper_type = paper_type.upper()
    if paper_type not in ["SHORT", "LONG"]:
        raise HTTPException(status_code=400, detail="Paper type must be SHORT or LONG")
    
    if count <= 0:
        raise HTTPException(status_code=400, detail="Count must be positive")
    
    command = f"PAPER {paper_type} {count}"
    result = serial_manager.send_command(command, timeout=50.0)  # Use a much longer timeout for paper dispensing
    
    if "error" in result:
        return {"error": result["error"]}
    
    # Return all response lines as they indicate progress and completion
    return {"response": result.get("response", [])}

@app.post("/stop")
def emergency_stop():
    """Emergency stop for all hopper operations"""
    result = serial_manager.send_command("STOP", wait_for_response=True)
    
    if "error" in result:
        return {"error": result["error"]}
    
    return {"success": True, "message": "Emergency stop sent"}

@app.post("/reset")
def reset_device():
    """Reset all device state - use before starting a new transaction"""
    result = serial_manager.send_command("RESET", wait_for_response=True)
    
    if "error" in result:
        return {"error": result["error"]}
    
    return {"success": True, "message": "Device reset completed"}

@app.post("/coinslot/{action}")
def control_coinslot(action: str):
    """
    Enable or disable the coin slot acceptor
    
    Args:
        action: "on" to enable coin acceptance, "off" to disable and reset counter
    
    Returns:
        Success message or error
    """
    action = action.upper()
    if action not in ["ON", "OFF"]:
        raise HTTPException(status_code=400, detail="Action must be 'on' or 'off'")
    
    command = f"COINSLOT {action}"
    result = serial_manager.send_command(command, wait_for_response=True, timeout=2.0)
    
    if "error" in result:
        return {"error": result["error"]}
    
    responses = result.get("response", [])
    
    # Check for the expected response
    expected_response = f"OK COINSLOT {'ENABLED' if action == 'ON' else 'DISABLED'}"
    success = any(expected_response in line for line in responses)
    
    return {
        "success": success,
        "action": action.lower(),
        "enabled": action == "ON",
        "responses": responses
    }

@app.get("/logs")
def get_logs(lines: int = 50):
    """Get the most recent serial communication logs
    
    Args:
        lines: Number of most recent lines to fetch (default: 50)
    """
    try:
        with open(serial_manager.log_file, "r") as f:
            all_lines = f.readlines()
            # Get the last N lines
            recent_lines = all_lines[-lines:] if len(all_lines) > lines else all_lines
            return {"logs": recent_lines}
    except Exception as e:
        return {"error": f"Error reading log file: {e}"}

@app.get("/health")
def health_check():
    """API health check endpoint"""
    serial_status = "connected" if serial_manager.connected else "disconnected"
    
    if serial_manager.connected:
        # Try to get status from Arduino
        status_cmd = serial_manager.send_command("STATUS?")
        if "error" not in status_cmd and "response" in status_cmd:
            arduino_status = status_cmd["response"][0]
        else:
            arduino_status = "unknown"
    else:
        arduino_status = "disconnected"
    
    return {
        "status": "online",
        "serial": serial_status,
        "arduino": arduino_status,
        "error": serial_manager.last_error
    }

# Application startup and shutdown events
@app.on_event("startup")
async def startup_event():
    """Initialize components on startup"""
    # Ensure serial connection is established
    if not serial_manager.connected:
        serial_manager.connect()

@app.on_event("shutdown")
async def shutdown_event():
    """Clean up resources on shutdown"""
    serial_manager.close()

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)