"""M5STACK CORES3 with ili9342c 320x240 display"""

from machine import freq
import m5cores3
import s3lcd


TFA = 0
BFA = 0
WIDE = 0
TALL = 1

# Set clock to 240MHz
freq(240000000)

def config(rotation=0, options=0):
    """Configure the display and return an ESPLCD instance."""
    custom_init = [
        (b"\x01", 150),  # soft reset
        (b"\x11", 255),  # exit sleep
        (b"\xCB\x39\x2C\x00\x34\x02",),  # power control A
        (b"\xCF\x00\xC1\x30",),  # power control B
        (b"\xE8\x85\x00\x78",),  # driver timing control A
        (b"\xEA\x00\x00",),  # driver timing control B
        (b"\xED\x64\x03\x12\x81",),  # power on sequence control
        (b"\xF7\x20",),  # pump ratio control
        (b"\xC0\x23",),  # power control,VRH[5:0]
        (b"\xC1\x10",),  # Power control,SAP[2:0];BT[3:0]
        (b"\xC5\x3E\x28",),  # vcm control
        (b"\xC7\x86",),  # vcm control 2
        (b"\x3A\x55",),  # pixel format
        (b"\x36\x08",),  # madctl
        (b"\xB1\x00\x18",),  # frameration control,normal mode full colours
        (b"\xB6\x08\x82\x27",),  # display function control
        (b"\xF2\x00",),  # 3gamma function disable
        (b"\x26\x01",),  # gamma curve selected
        # set positive gamma correction
        (b"\xE0\x0F\x31\x2B\x0C\x0E\x08\x4E\xF1\x37\x07\x10\x03\x0E\x09\x00",),
        # set negative gamma correction
        (b"\xE1\x00\x0E\x14\x03\x11\x07\x31\xC1\x48\x08\x0F\x0C\x31\x36\x0F",),
        (b"\x29", 100),  # display on
    ]

    custom_rotations = (
        (320, 240, 0, 0, False, False, False),
        (240, 320, 0, 0, True, True, False),
        (320, 240, 0, 0, False, True, True),
        (240, 320, 0, 0, True, False, True),
    )

    # To use baudrates above 26.6MHz you must use my firmware or modify the micropython
    # source code to increase the SPI baudrate limit by adding SPI_DEVICE_NO_DUMMY to
    # the .flag member of the spi_device_interface_config_t struct in the
    # machine_hw_spi_init_internal.c file.  Not doing so will cause the ESP32 to crash
    # if you use a baudrate that is too high.

    bus = s3lcd.SPI_BUS(
        1,
        mosi=37,
        sck=36,
        dc=35,
        cs=3,
        pclk=60_000_000,
        swap_color_bytes=True,
    )

    return s3lcd.ESPLCD(
        bus,
        240,
        320,
        inversion_mode=True,
        color_space=s3lcd.BGR,
        rotations=custom_rotations,
        rotation=rotation,
        custom_init=custom_init,
        options=options,
    )


def deinit(tft, display_off=False):
    """Take an ESPLCD instance and Deinitialize the display."""
    tft.deinit()
