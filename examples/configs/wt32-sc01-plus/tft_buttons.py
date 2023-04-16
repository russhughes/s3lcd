# input pins for buttons: you will need to change these to match your wiring

from machine import Pin

class Buttons:
    def __init__(self):
        self.name = "wt32-sc01-plus"
        self.left = Pin(0, Pin.IN)
        self.right = Pin(14, Pin.IN)

        # need more buttons for roids.py
        self.fire = 0
        self.thrust = 0
        self.hyper = 0
