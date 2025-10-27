import time
import json
import threading
import serial
from typing import Dict, Any, Optional, Callable

class SerialManager:
    def __init__(self, port: str, baud: int, timeout: float = 1.0, max_retries: int = 3):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.ser = None
        self.lock = threading.Lock()
        self.max_retries = max_retries
        self.status_callbacks = []
        self.last_error = None
        self.connected = False
        # Initialize the connection
        self._auto_reconnect_thread = None
        self.connect()
        
    def register_status_callback(self, callback: Callable[[bool, Optional[str]], None]) -> None:
        """Register a callback for connection status changes"""
        self.status_callbacks.append(callback)
        
    def _notify_status(self, connected: bool, error_msg: Optional[str] = None) -> None:
        """Notify all callbacks about status changes"""
        self.connected = connected
        self.last_error = error_msg
        for callback in self.status_callbacks:
            try:
                callback(connected, error_msg)
            except Exception as e:
                print(f"Error in status callback: {e}")
        
    def connect(self) -> bool:
        """Establish serial connection with retry logic"""
        with self.lock:
            try:
                # Close if already open
                if self.ser and self.ser.is_open:
                    self.ser.close()
                    time.sleep(0.3)  # Reduced delay for better responsiveness
                    
                # Create new connection
                self.ser = serial.Serial(
                    port=self.port,
                    baudrate=self.baud, 
                    timeout=self.timeout,
                    write_timeout=1.0,  # Add write timeout
                    inter_byte_timeout=0.1  # Improve responsiveness
                )
                time.sleep(0.5)  # Reduced delay for better responsiveness
                
                # Send a test command to verify connection
                self.ser.write(b"\n")
                self.ser.flush()
                
                self._notify_status(True)
                return True
                
            except Exception as e:
                error_msg = f"Serial connection error: {e}"
                print(error_msg)
                self._notify_status(False, error_msg)
                
                # Start auto-reconnect if not already running
                if not self._auto_reconnect_thread or not self._auto_reconnect_thread.is_alive():
                    self._auto_reconnect_thread = threading.Thread(
                        target=self._auto_reconnect, daemon=True)
                    self._auto_reconnect_thread.start()
                    
                return False
                
    def _auto_reconnect(self) -> None:
        """Background thread that attempts to reconnect periodically"""
        retry_delay = 5.0
        while not self.connected:
            time.sleep(retry_delay)
            print(f"Attempting auto-reconnect to {self.port}...")
            if self.connect():
                print("Auto-reconnect successful!")
                break
    
    def send_command(self, cmd: str, expect_json: bool = False, 
                    timeout: float = 2.5) -> Dict[str, Any]:
        """Send command with exclusive access and error handling"""
        # Fast fail if not connected
        if not self.connected and not self.connect():
            return {"error": f"Not connected to {self.port}", "status": "disconnected"}
            
        with self.lock:  # Only one thread can access the port at a time
            retry_count = 0
            
            while retry_count < self.max_retries:
                try:
                    # Clear input buffer for clean response
                    if self.ser and self.ser.in_waiting > 0:
                        self.ser.read(self.ser.in_waiting)
                    
                    # Send command with newline if needed
                    if not cmd.endswith("\n"):
                        cmd += "\n"
                    self.ser.write(cmd.encode("utf-8"))
                    self.ser.flush()
                    
                    # Wait a tiny bit for device to process command
                    time.sleep(0.05)  # Minimal delay for responsiveness
                    
                    # Read response with dynamic timeout
                    response_lines = []
                    end_time = time.time() + timeout
                    
                    while time.time() < end_time:
                        # Check if data available to avoid blocking
                        if self.ser.in_waiting > 0:
                            line = self.ser.readline().decode("utf-8", "ignore").strip()
                            if line:
                                response_lines.append(line)
                                # Stop if we see JSON end marker or complete message
                                if (expect_json and line.endswith("}")) or line == "END":
                                    break
                        else:
                            # Small sleep to avoid CPU spinning while waiting for more data
                            time.sleep(0.01)
                            # If we've waited a bit with no data, consider response complete
                            if response_lines and (time.time() - end_time) > -0.5:
                                break
                    
                    # Process response
                    response = "\n".join(response_lines)
                    if not response:
                        retry_count += 1
                        print(f"No response to '{cmd.strip()}', retrying {retry_count}/{self.max_retries}")
                        time.sleep(0.2 * retry_count)  # Progressive backoff
                        continue
                        
                    # Return parsed response
                    if expect_json:
                        try:
                            return json.loads(response)
                        except json.JSONDecodeError:
                            # Try to fix incomplete JSON responses by adding closing brace
                            if response.count('{') > response.count('}'):
                                try:
                                    return json.loads(response + "}")
                                except:
                                    pass
                            return {"raw": response, "error": "Invalid JSON response"}
                    else:
                        return {"raw": response}
                        
                except serial.SerialException as e:
                    error_msg = f"Serial error: {e}, retrying {retry_count+1}/{self.max_retries}"
                    print(error_msg)
                    self._notify_status(False, error_msg)
                    retry_count += 1
                    self.connect()  # Try to reconnect
                    time.sleep(0.5 * retry_count)  # Progressive backoff
            
            # If we get here, all retries failed
            error_msg = f"Failed after {self.max_retries} attempts"
            self._notify_status(False, error_msg)
            return {"error": error_msg, "status": "failed"}

    def close(self) -> None:
        """Close the serial connection safely"""
        with self.lock:
            if self.ser and self.ser.is_open:
                self.ser.close()
                self._notify_status(False, "Connection closed")