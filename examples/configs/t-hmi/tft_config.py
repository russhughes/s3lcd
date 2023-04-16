""" LilyGo T-HMI 240x320 ST7789 display """

from machine import freq, Pin
import s3lcd

LCD_POWER = Pin(10, Pin.OUT)
BL = Pin(38, Pin.OUT)

TFA = 0
BFA = 0
WIDE = 1
TALL = 0

freq(240_000_000)


def config(rotation=0, options=0):
    """Configure the display and return an ESPLCD instance."""
    i80_bus = s3lcd.I80_BUS(
        (48, 47, 39, 40, 41, 42, 45, 46),
        dc=7,
        wr=8,
        cs=6,
        swap_color_bytes=True,
        reverse_color_bits=False,
    )

    LCD_POWER.value(1)
    BL.value(1)

    return s3lcd.ESPLCD(
        i80_bus,
        240,
        320,
        color_space=s3lcd.RGB,
        inversion_mode=False,
        rotation=rotation,
        dma_rows=32,
        options=options,
    )


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
    if display_off:
        BL.value(0)
        LCD_POWER.value(0)
