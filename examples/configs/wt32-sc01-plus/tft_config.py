""" WT32-SC01 Plus 320x480 ST7796 display """

from machine import Pin, freq
import s3lcd


BL = Pin(45, Pin.OUT)

TFA = 0
BFA = 0
WIDE = 1
TALL = 0

freq(240_000_000)


def config(rotation=0, options=0):
    """Configure the display and return an ESPLCD instance."""

    custom_init = (
        (b"\x01", 120),
        (b"\x11", 120),
        (b"\xF0\xC3",),
        (b"\xF0\x96",),
        (b"\x36\x48",),
        (b"\x3A\x55",),
        (b"\xB4\x01",),
        (b"\xB6\x80\x02\x3B",),
        (b"\xE8\x40\x8A\x00\x00\x29\x19\xA5\x33",),
        (b"\xC1\x06",),
        (b"\xC2\xA7",),
        (b"\xC5\x18", 120),
        (b"\xE0\xF0\x09\x0B\x06\x04\x15\x2F\x54\x42\x3C\x17\x14\x18\x1B",),
        (b"\xE1\xE0\x09\x0B\x06\x04\x03\x2B\x43\x42\x3B\x16\x14\x17\x1B", 120),
        (b"\xF0\x3C",),
        (b"\xF0\x69",),
        (b"\x29",),
    )

    BL.value(1)

    bus = s3lcd.I80_BUS(
        (9, 46, 3, 8, 18, 17, 16, 15),
        dc=0,
        wr=47,
        cs=6,
        pclk=20_000_000,
        swap_color_bytes=True,
        reverse_color_bits=False,
    )

    return s3lcd.ESPLCD(
        bus,
        320,
        480,
        inversion_mode=True,
        color_space=s3lcd.BGR,
        reset=4,
        rotation=rotation,
        custom_init=custom_init,
        options=options,
    )


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
    if display_off:
        BL.value(0)
