""" LilyGo T-Dongle-S3 80x160 ST7735 display """

from machine import Pin, freq
import s3lcd

BL = Pin(37, Pin.OUT)

TFA = 1
BFA = 1
WIDE = 1
TALL = 0

freq(240_000_000)


def config(rotation=0, options=0):
    """Configure the display and return an ESPLCD instance."""

    BL.value(1)

    bus = s3lcd.SPI_BUS(
        2, mosi=3, sck=5, dc=2, cs=4, pclk=50000000, swap_color_bytes=True
    )

    return s3lcd.ESPLCD(
        bus,
        80,
        160,
        inversion_mode=True,
        color_space=s3lcd.BGR,
        reset=1,
        rotation=rotation,
        dma_rows=32,
        options=options,
    )


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
    if display_off:
        BL.value(0)
