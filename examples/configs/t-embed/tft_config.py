""" LilyGo T-embed 170x320 ST7789 display """

from machine import Pin, freq
import s3lcd


BL = Pin(15, Pin.OUT)
POWER = Pin(46, Pin.OUT)

TFA = 0
BFA = 0
WIDE = 1
TALL = 0

freq(240_000_000)


def config(rotation=0, options=0):
    """Configure the display and return an ESPLCD instance."""

    POWER.value(1)
    BL.value(1)

    bus = s3lcd.SPI_BUS(
        1,
        mosi=11,
        sck=12,
        dc=13,
        cs=10,
        pclk=60_000_000,
        swap_color_bytes=True,
    )

    return s3lcd.ESPLCD(
        bus,
        170,
        320,
        inversion_mode=True,
        color_space=s3lcd.RGB,
        reset=9,
        rotation=rotation,
        options=options,
    )


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
    if display_off:
        BL.value(0)
        POWER.value(0)
