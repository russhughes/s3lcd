"""
proverbs.py - Displays what I hope are chinese proverbs in simplified chinese to test UTF-8
    font support.
"""

import utime
import s3lcd
import tft_config
import notosanssc as font


tft = tft_config.config(tft_config.WIDE)


def cycle(p):
    """return the next item in a list"""
    try:
        len(p)
    except TypeError:
        cache = []
        for i in p:
            yield i
            cache.append(i)

        p = cache
    while p:
        yield from p


COLORS = (
    s3lcd.RED,
    s3lcd.GREEN,
    s3lcd.BLUE,
    s3lcd.CYAN,
    s3lcd.MAGENTA,
    s3lcd.YELLOW,
    s3lcd.WHITE,
)

COLOR = cycle(COLORS)


def main():

    proverbs = [
        "一口吃不成胖子",
        "万事起头难",
        "熟能生巧",
        "冰冻三尺，非一日之寒",
        "三个臭皮匠，胜过诸葛亮",
        "今日事，今日毕",
        "师父领进门，修行在个人",
        "欲速则不达",
        "百闻不如一见",
        "不入虎穴，焉得虎子",
    ]

    try:
        # initialize display
        tft.init()
        line_height = font.HEIGHT + 8
        half_height = tft.height() // 2
        half_width = tft.width() // 2

        tft.fill(s3lcd.BLACK)

        while True:
            for proverb in proverbs:
                proverb_lines = proverb.split("，")
                half_lines_height = len(proverb_lines) * line_height // 2

                tft.fill(s3lcd.BLACK)
                color = next(COLOR)

                for count, proverb_line in enumerate(proverb_lines):
                    half_length = tft.write_len(font, proverb_line) // 2

                    tft.write(
                        font,
                        proverb_line,
                        half_width - half_length,
                        half_height - half_lines_height + count * line_height,
                        color,
                    )
                tft.show()
                utime.sleep(5)

    finally:
        tft.deinit()


main()
