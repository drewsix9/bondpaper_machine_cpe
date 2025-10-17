# main.py — FastAPI brain that talks to Arduino via serial (no GPIO on Pi)

import json
import time
from typing import Any, Dict, Optional
from serial_manager import SerialManager

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware

# If you still want to reuse your existing Item model, keep this import:
from include.item_model import Item

import serial

# ---------- Serial Setup ----------
# Adjust the port if needed; on some Pis/boards it could be /dev/ttyUSB0
SERIAL_PORT = "/dev/ttyACM0"
BAUD = 115200

# 'timeout' makes readline() return after that many seconds even if no '\n'
serial_mgr = SerialManager(SERIAL_PORT, BAUD)

# ---------- FastAPI ----------
app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # tighten in prod
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# ---------- Helpers ----------

def _send(cmd: str) -> None:
    """Send a single-line command to Arduino (newline-terminated)."""
    while ser.in_waiting > 0:
        ser.read(ser.in_waiting)
    if not cmd.endswith("\n"):
        cmd += "\n"
    ser.write(cmd.encode("utf-8"))
    ser.flush()
    time.sleep(0.1)

def _read_response(timeout: float = 3.0) -> str:
    """Read multiple lines until timeout or empty line"""
    result = []
    end = time.time() + timeout
    while time.time() < end:
        line = ser.readline().decode("utf-8", "ignore").strip()
        if line:
            result.append(line)
            # Option 1: Stop after empty line
            if len(result) > 0 and not line:
                break
            # Option 2: Look for a completion marker
            if line == "END" or line.endswith("}"):
                break
    return "\n".join(result)

def _send_and_wait(cmd: str, expect_json: bool = True, timeout: float = 5.0) -> Dict[str, Any]:
    """
    Send a command and wait for one response line.
    If expect_json is True, parse JSON; raise 502 if invalid or error reported.
    If not JSON (e.g., 'get' may return a plain integer from your older coinslot),
    we return {"raw": "<line>"} and the caller can interpret it.
    """
    _send(cmd)
    line = _read_response(timeout=timeout)
    if line is None:
        raise HTTPException(status_code=504, detail=f"Timeout waiting for reply to '{cmd.strip()}'")

    # Try JSON first
    if expect_json:
        try:
            obj = json.loads(line)
            # If Arduino replies with {ok:false,...} treat as a 502 from backend
            if isinstance(obj, dict) and obj.get("ok") is False:
                raise HTTPException(status_code=502, detail=obj.get("error", "Arduino reported error"))
            return obj if isinstance(obj, dict) else {"data": obj}
        except json.JSONDecodeError:
            # Fallback for legacy non-JSON responses (e.g., get -> "7")
            return {"raw": line}

    # Not expecting JSON: return raw line
    return {"raw": line}

def _parse_coins_from_reply(reply: Dict[str, Any]) -> int:
    """
    Accepts either the JSON your new firmware prints (e.g., {"ok":true,"cmd":"get_coins","coins":7})
    OR legacy plain integer string from your old CoinSlot code (e.g., "7").
    """
    # JSON path
    if "coins" in reply:
        try:
            return int(reply["coins"])
        except Exception:
            pass
    # Raw integer fallback
    if "raw" in reply:
        s = str(reply["raw"]).strip()
        if s.isdigit():
            return int(s)
        # Sometimes raw could be JSON-as-text; try decoding:
        try:
            obj = json.loads(s)
            if isinstance(obj, dict) and "coins" in obj:
                return int(obj["coins"])
        except Exception:
            pass
    # Couldn’t parse
    raise HTTPException(status_code=502, detail=f"Unexpected coin reply: {reply}")

# ---------- Routes ----------

@app.get("/health")
def health():
    return {"status": "ok"}

@app.post("/coinslot/start")
def coinslot_start():
    response = serial_mgr.send_command("coinslot_start", expect_json=True, timeout=1.0)
    return response

@app.post("/coinslot/stop")
def coinslot_stop():
    # Replace the old _send_and_wait call with serial_mgr
    response = serial_mgr.send_command("coinslot_stop", expect_json=True, timeout=1.0)
    return response

