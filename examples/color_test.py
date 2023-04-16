import random
import time

import vga1_bold_16x32 as font
import tft_config
import s3lcd


def center(text, fg=s3lcd.WHITE, bg=s3lcd.BLACK):
    """
    Centers the given text on the display.
    """
    length = len(text)
    tft.text(
        font,
        text,
        tft.width() // 2 - length // 2 * font.WIDTH,
        tft.height() // 2 - font.HEIGHT // 2,
        fg,
        bg,
    )


try:
    tft = tft_config.config(tft_config.WIDE)
    tft.init()

    for name, background in (
        ("Red", s3lcd.RED),
        ("Green", s3lcd.GREEN),
        ("Blue", s3lcd.BLUE),
        ("Cyan", s3lcd.CYAN),
        ("Magenta", s3lcd.MAGENTA),
        ("Yellow", s3lcd.YELLOW),
        ("White", s3lcd.WHITE),
        ("Black", s3lcd.BLACK),
    ):

        color = (
            s3lcd.WHITE
            if background not in (s3lcd.WHITE, s3lcd.YELLOW)
            else s3lcd.BLACK
        )

        tft.fill(background)
        tft.rect(0, 0, tft.width(), tft.height(), color)

        center(name, color, background)
        tft.show()
        time.sleep(2)

finally:
    tft.deinit()
