# input pins for buttons: you will need to change these to match your wiring

from machine import Pin


class Buttons:
    def __init__(self):
        self.name = "t-qt-pro"
        self.left = Pin(0, Pin.IN)
        self.right = Pin(47, Pin.IN)

        # need more buttons for roids.py
        self.fire = 0
        self.thrust = 0
        self.hyper = 0