@app.get("/coins")
def get_coin_count():
    """
    Ask Arduino for current inserted coins.
    Uses robust serial manager with retry logic.
    """
    response = serial_mgr.send_command("get", expect_json=False)
    
    if "error" in response:
        # Return a fallback value during errors (optional)
        # return {"coins": 0, "error": response["error"]}
        raise HTTPException(status_code=503, detail=response["error"])
    
    # Parse coins from response
    if "raw" in response:
        s = str(response["raw"]).strip()
        if s.isdigit():
            return {"coins": int(s)}
        # Try parsing as JSON in raw text
        try:
            obj = json.loads(s)
            if isinstance(obj, dict) and "coins" in obj:
                return {"coins": int(obj["coins"])}
        except Exception:
            pass
    
    # Couldn't parse
    raise HTTPException(status_code=502, detail=f"Unexpected coin reply: {response}")

@app.post("/reset-coins")
def reset_coins():
    # New firmware replies with JSON; older code might reply nothing.
    try:
        r = serial_mgr.send_command("reset", expect_json=False)
    except HTTPException:
        # Fallback: even if no JSON, consider it reset if Arduino is alive
        r = {"ok": True}
    return {"message": "Coin count reset", "reply": r}

@app.post("/dispense/paper/{paper}/{qty}")
def dispense_paper(paper: str, qty: int):
    if paper.upper() not in ("A4", "LONG"):
        raise HTTPException(status_code=400, detail="paper must be A4 or LONG")
    if qty <= 0:
        raise HTTPException(status_code=400, detail="qty must be > 0")
    cmd = f"paper {paper.upper()} {qty}"
    r = _send_and_wait(cmd, expect_json=True, timeout=max(5.0, 10.0 * qty))  # allow longer for multiple sheets
    return r

@app.post("/dispense/coin/{value}/{count}")
def dispense_coin(value: int, count: int):
    if value not in (1, 5, 10):
        raise HTTPException(status_code=400, detail="value must be 1, 5, or 10")
    if count <= 0:
        raise HTTPException(status_code=400, detail="count must be > 0")
    cmd = f"dispense_coin {value} {count}"
    # Arduino stops on target reached OR timeout if no coin detected
    r = _send_and_wait(cmd, expect_json=True, timeout=max(6.0, 6.0 * count))
    return r

@app.post("/dispense/amount/{amount}")
def dispense_amount(amount: int):
    if amount < 0:
        raise HTTPException(status_code=400, detail="amount must be >= 0")
    cmd = f"dispense_amount {amount}"
    r = _send_and_wait(cmd, expect_json=True, timeout=max(6.0, 2.0 * amount))
    return r

@app.post("/buy")
def buy(item: Item):
    """
    Transaction flow:
      1) Read inserted coins.
      2) If sufficient (coins >= price), tell Arduino to dispense paper.
      3) Dispense change via 'dispense_amount'.
      4) Reset coin total.
    NOTE: Your original code was using 'quantity' as the price.
    """
    # 1) coins inserted
    coins_reply = _send_and_wait("get", expect_json=True, timeout=2.5)
    coins = _parse_coins_from_reply(coins_reply)

    price = int(item.quantity)
    change = coins - price
    if change < 0:
        return {"ok": False, "reason": "insufficient_funds", "coins": coins, "price": price}

    # 2) paper
    paper_cmd = f"paper {item.paper.upper()} {item.quantity}"
    paper_reply = _send_and_wait(paper_cmd, expect_json=True, timeout=max(10.0, 10.0 * item.quantity))
    if not paper_reply.get("ok", True):  # in case Arduino sends {ok:false}
        raise HTTPException(status_code=502, detail=f"paper failed: {paper_reply}")

    # 3) change
    if change > 0:
        change_reply = _send_and_wait(f"dispense_amount {change}", expect_json=True, timeout=max(8.0, 2.0 * change))
        if not change_reply.get("ok", True):
            raise HTTPException(status_code=502, detail=f"change failed: {change_reply}")
    else:
        change_reply = {"ok": True, "dispensed": 0}

    # 4) reset
    try:
        reset_reply = _send_and_wait("reset", expect_json=True, timeout=3.0)
    except HTTPException:
        reset_reply = {"ok": True}

    return {
        "ok": True,
        "coins_before": coins,
        "price": price,
        "change": change,
        "paper_reply": paper_reply,
        "change_reply": change_reply,
        "reset_reply": reset_reply,
        "request": item.dict(),
    }

# ---------- Run (optional) ----------
# If you run: python3 main.py
if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
