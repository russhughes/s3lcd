"""
alien.py

    Randomly draw alien.jpg on the display using various methods.

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
    Decode and draw jpg on display using various methods
    """

    try:
        tft = tft_config.config(tft_config.WIDE)

        # enable display and clear screen
        tft.init()

        # read png file into buffer without decoding
        with open("logo-64x64.jpg", "rb") as file:
            alien = file.read()

        # read and decode png file into a tuple containing (image, width, height)
        decoded_bitmap = tft.jpg_decode("logo-64x64.jpg")
        bitmap_width = decoded_bitmap[1]
        bitmap_height = decoded_bitmap[2]

        draw_count = 100

        # display png in random locations
        while True:
            tft.clear(0)
            # decode and draw jpg from a file
            start = time.ticks_ms()
            for _ in range(draw_count):
                tft.jpg(
                    "logo-64x64.jpg",
                    random.randint(0, tft.width() - bitmap_width),
                    random.randint(0, tft.height() - bitmap_height),
                    True,
                )

            print(
                f"Drawing {draw_count} jpg's from a file took {time.ticks_ms() - start} ms."
            )
            tft.show()
            tft.clear(0)
            # decode and draw jpg from a buffer
            start = time.ticks_ms()
            for _ in range(draw_count):
                tft.jpg(
                    alien,
                    random.randint(0, tft.width() - bitmap_width),
                    random.randint(0, tft.height() - bitmap_height),
                    True,
                )

            print(
                f"Drawing {draw_count} jpg's from a buffer took {time.ticks_ms() - start} ms."
            )
            tft.show()
            tft.clear(0)
            # draw decoded bitmap from a tuple created by jpg_decode()
            start = time.ticks_ms()
            for _ in range(draw_count):
                tft.blit_buffer(
                    decoded_bitmap[0],
                    random.randint(0, tft.width() - bitmap_width),
                    random.randint(0, tft.height() - bitmap_height),
                    decoded_bitmap[1],
                    decoded_bitmap[2],
                )

            print(
                f"Drawing {draw_count} jpg's from a decoded buffer took {time.ticks_ms() - start} ms.\n"
            )
            tft.show()

    finally:
        tft.deinit()


main()
