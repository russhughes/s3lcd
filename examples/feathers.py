"""
feathers.py
    Smoothly scroll rainbow-colored mirrored random curves across the display.
"""

import random
import math
import utime
import s3lcd
import tft_config


def between(left, right, along):
    """returns a point along the curve from left to right"""
    dist = (1 - math.cos(along * math.pi)) / 2
    return left * (1 - dist) + right * dist


def color_wheel(position):
    """returns a 565 color from the given position of the color wheel"""
    position = (255 - position) % 255

    if position < 85:
        return s3lcd.color565(255 - position * 3, 0, position * 3)

    if position < 170:
        position -= 85
        return s3lcd.color565(0, position * 3, 255 - position * 3)

    position -= 170
    return s3lcd.color565(position * 3, 255 - position * 3, 0)


def main():
    """
    The big show!
    """
    try:
        tft = tft_config.config(tft_config.WIDE)
        tft.init()  # initialize display

        height = tft.height()  # height of display in pixels
        width = tft.width()  # width if display in pixels

        wheel = 0  # color wheel position

        tft.fill(s3lcd.BLACK)  # clear screen
        tft.show()  # show the cleared screen
        half = (height >> 1) - 1  # half the height of the display
        interval = 0  # steps between new points
        increment = 0  # increment per step
        counter = 1  # step counter, overflow to start
        current_y = 0  # current_y value (right point)
        last_y = 0  # last_y value (left point)
        segments = 0  # number of segments to draw
        x_offsets = []  # x offsets for each segment

        tween = 0  # tween value between last_y and current_y
        last_tween = 0  # last tween value
        wheel = 0  # color wheel position

        while True:

            # when the counter exceeds the interval, save current_y to last_y,
            # choose a new random value for current_y between 0 and 1/2 the
            # height of the display, choose a new random interval then reset
            # the counter to 0 and randmonize the number of segments to draw

            if counter > interval:
                last_y = current_y

                current_y = random.randint(0, half)
                counter = 0

                interval = random.randint(10, 100)
                increment = 1 / interval

                segments = random.randint(5, 10)
                offsets = width // segments
                x_offsets = [
                    x * offsets - offsets +1 for x in range(1, segments + 1)
                ]

            # get the next point between last_y and current_y
            last_tween = tween
            tween = int(between(last_y, current_y, counter * increment))

            # draw mirrored pixels across the display at the offsets using the color_wheel effect

            color = color_wheel((wheel))
            for x_offset in x_offsets:
                col = x_offset % width
                tft.pixel(col, half + last_tween, color)
                tft.pixel(col, half - last_tween, color)

            # update the display
            tft.scroll(1, 0)
            tft.show()

            # increment scroll, counter, and wheel
            wheel = (wheel + 1) % 256
            counter += 1

    finally:
        tft.deinit()


main()
