"""
png_write.py

    Writes "Hello!" in the center of the display and saves the framebuffer to a png file.

"""

import random
import time

import vga2_bold_16x32 as big
import vga1_8x8 as small
import tft_config
import s3lcd

tft = tft_config.config(tft_config.WIDE)


def center(using_font, text, fg=s3lcd.WHITE, bg=s3lcd.BLACK):
    """
    Centers the given text on the display.
    """
    length = 1 if isinstance(text, int) else len(text)
    tft.text(
        using_font,
        text,
        tft.width() // 2 - length // 2 * using_font.WIDTH,
        tft.height() // 2 - using_font.HEIGHT // 2,
        fg,
        bg,
    )


def main():
    """
    The big show!
    """
    try:
        tft.init()
        font = small if tft.height() < 96 else big
        tft.fill(s3lcd.RED)
        tft.rect(0, 0, tft.width(), tft.height(), s3lcd.WHITE)
        center(font, b"\xAEHello\xAF", s3lcd.WHITE, s3lcd.RED)
        tft.show()
        tft.png_write("hello.png")

    finally:
        tft_config.deinit(tft)


main()
