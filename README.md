# 🧾 Bondpaper Machine

An automated **bond paper vending and dispensing system** that integrates:
- a **FastAPI backend** hosted on Raspberry Pi 4B  
- a **Progressive Web App (PWA)** frontend for user interaction  
- an **Arduino-based hardware controller** for motor, coin, and sensor operations  

This project automates paper dispensing in a self-service setup with accurate coin detection and precise stepper/DC motor control.

---

## 📁 Project Structure

```text
bondpaper_machine_me_cpe/
├── api/
│   ├── main.py             # FastAPI backend (main app on Raspberry Pi)
│   ├── requirements.txt    # Python dependencies
│   └── ...
│
├── pwa/
│   ├── src/                # Progressive Web App source code
│   ├── public/             # Static frontend assets
│   └── package.json        # Frontend dependencies and scripts
│
└── src/
    └── main-final/         # Arduino working source code
        ├── Config.h        # Pin definitions and configuration
        ├── main-final.ino  # Main firmware logic
        └── ...
```

---

## ⚙️ Backend – FastAPI (Raspberry Pi 4B)

**Location:** `/api/main.py`  
**Purpose:** Central controller that bridges:
- The **Arduino module** via serial interface  
- The **PWA frontend** via REST API endpoints  

### Run the API Server

```bash
cd api
pip install -r requirements.txt
uvicorn main:app --host 0.0.0.0 --port 8000
```

**Access:**  
- API root → http://localhost:8000  

---

## 💻 Frontend – PWA

**Location:** `/pwa/`  
**Description:** Web-based UI for selecting paper size, inserting coins, and monitoring machine status.

### Run the PWA

```bash
cd pwa
npm install
npm run dev
```

Open in browser → [http://localhost:3000](http://localhost:3000)

---

## 🔩 Firmware – Arduino

**Location:** `/src/main-final/`  
**Microcontroller:** Arduino Mega 2560  

**Handles:**
- Coin pulse reading and denomination mapping  
- Stepper & DC motor control (short/long paper)  
- Limit switch & sensor feedback  
- Serial communication with Raspberry Pi  

### Uploading

```text
1. Open main-final.ino in Arduino IDE
2. Select the correct board and port
3. Click Upload
```

---

## 🧠 System Overview

| Layer | Component | Function |
|-------|------------|-----------|
| **Hardware** | Arduino + Sensors & Motors | Mechanical control and sensor readings |
| **Middleware** | FastAPI on Raspberry Pi | Central logic and API bridge |
| **Frontend** | Progressive Web App (PWA) | User interface for interaction and status display |

---

## 🧰 Requirements

### Hardware
- Raspberry Pi 4B  
- Arduino Mega 2560 
- Coin Acceptor (Allan-type)  
- Stepper Motors + Drivers  
- Relay Modules
- Coin Hoppers
- DC Motors  
- Limit Switches / Sensors 
- Buck/Boost Converter 
- Regulated Power Supply (12V/30A)

### Software
- Python ≥ 3.10 (FastAPI, Uvicorn)  
- Node.js ≥ 18  
- Arduino IDE
