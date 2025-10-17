import time
import threading
import serial
from typing import Optional, List, Dict, Any

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import uvicorn

# Serial port configuration
SERIAL_PORT = '/dev/ttyACM0'  # Default Arduino port on Raspberry Pi
BAUDRATE = 9600  # As specified in cmd.txt
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
    def __init__(self, port: str, baud: int, timeout: float = 1.0):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.ser = None
        self.lock = threading.Lock()
        self.connected = False
        self.last_error = None
        self.connect()
    
    def connect(self) -> bool:
        with self.lock:
            try:
                # Close if already open
                if self.ser and self.ser.is_open:
                    self.ser.close()
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
                return True
            
            except Exception as e:
                self.last_error = f"Serial connection error: {e}"
                print(self.last_error)
                self.connected = False
                return False
    
    def send_command(self, cmd: str, wait_for_response: bool = True, timeout: float = 2.0) -> Dict[str, Any]:
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
                    self.ser.read(self.ser.in_waiting)
                
                # Send command with newline if needed
                if not cmd.endswith('\n'):
                    cmd += '\n'
                self.ser.write(cmd.encode('utf-8'))
                self.ser.flush()
                
                if not wait_for_response:
                    return {"success": True}
                
                # Read response with timeout
                response_lines = []
                end_time = time.time() + timeout
                
                while time.time() < end_time:
                    if self.ser.in_waiting > 0:
                        line = self.ser.readline().decode('utf-8', 'ignore').strip()
                        if line:
                            response_lines.append(line)
                            # If we got an error response, we can stop reading
                            if line.startswith("ERR"):
                                break
                            # For commands that return a specific value and don't need more lines
                            if cmd.strip() in ["COINS?", "STATUS?"] and not line.startswith("ERR"):
                                break
                    else:
                        time.sleep(0.01)  # Short sleep to avoid CPU spinning
                        # If we have at least one response and no data for a while, consider done
                        if response_lines and (time.time() - end_time) > -0.5:
                            break
                
                # Process response
                if not response_lines:
                    return {"error": "No response from device"}
                
                # Return all response lines
                return {"response": response_lines}
                
            except serial.SerialException as e:
                self.connected = False
                error_msg = f"Serial error: {e}"
                print(error_msg)
                return {"error": error_msg}
    
    def close(self) -> None:
        with self.lock:
            if self.ser and self.ser.is_open:
                self.ser.close()
                self.connected = False

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
    
    command = f"HOPPER {denomination} {count}"
    result = serial_manager.send_command(command, timeout=5.0)  # Longer timeout for dispensing
    
    if "error" in result:
        return {"error": result["error"]}
    
    # Return all response lines as they indicate progress and completion
    return {"response": result.get("response", [])}

@app.post("/change/{amount}")
def dispense_change(amount: int):
    """
    Automatically calculate and dispense change
    
    Args:
        amount: Total change amount to dispense
    """
    if amount <= 0:
        raise HTTPException(status_code=400, detail="Amount must be positive")
    
    command = f"CHANGE {amount}"
    result = serial_manager.send_command(command, timeout=10.0)  # Longer timeout for change dispensing
    
    if "error" in result:
        return {"error": result["error"]}
    
    # Return all response lines as they indicate progress and completion
    return {"response": result.get("response", [])}

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
    result = serial_manager.send_command(command, wait_for_response=True, timeout=5.0)
    
    if "error" in result:
        return {"error": result["error"]}
    
    # Paper command may not return a response unless there's an error
    return {"success": True, "message": f"Dispensed {count} sheets of {paper_type} paper"}

@app.post("/stop")
def emergency_stop():
    """Emergency stop for all hopper operations"""
    result = serial_manager.send_command("STOP", wait_for_response=False)
    
    if "error" in result:
        return {"error": result["error"]}
    
    return {"success": True, "message": "Emergency stop sent"}

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