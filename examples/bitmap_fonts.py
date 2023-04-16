"""
fonts.py

    Cycles through all characters of four bitmap fonts on the display

"""

import utime
import s3lcd
import tft_config
import vga1_8x8 as font1
import vga1_8x16 as font2
import vga1_bold_16x16 as font3
import vga1_bold_16x32 as font4


tft = tft_config.config(tft_config.WIDE)


def main():

    try:
        tft.init()

        while True:
            for font in (font1, font2, font3, font4):
                tft.fill(s3lcd.BLUE)
                line = 0
                col = 0
                for char in range(font.FIRST, font.LAST):
                    tft.text(font, chr(char), col, line, s3lcd.WHITE, s3lcd.BLUE)
                    tft.show()
                    col += font.WIDTH
                    if col > tft.width() - font.WIDTH:
                        col = 0
                        line += font.HEIGHT

                        if line > tft.height() - font.HEIGHT:
                            utime.sleep(3)
                            tft.fill(s3lcd.BLUE)
                            line = 0
                            col = 0

                utime.sleep(3)

    finally:
        tft.deinit()


main()
