#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * SerialMessage - Structure to hold message data for centralized serial communication
 * This is used by modules to pass messages to the main program for serial output
 */
struct SerialMessage {
  String source;        // Module name (e.g., "CoinCounter", "System")
  String type;          // Message type (e.g., "status", "event", "ack", "error")
  JsonDocument* data;   // Pointer to JSON document containing the message data
  unsigned long timestamp;  // Timestamp when the message was created
  
  // Constructor
  SerialMessage(String src, String t, JsonDocument* d) : 
    source(src), type(t), data(d), timestamp(millis()) {}
};

/**
 * SerialComm - Centralized serial communication manager
 * This class handles all serial communication for the entire system
 */
class SerialComm {
public:
  // Initialize serial communication
  static void begin(long baudRate) {
    Serial.begin(baudRate);
    delay(100);
  }
  
  // Send a message to the serial port
  static void sendMessage(const SerialMessage& message) {
    // If the message contains a valid JSON document
    if (message.data != nullptr) {
      // Create a fresh document for the final output
      StaticJsonDocument<512> outputDoc;
      
      // Set standard fields
      outputDoc["v"] = 1;
      outputDoc["source"] = message.source;
      outputDoc["type"] = message.type;
      outputDoc["ts"] = message.timestamp;
      
      // Copy the data object from the message to the output
      JsonObject dataObj = outputDoc.createNestedObject("data");
      
      // Iterate through the message data and copy to the output data object
      for (JsonPair p : message.data->as<JsonObject>()) {
        // Copy each field from source to destination
        JsonVariant value = p.value();
        if (value.is<JsonObject>()) {
          // Recursively copy nested objects
          JsonObject nestedObj = dataObj.createNestedObject(p.key().c_str());
          for (JsonPair np : value.as<JsonObject>()) {
            nestedObj[np.key()] = np.value();
          }
        } else if (value.is<JsonArray>()) {
          // Copy arrays
          JsonArray srcArray = value.as<JsonArray>();
          JsonArray dstArray = dataObj.createNestedArray(p.key().c_str());
          for (JsonVariant v : srcArray) {
            dstArray.add(v);
          }
        } else {
          // Copy primitive values
          dataObj[p.key()] = value;
        }
      }
      
      // Send the complete message
      serializeJson(outputDoc, Serial);
      Serial.println();
    }
  }
  
  // Helper to send a system acknowledgement
  static void sendSystemAck(const char* cmd, bool ok, const char* status = nullptr) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["cmd"] = cmd;
    (*data)["ok"] = ok;
    
    if (status) {
      (*data)["status"] = status;
    }
    
    SerialMessage message("System", "ack", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper to send an error message
  static void sendError(const char* source, const char* action, const char* errorMsg) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["action"] = action;
    (*data)["error"] = errorMsg;
    
    SerialMessage message(source, "error", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper for coin counter messages
  static void sendCoinCounterStatus(const char* name, int count, int target, bool dispensing) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["count"] = count;
    
    if (dispensing && target > 0) {
      (*data)["target"] = target;
      (*data)["status"] = "dispensing";
    } else {
      (*data)["status"] = "idle";
    }
    
    SerialMessage message("CoinCounter", "status", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper for coin counter event messages
  static void sendCoinCounterEvent(const char* name, const char* event, int count, int target) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["event"] = event;
    (*data)["count"] = count;
    
    if (target > 0) {
      (*data)["target"] = target;
    }
    
    SerialMessage message("CoinCounter", "event", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper for coin counter timeout messages
  static void sendCoinCounterTimeout(const char* name, int finalCount, int target) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["event"] = "timeout";
    (*data)["final"] = finalCount;
    (*data)["target"] = target;
    
    SerialMessage message("CoinCounter", "error", data);
    sendMessage(message);
    
    delete data;
  }

  // Helper for coin counter acknowledgment messages
  static void sendCoinCounterAck(const char* name, const char* action, bool ok, int value = 0, const char* status = nullptr) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["action"] = action;
    (*data)["ok"] = ok;
    
    if (value > 0) {
      (*data)["value"] = value;
    }
    
    if (status != nullptr) {
      (*data)["status"] = status;
    }
    
    SerialMessage message("CoinCounter", "ack", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper for paper dispenser status messages
  static void sendPaperDispenserStatus(const char* name, const char* status, int current = 0, int total = 0) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["status"] = status;
    
    if (total > 0) {
      (*data)["current"] = current;
      (*data)["total"] = total;
    }
    
    SerialMessage message("PaperDispenser", "status", data);
    sendMessage(message);
    
    delete data;
  }

  // Helper for coin slot event messages
  static void sendCoinSlotEvent(int value, int total) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["value"] = value;
    (*data)["total"] = total;
    
    SerialMessage message("CoinSlot", "event", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper for relay status messages
  static void sendRelayStatus(const char* name, bool relay1, bool relay2, bool relay3) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["relay1"] = !relay1; // Invert because true = OFF, false = ON
    (*data)["relay2"] = !relay2;
    (*data)["relay3"] = !relay3;
    
    SerialMessage message("Relay", "status", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper for paper dispenser event messages
  static void sendPaperDispenserEvent(const char* name, const char* event, int total = 0) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["event"] = event;
    
    if (total > 0) {
      (*data)["total"] = total;
    }
    
    SerialMessage message("PaperDispenser", "event", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper for paper dispenser error messages
  static void sendPaperDispenserError(const char* name, const char* error, const char* details, int current = 0, int total = 0) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["error"] = error;
    (*data)["details"] = details;
    
    if (total > 0) {
      (*data)["current"] = current;
      (*data)["total"] = total;
    }
    
    SerialMessage message("PaperDispenser", "error", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Helper for paper dispenser acknowledgment messages
  static void sendPaperDispenserAck(const char* name, const char* action, bool ok, int value = 0, const char* status = nullptr) {
    StaticJsonDocument<128>* data = new StaticJsonDocument<128>();
    
    (*data)["name"] = name;
    (*data)["action"] = action;
    (*data)["ok"] = ok;
    
    if (value > 0) {
      (*data)["value"] = value;
    }
    
    if (status != nullptr) {
      (*data)["status"] = status;
    }
    
    SerialMessage message("PaperDispenser", "ack", data);
    sendMessage(message);
    
    delete data;
  }
  
  // Other helper methods can be added as needed for specific message types
};

#endif // SERIAL_COMM_H