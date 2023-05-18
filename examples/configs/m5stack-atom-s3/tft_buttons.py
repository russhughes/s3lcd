# esp32_s3_box

from machine import Pin


class Buttons:
    def __init__(self):
        self.name = "atom-s3"
        self.left = Pin(1, Pin.IN, Pin.PULL_UP)  # PORT A
        self.right = Pin(2, Pin.IN, Pin.PULL_UP) # PORT A
        self.hyper = 0
        self.thrust = 0
        self.fire = 0
