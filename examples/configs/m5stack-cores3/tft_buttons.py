# M5STACK CORES3
#  - Not working with DUAL BUTTON unit

# DIN Base
#   Port A  1,  2
#   Port B  8,  9
#   Port C 18, 17

from machine import Pin

class Buttons:
    def __init__(self):
        self.name = "m5cores3"
        self.left = Pin(1, Pin.IN, Pin.PULL_UP)  # PORT A
        self.right = Pin(2, Pin.IN, Pin.PULL_UP) # PORT A
        self.hyper = 0
        self.thrust = 0
        self.fire = 0
