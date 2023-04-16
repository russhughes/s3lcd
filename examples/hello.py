"""
hello.py

    Writes "Hello!" in random colors at random locations on the display.
"""

import random
import time

import vga1_bold_16x32 as big
import vga1_8x8 as small
import tft_config
import s3lcd

tft = tft_config.config(tft_config.WIDE)


def complementary_color(color565):
    """returns the complementary color of the given 565 color"""
    inverted_color = ~color565
    return (
        ((inverted_color & 0x001F) << 11)
        | (inverted_color & 0x07E0)
        | ((inverted_color & 0xF800) >> 11)
    )


def color_wheel(position):
    """returns a 565 color from the given position of the color wheel"""
    if position < 85:
        return s3lcd.color565(255 - position * 3, 0, position * 3)

    if position < 170:
        position -= 85
        return s3lcd.color565(0, position * 3, 255 - position * 3)

    position -= 170
    return s3lcd.color565(position * 3, 255 - position * 3, 0)


def center(using_font, text, fg=s3lcd.WHITE, bg=s3lcd.BLACK):
    """
    Centers the given text on the display.
    """
    length = len(text)
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
        for color in [s3lcd.RED, s3lcd.GREEN, s3lcd.BLUE]:
            tft.fill(color)
            tft.rect(0, 0, tft.width(), tft.height(), complementary_color(color))
            center(font, "Hello!", s3lcd.WHITE, color)
            tft.show()
            time.sleep(1)

        wheel = 0

        while True:
            tft.fill(s3lcd.BLACK)
            for rot in range(4):
                tft.rotation(rot)
                tft.fill(s3lcd.BLACK)
                col_max = tft.width() - font.WIDTH * 6
                row_max = tft.height() - font.HEIGHT

                now = time.ticks_ms()
                for i in range(50):
                    wheel = (wheel + 43) % 255
                    opacity = i * 255 // 10

                    tft.text(
                        font,
                        "Hello!",
                        random.randint(0, col_max),
                        random.randint(0, row_max),
                        color_wheel(wheel),
                        s3lcd.TRANSPARENT,
                        opacity,
                    )

                    tft.show()

                # print(time.ticks_ms() - now, "ms")

    finally:
        tft_config.deinit(tft)


main()
