"""
hershey.py

    Demo program that draws greetings on display cycling thru hershey fonts and colors.

"""

import time
import s3lcd
import tft_config

# Load several frozen fonts from flash

import greeks
import italicc
import italiccs
import meteo
import romanc
import romancs
import romand
import romanp
import romans
import scriptc
import scripts

tft = tft_config.config(tft_config.TALL, options=s3lcd.WRAP_V)


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

# List Hershey fonts
FONTS = (
    greeks,
    italicc,
    italiccs,
    meteo,
    romanc,
    romancs,
    romand,
    romanp,
    romans,
    scriptc,
    scripts,
)

# Create a cycle of tuples consisting of a FONT[] and the HEIGHT of the next FONT[] in the cycle
FONT = cycle(
    [(font, FONTS[(i + 1) % len(FONTS)].HEIGHT) for i, font in enumerate(FONTS)]
)

# Greetings
GREETINGS = (
    "Ahoy",
    "Bonjour",
    "Bonsoir",
    "Buenos Dias",
    "Buenas tardes",
    "Buenas Noches",
    "Ciao",
    "Dude",
    "Good Morning",
    "Good Day",
    "Good Evening",
    "Hello",
    "Hey",
    "Hi",
    "Hiya",
    "Hola",
    "How Are You",
    "How Goes It",
    "Howdy",
    "How you doing",
    "Konnichiwa",
    "Salut",
    "Shalom",
    "Welcome",
    "What's Up",
    "Yo!",
)

# Create a cycle of tuples consisting of a list of words from a GREETING, the number of spaces+1
# in the that GREETING, and the number of spaces+1 in the next GREETING of the cycle
GREETING = cycle(
    [
        (
            greeting.split(),
            greeting.count(" ") + 1,
            GREETINGS[(i + 1) % len(GREETINGS)].count(" ") + 1,
        )
        for i, greeting in enumerate(GREETINGS)
    ]
)


def main():
    """Scroll greetings on the display cycling thru Hershey fonts and colors"""

    try:
        tft.init()
        tft.fill(s3lcd.BLACK)

        height = tft.height()
        width = tft.width()

        if width > 240:
            size = 1.5
        elif width > 80:
            size = 1
        else:
            size = 0.5

        to_scroll = 0
        wheel = 0
        ticks = 1000 // 45

        while True:
            last = time.ticks_ms()
            # if we have scrolled high enough for the next greeting
            if to_scroll == 0:
                font = next(FONT)  # get the next font
                greeting = next(GREETING)  # get the next greeting
                wheel = (wheel + 29) % 255  # update the color wheel
                color = next(COLOR)  # get the next color
                lines = greeting[2]  # number of lines in the greeting
                to_scroll = lines * font[1] + 4 * size  # number of rows to scroll

                # draw each line of the greeting
                for i, word in enumerate(greeting[0][::-1]):
                    word_len = tft.draw_len(font[0], word, size)  # width in pixels
                    col = (
                        0 if word_len > width else (width // 2 - word_len // 2)
                    )  # column to center
                    row = height - ((i + 1) * font[0].HEIGHT) % height  # row to draw
                    tft.draw(font[0], word, col, row, color, size)  # draw the word

            tft.show()  # show the display
            tft.scroll(0, -1)  # scroll the display
            to_scroll -= 1  # update rows left to scroll

            if time.ticks_ms() - last < ticks:
                time.sleep_ms(ticks - (time.ticks_ms() - last))

    finally:
        tft.deinit()


main()
