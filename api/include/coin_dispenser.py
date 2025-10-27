import RPi.GPIO as GPIO
#import lgpio as GPIO
import time

class CoinDispenser:
    #coin_dispensed = False
    def __init__(self, pin, sensor_pin=23, duration=2):
        self.pin = pin
        self.duration = duration
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(self.pin, GPIO.OUT)
        GPIO.output(self.pin, GPIO.HIGH)
        #self.sensor_pin = sensor_pin
        #GPIO.setup(self.sensor_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
        #GPIO.add_event_detect(self.sensor_pin, GPIO.FALLING, callback=self.check_coin_dispensed, bouncetime=100)        

    #def check_coin_dispensed(self):
    #    coin_dispensed = True

    def dispense_coin_start(self):
        GPIO.output(self.pin, GPIO.LOW)
        #while not coin_dispensed:
        #    pass
        #for i in range(count):
        #    time.sleep(self.duration)

    def dispense_coin_end(self):
        GPIO.output(self.pin, GPIO.HIGH)
        print(f"Coin dispensed from GPIO{self.pin}!")

    def cleanup(self):
        GPIO.cleanup(self.pin)


coins = {
    1: CoinDispenser(pin=8, duration=2),
    5: CoinDispenser(pin=7, duration=2),
    10: CoinDispenser(pin=1, duration=2),
}


def dispense_amount(amount):
    """Determines the number of each coin needed to match the given amount."""
    global coins
    for value in sorted(coins.keys(), reverse=True):
        count = amount // value
        if count > 0:
            coins[value].dispense_coin(count)
            amount -= count * value


if __name__ == "__main__":
    dispense_amount(17)
