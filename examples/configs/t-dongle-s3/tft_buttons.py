# input pins for esp32 T-Dongle-S3 module

from machine import Pin


class Buttons:
    def __init__(self):
        self.name = "t-dongle-s3"
        self.button = Pin(0, Pin.IN)
        self.left = self.button
        self.right = self.button

        # need more buttons for roids.py
        self.fire = 0
        self.thrust = 0
        self.hyper = 0
