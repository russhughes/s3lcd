"""
png_bounce.py

    Bounce a png around the display to test visibility clipping.

"""

import gc
import random
import time
import tft_config
import s3lcd

gc.enable()
gc.collect()

LOGO_WIDTH = 30
LOGO_HEIGHT = 30


def main():
    """
    Bounce a png around the display.
    """

    try:
        tft = tft_config.config(tft_config.WIDE)

        # enable display and clear screen
        tft.init()
        width = tft.width()
        height = tft.height()
        col = width // 2 - LOGO_WIDTH // 2
        row = height // 2 - LOGO_HEIGHT // 2
        xd = 3
        yd = 2

        ticks = 1000 // 45

        # read jpg file into buffer without decoding
        with open("alien.png", "rb") as file:
            alien = file.read()

        while True:
            last = time.ticks_ms()
            tft.png(alien, col, row)
            tft.show()
            tft.clear(0)

            # Update the position to bounce the bitmap around the screen
            col += xd
            if col <= -LOGO_WIDTH - 1 or col > width:
                xd = -xd

            row += yd
            if row <= -LOGO_HEIGHT - 1 or row > height:
                yd = -yd

            if time.ticks_ms() - last < ticks:
                time.sleep_ms(ticks - (time.ticks_ms() - last))

    finally:
        tft.deinit()


main()
