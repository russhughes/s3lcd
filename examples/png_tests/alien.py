"""
alien.py

    Randomly draw alien.png to test alpha-channel masking

    The alien.png is from the Erik Flowers Weather Icons available from
    https://github.com/erikflowers/weather-icons and is licensed under
    SIL OFL 1.1

"""

import gc
import random
import time
import tft_config
import s3lcd

gc.enable()
gc.collect()


def main():
    """
    Decode and draw png on display
    """

    try:
        tft = tft_config.config(tft_config.WIDE)
        tft.init()
        width = tft.width()
        height = tft.height()

        # display png in random locations
        while True:
            x = random.randint(0, width - 32)
            y = random.randint(0, height - 32)
            tft.png("alien.png", x, y)
            tft.show()

    finally:
        tft.deinit()


main()
