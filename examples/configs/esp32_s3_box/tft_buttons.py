# esp32_s3_box

from machine import Pin


class Buttons:
    def __init__(self):
        self.name = "esp32-s3-box"
        self.left = 0
        self.right = 0
        self.hyper = 0
        self.thrust = 0
        self.fire = 0
