import RPi.GPIO as GPIO
#import lgpio as GPIO
import time
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from include.config import a4_step_motors, long_step_motors
from include.dispenser_final import PaperDispenser
from include.item_model import Item
from include.coin_dispenser import CoinDispenser

import uvicorn
# for connecting to Arduino Coinslot
import serial

ser = serial.Serial('/dev/ttyACM0', 115200)

GPIO.setmode(GPIO.BCM)
app = FastAPI()
origins = ["*"]

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Set Pin Numbers Here:
COIN_PIN = 17  # Change to your actual GPIO pin
coin_hopper_state_pin = 26 #TODO
coin_dispensed = False
GPIO.setup(coin_hopper_state_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

def coin_dispense_detected(channel):
    global coin_dispensed
    coin_dispensed = True
    print("CALLED")

GPIO.add_event_detect(coin_hopper_state_pin, GPIO.FALLING, callback=coin_dispense_detected, bouncetime=300)


# REMOVE
GPIO.setwarnings(False)

coin_count = 0  # Global variable to store the count
pulse_count = 0

dispensers = {
"A4": PaperDispenser(a4_step_motors['stepper'], a4_step_motors['dc_motor']),
"LONG": PaperDispenser(long_step_motors['stepper'], long_step_motors['dc_motor'])
}

# Define coin dispensers for different values
coins = {
    1: CoinDispenser(pin=8, duration=2),
    5: CoinDispenser(pin=7, duration=2),
    10: CoinDispenser(pin=1, duration=2),
}
def dispense_amount(amount):
    """Determines the number of each coin needed to match the given amount."""
    global coins
    global coin_dispensed
    for value in sorted(coins.keys(), reverse=True):
        count = amount // value
        if count > 0:
            for i in range(count):
                coins[value].dispense_coin_start()
                time.sleep(1)
                #while not coin_dispensed:
                #    pass
                #coin_dispensed = False
                coins[value].dispense_coin_end()
            amount -= count * value


def check_coin_slot_interrupt():
    global pulse_count
    if pulse_count > 0:
        # Determine coin value based on pulse count
        if pulse_count == 5:
            current_coin_value = 1  # 1 Peso
        elif pulse_count == 6:
            current_coin_value = 5  # 5 Pesos
        elif pulse_count == 7:
            current_coin_value = 10  # 10 Pesos
        elif pulse_count == 8:
            current_coin_value = 20  # 20 Pesos
        else:
            current_coin_value = 0  # Unknown coin
        # Add logic here to handle the coin insertion 
        # (e.g., update a counter, dispense a product, etc.)
        pulse_count = 0  # Reset pulse count for the next coin
        return current_coin_value
    return 0
    
def coin_inserted():
    global coin_count

    # read serial line
    ser.write('get\n'.encode())
    coinslot_return = ser.readline().decode('utf-8').strip()

    print('RETURNED VALUE', coinslot_return)
    coin_count = int(coinslot_return)
    #coin_count += check_coin_slot_interrupt()
    #print(f"Coin detected! Total: {coin_count}")
    #return coin_count


def count_pulse(channel):
    """Callback function for each pulse received."""
    global pulse_count
    pulse_count += 1
    print(pulse_count)

# Detect falling edge (coin pulse)
GPIO.setup(COIN_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.add_event_detect(COIN_PIN, GPIO.FALLING, callback=count_pulse, bouncetime=5)


# ROUTES
@app.get("/coins")
def get_coin_count():
    global coin_count
    coin_inserted()
    return {"coins": coin_count}

@app.post("/reset-coins")
def reset_coins():
    global coin_count
    ser.write('reset\n'.encode())
    coin_count = 0
    return {"message": "Coin count reset"}

@app.post("/buy")
async def get_coin_count(item: Item):
    global coin_count
    change = 0
    print("Coin count:", coin_count)
    try:
        print("Item quantity:", item.quantity)
        change = coin_count - item.quantity
        if (change >= 0):
            dispensers[item.paper].dispense(item.quantity)
            dispense_amount(change)
            coin_count = 0
            ser.write('reset\n'.encode())
        else:
            print("Invalid value for change")
    except ValueError:
        print("Invalid input. Please enter a valid amount.")

    return {"coins": coin_count, "request": item}


if __name__ == "__main__":
    #uvicorn.run(app, host="0.0.0.0", port=8000)
    #time.sleep(2)
    #while True:
    #    coin_inserted()
    #    time.sleep(1)

    #dispensers["A4"].dispense(1)
    #time.sleep(3)
    #dispensers["LONG"].dispense(1)
    #coin_dispensed = True
    dispense_amount(10)
    #while True:
    #    print("VALUE:", GPIO.input(coin_hopper_state_pin))

