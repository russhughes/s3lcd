# input pins for buttons: you will need to change these to match your wiring

from machine import Pin


class Buttons:
    def __init__(self):
        self.name = "t-embed"
        self.left = Pin(17, Pin.IN, Pin.PULL_UP)  # middle GROVE connector
        self.right = Pin(18, Pin.IN, Pin.PULL_UP)

        # need more buttons for roids.py
        self.fire = 0
        self.thrust = 0
        self.hyper = 0
