"""
chango.py proportional font test for font2bitmap converter.
"""

import time
import gc
import s3lcd
import tft_config

tft = tft_config.config(tft_config.WIDE)

#
# Large fonts take alot of memory, they should be frozen in the
# firmaware or cross-compiled using the mpy-cross compiler.
#

import chango_16 as font_16
import chango_32 as font_32
import chango_64 as font_64

gc.collect()


def display_font(font):

    tft.fill(s3lcd.BLUE)  # clear the screen
    column = 0  # first column
    row = 0  # first row

    for char in font.MAP:  # for each character in the font map
        width = tft.write_len(font, char)  # get the width of the character

        if (
            column + width > tft.width()
        ):  # if the character will not fit on the current line
            row += font.HEIGHT  # move to the next row
            column = 0  # reset the column

            if (
                row + font.HEIGHT > tft.height()
            ):  # if the row will not fit on the screen
                time.sleep(1)  # pause for a second
                tft.fill(s3lcd.BLUE)  # clear the screen
                row = 0  # reset the row

        tft.write(  # write to the screen
            font,  # in this font
            char,  # the character
            column,  # at this column
            row,  # on this row
            s3lcd.WHITE,  # in white
            s3lcd.BLUE,
        )  # with blue background

        tft.show()  # show the screen
        column += width  # move the column past the character


def main():
    try:
        tft.init()
        tft.fill(s3lcd.BLUE)

        for font in [font_16, font_32, font_64]:  # for each font
            display_font(font)  # display the font characters
            time.sleep(1)  # pause for a second

    finally:
        tft.deinit()


main()
