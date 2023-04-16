"""
logo.py

    Draw the MicroPython Logo.

"""

import random
import tft_config

tft = tft_config.config(tft_config.WIDE)


def main():
    """
    Decode and draw jpg on display
    """

    try:
        tft.init()
        width=tft.width()
        height=tft.height()

        image = f'logo-{width}x{height}.jpg'
        print(f"Loading {image}")
        tft.jpg(image, 0, 0)
        tft.show()

    finally:
        tft.deinit()


main()
