"""
hello.py

    Writes "Hello!" in random colors at random locations on the display.
"""

import random
import time

import vga1_8x8 as font
import tft_config
import s3lcd

tft = tft_config.config(tft_config.WIDE)


def center(text, fg=s3lcd.WHITE, bg=s3lcd.BLACK):
    """
    Centers the given text on the display.
    """
    length = len(text)
    tft.text(
        font,
        text,
        tft.width() // 2 - length // 2 * font.WIDTH,
        tft.height() // 2 - font.HEIGHT,
        fg,
        bg,
    )


def main():
    """
    The big show!
    """
    try:
        tft.init()
        for color in [s3lcd.RED, s3lcd.GREEN, s3lcd.BLUE]:
            tft.fill(color)
            tft.rect(0, 0, tft.width(), tft.height(), s3lcd.WHITE)
            center("Hello!", s3lcd.WHITE, color)
            tft.show()
            time.sleep(1)

        while True:
            for rotation in range(4):
                now = time.ticks_ms()
                tft.rotation(rotation)
                tft.fill(0)
                col_max = tft.width() - font.WIDTH * 6
                row_max = tft.height() - font.HEIGHT

                for _ in range(128):
                    tft.text(
                        font,
                        "Hello!",
                        random.randint(0, col_max),
                        random.randint(0, row_max),
                        s3lcd.color565(
                            random.getrandbits(8),
                            random.getrandbits(8),
                            random.getrandbits(8),
                        ),
                        s3lcd.color565(
                            random.getrandbits(8),
                            random.getrandbits(8),
                            random.getrandbits(8),
                        ),
                    )

                    tft.show()

                # print(time.ticks_ms() - now, "ms")

    finally:
        tft_config.deinit(tft)


main()
