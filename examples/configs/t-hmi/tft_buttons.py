# input pins for buttons: you will need to change these to match your wiring

from machine import Pin, ADC


class Buttons:
    def __init__(self):
        self.name = "t-hmi"

        # Middle GROVE connector
        self.left = Pin(17, Pin.IN)
        self.right = Pin(18, Pin.IN)

        # Right GROVE connector
        self.fire = Pin(15, Pin.IN)
        self.thrust = Pin(16, Pin.IN)
        self.hyper = 0
