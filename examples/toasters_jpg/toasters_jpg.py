"""
toasters_jpg.py

    An example using a jpg sprite map to draw sprites on T-Display.  This is an older version of the
    toasters.py and tiny_toasters example.  It uses the jpg_decode() method to grab a bitmap of each
    sprite from the toaster.jpg sprite sheet.

    youtube video: https://youtu.be/0uWsjKQmCpU

    spritesheet from CircuitPython_Flying_Toasters
    https://learn.adafruit.com/circuitpython-sprite-animation-pendant-mario-clouds-flying-toasters
"""

import time
import random
import tft_config
import s3lcd

tft = tft_config.config(tft_config.WIDE)


class toast:
    """
    toast class to keep track of a sprites locaton and step
    """

    def __init__(self, sprites, x, y):
        self.sprites = sprites
        self.steps = len(sprites)
        self.x = x
        self.y = y
        self.step = random.randint(0, self.steps - 1)
        self.speed = random.randint(2, 5)

    def move(self):
        if self.x <= 0:
            self.speed = random.randint(2, 5)
            self.x = tft.width() - 64

        self.step += 1
        self.step %= self.steps
        self.x -= self.speed


def main():
    """
    Draw and move sprite
    """

    try:
        # enable display and clear screen
        tft.init()
        tft.fill(s3lcd.BLACK)
        tft.show()

        width = 64
        height = 64

        # grab each sprite from the toaster.jpg sprite sheet
        t1 = tft.jpg_decode("toaster.jpg", 0, 0, width, height)
        t2 = tft.jpg_decode("toaster.jpg", width, 0, width, height)
        t3 = tft.jpg_decode("toaster.jpg", width * 2, 0, width, height)
        t4 = tft.jpg_decode("toaster.jpg", 0, height, width, height)
        t5 = tft.jpg_decode("toaster.jpg", width, height, width, height)

        TOASTERS = [t1[0], t2[0], t3[0], t4[0]]
        TOAST = [t5[0]]

        sprites = [
            toast(TOASTERS, tft.width() - width, 0),
            toast(TOAST, tft.width() - width, height),
            toast(TOASTERS, tft.width() - width, height * 2),
        ]

        # move and draw sprites
        while True:
            for man in sprites:
                bitmap = man.sprites[man.step]

                tft.fill_rect(
                    man.x + width - man.speed,
                    man.y,
                    man.speed,
                    height,
                    s3lcd.BLACK,
                )

                man.move()

                if man.x > 0:
                    tft.blit_buffer(bitmap, man.x, man.y, width, height)
                else:
                    tft.fill_rect(0, man.y, width, height, s3lcd.BLACK)

            tft.show()
            time.sleep(0.03)

    finally:
        tft.deinit()


main()
