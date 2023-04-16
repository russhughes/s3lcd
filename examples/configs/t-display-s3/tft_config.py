""" LilyGo T-DISPLAY-S3 170x320 ST7789 display """

from machine import Pin, freq
import s3lcd


LCD_POWER = Pin(15, Pin.OUT)
RD = Pin(9, Pin.OUT)
BACKLIGHT = Pin(38, Pin.OUT)

TFA = 0
BFA = 0
WIDE = 1
TALL = 0

freq(240_000_000)


def config(rotation=0, options=0):
    """Configure the display and return an ESPLCD instance."""
    LCD_POWER.value(1)
    RD.value(1)
    BACKLIGHT.value(1)

    bus = s3lcd.I80_BUS(
        (39, 40, 41, 42, 45, 46, 47, 48),
        dc=7,
        wr=8,
        cs=6,
        pclk=20_000_000,
        swap_color_bytes=True,
        reverse_color_bits=False,
    )

    return s3lcd.ESPLCD(
        bus,
        170,
        320,
        inversion_mode=True,
        color_space=s3lcd.RGB,
        reset=5,
        rotation=rotation,
        options=options,
    )


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
    if display_off:
        BACKLIGHT.value(0)
        RD.value(0)
        LCD_POWER.value(0)
