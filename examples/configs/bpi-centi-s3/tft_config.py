""" BPI-Centi-S3 170x320 ST7789 display """

import s3lcd
from machine import freq, Pin

freq(240_000_000)  # Set esp32s3 cpu frequency to 240MHz
BACKLIGHT = Pin(2, Pin.OUT)
RD = Pin(7, Pin.OUT)

TFA = 0
BFA = 0
WIDE = 1
TALL = 0


def config(rotation=0, options=0):
    BACKLIGHT.value(1)
    RD.value(1)
    bus = s3lcd.I80_BUS(
        (8, 9, 10, 11, 12, 13, 14, 15),
        dc=5,
        wr=6,
        rd=7,
        cs=4,
        # pclk=16_000_000,
        swap_color_bytes=True,
        reverse_color_bits=False,
        pclk_active_neg=False,
        pclk_idle_low=True,
    )

    return s3lcd.ESPLCD(
        bus,
        170,
        320,
        inversion_mode=True,
        color_space=s3lcd.RGB,
        reset=3,
        rotation=rotation,
        dma_rows=32,
        options=options,
    )


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
    if display_off:
        BACKLIGHT.value(0)
        RD.value(0)
