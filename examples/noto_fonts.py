"""
noto_fonts Writes the names of three Noto fonts centered on the display
    using the font. The fonts were converted from True Type fonts using
    the font2bitmap utility.
"""
import time
import s3lcd
import tft_config
import NotoSans_32 as noto_sans
import NotoSerif_32 as noto_serif
import NotoSansMono_32 as noto_mono


tft = tft_config.config(tft_config.WIDE)


def center(font, s, row, color=s3lcd.WHITE):
    screen = tft.width()  # get screen width
    width = tft.write_len(font, s)  # get the width of the string
    col = tft.width() // 2 - width // 2 if width and width < screen else 0
    tft.write(font, s, col, row, color)  # and write the string


def main():
    try:
        # init display
        tft.init()
        tft.fill(s3lcd.BLACK)

        # center the name of the first font, using the font
        row = 16
        center(noto_sans, "NotoSans", row, s3lcd.RED)
        row += noto_sans.HEIGHT

        # center the name of the second font, using the font
        center(noto_serif, "NotoSerif", row, s3lcd.GREEN)
        row += noto_serif.HEIGHT

        # center the name of the third font, using the font
        center(noto_mono, "NotoSansMono", row, s3lcd.BLUE)
        row += noto_mono.HEIGHT

        tft.show()
        time.sleep(5)
    finally:
        tft.deinit()


main()
