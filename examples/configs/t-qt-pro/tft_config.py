"""LILYGO T-QT Pro GC9107 128x128 display"""

from machine import Pin, SPI, freq
import s3lcd

TFA = 0
BFA = 0
WIDE = 0
TALL = 0

BACKLIGHT = Pin(10, Pin.OUT)

# Set clock to 240MHz
freq(240000000)


def config(rotation=0, options=0):
    """Configure the display and return an ESPLCD instance."""

    # Same as GC9A01
    custom_init = (
        (b"\xEF",),
        (b"\xEB\x14",),
        (b"\xFE",),
        (b"\xEF",),
        (b"\xEB\x14",),
        (b"\x84\x40",),
        (b"\x85\xFF",),
        (b"\x86\xFF",),
        (b"\x87\xFF",),
        (b"\x88\x0A",),
        (b"\x89\x21",),
        (b"\x8A\x00",),
        (b"\x8B\x80",),
        (b"\x8C\x01",),
        (b"\x8D\x01",),
        (b"\x8E\xFF",),
        (b"\x8F\xFF",),
        (b"\xB6\x00\x20",),
        (b"\x3A\x05",),
        (b"\x90\x08\x08\x08\x08",),
        (b"\xBD\x06",),
        (b"\xBC\x00",),
        (b"\xFF\x60\x01\x04",),
        (b"\xC3\x13",),
        (b"\xC4\x13",),
        (b"\xC9\x22",),
        (b"\xBE\x11",),
        (b"\xE1\x10\x0E",),
        (b"\xDF\x21\x0c\x02",),
        (b"\xF0\x45\x09\x08\x08\x26\x2A",),
        (b"\xF1\x43\x70\x72\x36\x37\x6F",),
        (b"\xF2\x45\x09\x08\x08\x26\x2A",),
        (b"\xF3\x43\x70\x72\x36\x37\x6F",),
        (b"\xED\x1B\x0B",),
        (b"\xAE\x77",),
        (b"\xCD\x63",),
        (b"\x70\x07\x07\x04\x0E\x0F\x09\x07\x08\x03",),
        (b"\xE8\x34",),
        (b"\x62\x18\x0D\x71\xED\x70\x70\x18\x0F\x71\xEF\x70\x70",),
        (b"\x63\x18\x11\x71\xF1\x70\x70\x18\x13\x71\xF3\x70\x70",),
        (b"\x64\x28\x29\xF1\x01\xF1\x00\x07",),
        (b"\x66\x3C\x00\xCD\x67\x45\x45\x10\x00\x00\x00",),
        (b"\x67\x00\x3C\x00\x00\x00\x01\x54\x10\x32\x98",),
        (b"\x74\x10\x85\x80\x00\x00\x4E\x00",),
        (b"\x98\x3e\x07",),
        (b"\x35",),
        (b"\x21",),
        (b"\x11", 120),
        (b"\x29", 20),
    )

    custom_rotations = (
        (128, 128, 2, 1, False, True, True),
        (128, 128, 1, 2, True, False, True),
        (128, 128, 2, 1, False, False, False),
        (128, 128, 1, 2, True, True, False),
    )

    # To use baudrates above 26.6MHz you must use my firmware or modify the micropython
    # source code to increase the SPI baudrate limit by adding SPI_DEVICE_NO_DUMMY to the
    # .flag member of the spi_device_interface_config_t struct in the machine_hw_spi_init_internal.c
    # file.  Not doing so will cause the ESP32 to crash if you use a baudrate that is too high.

    bus = s3lcd.SPI_BUS(
        1,
        mosi=2,
        sck=3,
        dc=6,
        cs=5,
        pclk=60_000_000,
        swap_color_bytes=True,
    )

    tft = s3lcd.ESPLCD(
        bus,
        128,
        128,
        inversion_mode=True,
        color_space=s3lcd.BGR,
        reset=1,
        rotations=custom_rotations,
        rotation=rotation,
        custom_init=custom_init,
        options=options,
    )

    BACKLIGHT.value(0)
    return tft


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
    if display_off:
        BACKLIGHT.value(1)
