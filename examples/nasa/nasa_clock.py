"""
nasaclock.py

    Displays a clock over background images on the display.

    The buttons on the module can be used to set the time.

    Background images courtesy of the NASA image and video gallery available at
    https://images.nasa.gov/

    The Font is Copyright 2018 The Pacifico Project Authors (https://github.com/googlefonts/Pacifico)
    This Font Software is licensed under the SIL Open Font License, Version 1.1.
    This license is copied below, and is also available with a FAQ at:
    http://scripts.sil.org/OFL

"""
import gc
import sys
import utime
from machine import Pin, SPI, RTC
import s3lcd
import tft_config
import pacifico as font

tft = tft_config.config(tft_config.WIDE)
rtc = RTC()
BCKGROUND_LOCK = 0  # prevents background change while > 0


def cycle(items):
    """return the next item in a list"""
    try:
        len(items)
    except TypeError:
        cache = []
        for i in items:
            yield i
            cache.append(i)

        items = cache
    while items:
        yield from items


class Button:
    """
    Debounced pin handler

    Modifed from https://gist.github.com/jedie/8564e62b0b8349ff9051d7c5a1312ed7
    """

    def __init__(self, pin, callback, trigger=Pin.IRQ_FALLING, debounce=350):
        self.callback = callback
        self.debounce = debounce
        self._next_call = utime.ticks_ms() + self.debounce
        pin.irq(trigger=trigger, handler=self.debounce_handler)

    def call_callback(self, pin):
        """call the callback function for the pin"""
        self.callback(pin)

    def debounce_handler(self, pin):
        """debounce the pin"""
        if utime.ticks_ms() > self._next_call:
            self._next_call = utime.ticks_ms() + self.debounce
            self.call_callback(pin)


def hour_pressed(pin):
    """Increment the hour"""
    global BCKGROUND_LOCK
    tm = rtc.datetime()
    rtc.init((tm[0], tm[1], tm[2], tm[3], tm[4], tm[5] + 1, tm[6], tm[7]))
    BCKGROUND_LOCK = 10


def minute_pressed(pin):
    """increment the minute"""
    global BCKGROUND_LOCK
    tm = rtc.datetime()
    rtc.init((tm[0], tm[1], tm[2], tm[3], tm[4] + 1, tm[5], tm[6], tm[7]))
    BCKGROUND_LOCK = 10


def main():
    """
    Initialize the display and show the time
    """
    global BCKGROUND_LOCK

    try:
        tft.init()

        tft.clear(0)
        tft.show()
        # image, Time Column, Time Row, Time Color
        background = cycle(
            (
                "nasa01.jpg",
                "nasa02.jpg",
                "nasa03.jpg",
                "nasa04.jpg",
                "nasa05.jpg",
                "nasa06.jpg",
                "nasa07.jpg",
                "nasa08.jpg",
                "nasa09.jpg",
                "nasa10.jpg",
                "nasa11.jpg",
                "nasa12.jpg",
                "nasa13.jpg",
                "nasa14.jpg",
                "nasa15.jpg",
                "nasa16.jpg",
                "nasa17.jpg",
                "nasa18.jpg",
                "nasa19.jpg",
                "nasa20.jpg",
                "nasa21.jpg",
                "nasa22.jpg",
                "nasa23.jpg",
                "nasa24.jpg",
                "nasa25.jpg",
            )
        )

        digit_columns = []
        background_change = True
        time_col = tft.width() // 2 - font.MAX_WIDTH * 5 // 2
        time_row = tft.height() // 2 - font.HEIGHT // 2
        time_color = s3lcd.WHITE
        last_time = "-----"

        #
        # change these to match your button connections
        #

        # Button(pin=Pin(0, mode=Pin.IN), callback=hour_pressed)
        # Button(pin=Pin(14, mode=Pin.IN), callback=minute_pressed)

        while True:

            # create new digit_backgrounds and change the background image
            if background_change:
                image = next(background)
                background_change = False

                # clear the old backgrounds and gc
                digit_background = []
                gc.collect()

                # select the next image from the nasa_{WIDTH}x{HEIGHT} directory
                image_file = f"nasa_{tft.width()}x{tft.height()}/{image}"

                # calculate the starting column for each time digit
                digit_columns = [
                    time_col + digit * font.MAX_WIDTH for digit in range(5)
                ]

                # nudge the ':' to the right since it is narrower then the digits
                digit_columns[2] += font.MAX_WIDTH // 4

                # get the background bitmap behind each clock digit from the jpg file and store it
                # in a list so it can be used to write each digit simulating transparency. The jpg_decode
                # method returns a tuple of the bitmap data, width and height of the bitmap in pixels.

                digit_background = [
                    tft.jpg_decode(
                        image_file,  # jpg file name
                        digit_columns[digit],  # column to start bitmap at
                        time_row,  # row to start bitmap at
                        font.MAX_WIDTH
                        if (digit != 2)
                        else font.MAX_WIDTH // 2,  # width of bitmap to save
                        font.HEIGHT,
                    )  # height of bitmap to save
                    for digit in range(5)
                ]

                # draw the background image
                tft.jpg(image_file, 0, 0)
                tft.show()

                # cause all digits to be updated
                last_time = "-----"

            # get the current hour and minute
            _, _, _, hour, minute, _, _, _ = utime.localtime()

            # 12 hour time
            if hour == 0:
                hour = 12
            if hour > 12:
                hour -= 12

            # format time  string as "HH:MM"
            time = f"{hour:2d}:{minute:02d}"

            # loop through the time string
            for digit in range(5):

                # Check if this digit has changed
                if time[digit] != last_time[digit]:

                    # digit 1 is the hour, change the background every hour
                    # digit 3 is the tens of the minute, change the background every 10 minutes
                    # digit 4 is the ones of the minute, change the background every minute
                    if digit == 3 and last_time[digit] != "-" and BCKGROUND_LOCK == 0:
                        background_change = True

                    # draw the changed digit, don't fill to the right
                    # of the ':' because it is always the same width

                    tft.bitmap(digit_background[digit], digit_columns[digit], time_row)

                    tft.write(
                        font,  # the font to write to the display
                        time[digit],  # time string digit to write
                        digit_columns[digit],  # write to the correct column
                        time_row,  # write on row
                        time_color,  # color of time text
                        s3lcd.TRANSPARENT,
                    )  # transparent background

            # save the current time
            last_time = time

            # decrement the background lock
            if BCKGROUND_LOCK:
                BCKGROUND_LOCK -= 1

            tft.show()
            utime.sleep(0.5)
            gc.collect()

    finally:
        tft.deinit()


main()
