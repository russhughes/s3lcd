"""
bounce_jpg.py

    Bounce a jpg around the display to test visibility clipping.

"""

import gc
import random
import time
import tft_config
import s3lcd

gc.enable()
gc.collect()

LOGO_WIDTH = 64
LOGO_HEIGHT = 64


def main():
    """
    Bounce a jpg around the display.
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

        with open(f'logo-{LOGO_WIDTH}x{LOGO_HEIGHT}.jpg', "rb") as file:
            jpg_logo = file.read()

        while True:
            last = time.ticks_ms()

            tft.jpg(jpg_logo, col, row)
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
