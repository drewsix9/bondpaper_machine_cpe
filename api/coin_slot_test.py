import RPi.GPIO as GPIO
import time

COIN_PIN = 17  # Change to your actual GPIO pin
GPIO.setmode(GPIO.BCM)
GPIO.setup(COIN_PIN, GPIO.IN)
GPIO.setwarnings(False)

coin_count = 0  # Global variable to store the count
pulse_count = 0

def count_pulse(channel):
    """Callback function for each pulse received."""
    global pulse_count
    pulse_count += 1
    print(pulse_count)

def check_coin_slot_interrupt():
    global pulse_count
    if pulse_count > 0:
        # Determine coin value based on pulse count
        if pulse_count == 1:
            current_coin_value = 1  # 1 Peso
        elif pulse_count == 5:
            current_coin_value = 5  # 5 Pesos
        elif pulse_count == 10:
            current_coin_value = 10  # 10 Pesos
        else:
            current_coin_value = 0  # Unknown coin
        # Add logic here to handle the coin insertion 
        # (e.g., update a counter, dispense a product, etc.)
        pulse_count = 0  # Reset pulse count for the next coin
        return current_coin_value
    return 0
    
def coin_inserted():
    global coin_count
    coin_count += check_coin_slot_interrupt()
    print(f"Coin detected! Total: {coin_count}")

# Detect falling edge (coin pulse)
GPIO.add_event_detect(COIN_PIN, GPIO.FALLING, callback=count_pulse, bouncetime=10)

if __name__ == '__main__':
    while True:
        pass
    #    coin_inserted()
    #    time.sleep(1)

