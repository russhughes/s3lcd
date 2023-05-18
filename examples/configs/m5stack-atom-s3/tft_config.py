""" M5STACK ATOM-S3 128x128 display """

from machine import Pin, freq
import s3lcd

BL = Pin(16, Pin.OUT)

TFA = 1
BFA = 1
WIDE = 1
TALL = 0

freq(240_000_000)


def config(rotation=0, options=0):
    """Configure the display and return an ESPLCD instance."""

    BL.value(1)

    custom_rotations = (
        (128, 128, 2, 1, False, False, False),
        (128, 128, 1, 2, True,  True,  False),
        (128, 128, 2, 1, False, True,  True),
        (128, 128, 1, 2, True,  False, True),
    )

    bus = s3lcd.SPI_BUS(
        2, mosi=21, sck=17, dc=33, cs=15, pclk=27000000, swap_color_bytes=True
    )

    return s3lcd.ESPLCD(
        bus,
        128,
        128,
        inversion_mode=True,
        color_space=s3lcd.BGR,
        reset=34,
        rotations=custom_rotations,
        rotation=rotation,
        dma_rows=32,
        options=options,
    )


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
    if display_off:
        BL.value(0)
