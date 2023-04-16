"""
mono_fonts.py test for monofont2bitmap converter and bitmap method. This is the older method of
converting monofonts to bitmaps.  See the newer method in prop_fonts/chango.py that works with
mono and proportional fonts using the write method.
"""

import time
import s3lcd
import tft_config

import inconsolata_16 as font_16
import inconsolata_32 as font_32
import inconsolata_64 as font_64

tft = tft_config.config(tft_config.WIDE)


def display_font(font, fast):
    tft.fill(s3lcd.BLUE)
    column = 0
    row = 0
    for char in font.MAP:
        tft.bitmap(font, column, row, font.MAP.index(char))
        tft.show()
        column += font.WIDTH
        if column >= tft.width() - font.WIDTH:
            row += font.HEIGHT
            column = 0

            if row > tft.height() - font.HEIGHT:
                row = 0

        if not fast:
            time.sleep(0.05)


def main():
    fast = False

    try:
        tft.init()

        while True:
            for font in [font_16, font_32, font_64]:
                display_font(font, fast)

            fast = not fast

    finally:
        tft.deinit()


main()
