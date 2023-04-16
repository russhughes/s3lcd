"""
blit_bounce.py

    Bounce a blitable buffer around the display to test visibility clipping.

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
    Decode and draw jpg on display using various methods
    """

    try:
        tft = tft_config.config(tft_config.WIDE)

        # enable display and clear screen
        tft.init()
        width = tft.width()
        height = tft.height()
        col = width // 2 - LOGO_WIDTH // 2
        row = height // 2 - LOGO_HEIGHT // 2
        xd = 1
        yd = 1

        bitmap, bitmap_width, bitmap_height = tft.jpg_decode("logo-64x64.jpg")
        ticks = 1000 // 45

        while True:
            last = time.ticks_ms()
            tft.clear(0)
            tft.blit_buffer(bitmap, col, row, bitmap_width, bitmap_height)
            tft.show()

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
