"""
nasa_images.py - Display a series of NASA images on the display from the
nasa_480x320/ folder.

Images courtesy of the NASA image and video gallery available at
https://images.nasa.gov/
"""

import random
import time
import s3lcd
import tft_config
from machine import freq

tft = tft_config.config(tft_config.WIDE)


def main():

    """
    Decode and draw jpg on display
    """
    try:
        tft.init()
        width = tft.width()
        height = tft.height()

        while True:
            for image in range(1, 25):
                filename = f"nasa_{width}x{height}/nasa{image:02d}.jpg"
                tft.jpg(filename, 0, 0)  # Draw full screen jpg
                tft.show()  # Show the framebuffer and wait for it to finish
                time.sleep(5)  # Wait a second

    finally:
        tft.deinit()  # Deinitialize the display or it will cause a crash on the next run


main()
