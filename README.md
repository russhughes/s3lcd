# ESP_LCD MicroPython driver for ESP32-S3 Devices with ST7789 or compatible displays.

****Warning:**** This work in progress may contain bugs or incorrect documentation.

## Overview

This is a driver for MicroPython for devices using the esp_lcd intel 8080 8-bit parallel bus and SPI interfaces. The driver is written in C and is based on [devbis' st7789_mpy driver.](https://github.com/devbis/st7789_mpy)

I modified the original driver to add the following features:

- Support for esp-idf ESP_LCD intel 8080 parallel and SPI interfaces using DMA.
- Display framebuffer enabling alpha blending for many drawing methods.
- Display Rotation.
- Hardware Scrolling.
- Writing text using fonts converted from True Type fonts.
- Drawing text using eight and 16-bit wide bitmap fonts, including 12 bitmap fonts derived from classic pc text mode fonts.
- Drawing text using 26 included Hershey vector fonts.
- Drawing JPGs using the TJpgDec - Tiny JPEG Decompressor R0.01d. from http://elm-chan.org/fsw/tjpgd/00index.html.
- Drawing PNGs using the pngle library from https://github.com/kikuchan/pngle.
- Writing PNGs from the framebuffer using the PNGenc library from https://github.com/bitbank2/PNGenc
- Drawing and rotating Polygons and filled Polygons.
- Several example programs. The example programs require a tft_config.py module to be present. Some examples require a tft_buttons.py module as well. You may need to modify the tft_buttons.py module to match the pins your device uses.
- tft_config.py and tft_buttons.py configuration examples are provided for:
  - ESP32S3-BOX and BOX-Lite
  - LilyGo T-Display-S3
  - LilyGo T-Dongle-S3
  - LilyGo T-Embed
  - LilyGo T-HMI
  - LilyGo T-QT Pro
  - Seeed Studio WT32-SC01 Plus
  - M5STACK ATOM-S3
  - M5STACK CORES3
  - BananaPi BPI-Centi-S3

## Pre-compiled firmware

The firmware directory contains pre-compiled MicroPython v1.20.0 firmware compiled using ESP IDF v4.4.4. In addition, the firmware includes the C driver and several frozen Python font files. See the README.md file in the fonts folder for more information about the font files.

## Driver API

Note: Curly braces `{` and `}` enclose optional parameters and do not imply a Python dictionary.

## I80_BUS Methods

- `s3lcd.I80_BUS(data, dc, wr, {rd, cs, pclk, bus_width, lcd_cmd_bits, lcd_param_bits, dc_idle_level, dc_cmd_level, dc_dummy_level, dc_data_level, cs_active_high, reverse_color_bits, swap_color_bytes, pclk_active_neg, pclk_idle_low})`

  This method sets the interface configuration of an intel 8080 parallel bus for the ESPLCD driver. The ESPLCD driver will automatically initialize and deinitialize the I80 bus.

    ### Required Parameters:

    - `data` tuple of pin numbers attached to the data bus in least to most significant bit order
    - `dc` data/command pin number
    - `wr` write pin number

    ### Optional Parameters:

    - `rd` read pin number
    - `cs` chip select pin number
    - `pclk pixel` clock frequency in Hz
    - `bus_width` bus width in bits
    - `lcd_cmd_bits` number of bits used to send commands to the LCD
    - `lcd_param_bits` number of bits used to send parameters to the LCD
    - `dc_idle_level` D/C pin level when idle
    - `dc_cmd_level` D/C pin level when sending commands
    - `dc_dummy_level` D/C pin level when sending dummy data
    - `dc_data_level`  D/C pin level when sending data
    - `cs_active_high` CS pin level when active
    - `reverse_color_bits` reverse the order of color bits
    - `swap_color_bytes` swap the order of color bytes
    - `pclk_active_neg` pixel clock is active negative
    - `pclk_idle_low` pixel clock is idle low

## SPI_BUS Methods

- `s3lcd.SPI_BUS(spi_host, sck, mosi, dc, {cs, spi_mode, pclk, lcd_cmd_bits, lcd_param_bits, dc_idle_level, dc_as_cmd_phase, dc_low_on_data, octal_mode, lsb_first, swap_color_bytes})`

  This method sets the interface configuration of an SPI bus for the ESPLCD driver. The ESPLCD driver will automatically initialize and deinitialize the SPI bus.

    ### Required Parameters:

    - `spi_host` SPI host to use.
    - `sck` sck pin number
    - `mosi` MOSI pin number
    - `dc` D/C pin number
    -
    ### Optional Parameters:

    - `cs` CS pin number
    - `spi_mode` Traditional SPI mode (0~3)
    - `pclk` Frequency of pixel clock
    - `lcd_cmd_bits` Bit-width of LCD command
    - `lcd_param_bits` Bit-width of LCD parameter
    - `dc_idle_level`  D/C pin level when idle
    - `dc_as_cmd_phase` D/C line value is encoded into SPI transaction command phase
    - `dc_low_on_data` If this flag is enabled, D/C line = 0 means transfer data, D/C line = 1 means transfer command
    - `octal_mode` transmit using octal mode (8 data lines)
    - `lsb_first` transmit LSB bit first
    - `swap_color_bytes` (bool) Swap data byte order

## ESPLCD Methods

- `s3lcd.ESPLCD(bus, width, height, {reset, rotations, rotation, inversion, dma_rows, options})`

    ### Required positional arguments:
    - `bus` I80_BUS or SPI_BUS object
    - `width` display width
    - `height` display height

    ### Optional keyword arguments:

    - `reset` reset pin number

    - `rotations` Creates a custom rotation table. A rotation table is a list of tuples for each `rotation` containing the width, height, x_gap, y_gap, swap_xy, mirror_x, and mirror_y values for each rotation.

      Default `rotations` are included for the following display sizes:

      | Display | Default Orientation Tables |
      | ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
      | 320x480 | ((320, 480, 0, 0, false, true,  false), (480, 320, 0, 0, true,  false, false), (320, 480, 0, 0, false, false, true), (480, 320, 0, 0, true,  true,  true)) |
      | 240x320 | ((240, 320, 0, 0, false, false, false), (320, 240, 0, 0, true, true, false), (240, 320, 0, 0, false, true, true), (320, 240, 0, 0, true, false, true))         |
      | 170x320 | ((170, 320, 35, 0, false, false, false), (320, 170, 0, 35, true, true, false), (170, 320, 35, 0, false, true, true), (320, 170, 0, 35, true, false, true))     |
      | 240x240 | ((240, 240, 0, 0, false, false, false), (240, 240, 0, 0, true, true, false), (240, 240, 0, 80, false, true, true), (240, 240, 80, 0, true, false, true))       |
      | 135x240 | ((135, 240, 52, 40, false, false, false), (240, 135, 40, 53, true, true, false), (135, 240, 53, 40, false, true, true), (240, 135, 40, 52, true, false, true)) |
      | 128x160 | ((128, 160, 0, 0, false, false, false), (160, 128, 0, 0, true, true, false), (128, 160, 0, 0, false, true, true), (160, 128, 0, 0, true, false, true))         |
      | 80x160  | ((80, 160, 26, 1, false, false, false), (160, 80, 1, 26, true, true, false), (80, 160, 26, 1, false, true,  true), (160, 80, 1, 26, true,  false, true))       |
      | 128x128 | ((128, 128, 2, 1, false, false, false), (128, 128, 1, 2, true, true, false), (128, 128, 2, 3, false, true, true), (128, 128, 3, 2, true, false, true))         |

      You may define up to 4 rotations.

    - `rotation` sets the active display rotation according to the orientation table.

      The default orientation table defines four counterclockwise rotations for 240x320, 240x240, 134x240, 128x160, 80x160, and 128x128 displays with the LCD's ribbon cable at the bottom of the display. The default rotation is Portrait (0 degrees).

      | Index | Rotation
      | ----- | ------------------------------- |
      | 0     | Portrait (0 degrees)            |
      | 1     | Landscape (90 degrees)          |
      | 2     | Reverse Portrait (180 degrees)  |
      | 3     | Reverse Landscape (270 degrees) |

    - `inversion` Sets the display color inversion mode if True and clears the color inversion mode if false.

    - `dma_rows` Sets the number of the framebuffer rows to transfer to the display in a single DMA transaction. The default value is 16 rows. Larger values may perform better but use more DMA-capable memory from the ESP-IDF heap. On the other hand, using a large value may starve other ESP-IDF functions like WiFi of memory.

    - `options` Sets driver option flags.

      | Option       | Description                                                                                              |
      | ------------ | -------------------------------------------------------------------------------------------------------- |
      | s3lcd.WRAP   | pixels, lines, polygons, and Hershey text will wrap around the display both horizontally and vertically. |
      | s3lcd.WRAP_H | pixels, lines, polygons, and Hershey text will wrap around the display horizontally.                     |
      | s3lcd.WRAP_V | pixels, lines, polygons, and Hershey text will wrap around the display vertically.                       |

- `deinit()`

    Frees the buffer memory and deinitializes the I80_BUF or SPI_BUF object. Call this method before reinitializing the display without performing a hard reset.

- `show()`

    Update the display from the framebuffer. You must use the show() method to transfer the framebuffer to the display. This method blocks until the display refresh is complete.

- `inversion_mode(bool)` Sets the display color inversion mode if True, clears the display color inversion mode if False.

- `init()`

  Must be called to initialize the display.

- clear({ 8_bit_color})

    Fast clear the framebuffer by setting the high and low bytes of the color to the specified value.

    Optional parameters:
        -- 8_bit_color defaults to 0x00 BLACK

- `fill({color, alpha})`

  Fill the framebuffer with the specified color, optionally `alpha` blended with the background. The `color` defaults to BLACK, and the `alpha` defaults to 255.

- `pixel(x, y {, color, alpha})`

  Set the specified pixel to the given `color`. The `color` defaults to WHITE, and the `alpha` defaults to 255.

- `line(x0, y0, x1, y1 {, color, alpha})`

  Draws a single line with the provided `color` from (`x0`, `y0`) to (`x1`, `y1`). The `color` defaults to BLACK, and the `alpha` defaults to 255.

- `hline(x, y, w {, color, alpha})`

  Draws a horizontal line with the provided `color` and `length` in pixels. The `color` defaults to BLACK, and the `alpha` defaults to 255.

- `vline(x, y, length {, color, alpha})`

  Draws a vertical line with the provided `color` and `length` in pixels. The `color` defaults to BLACK, and the `alpha` defaults to 255.

- `rect(x, y, width, height {, color, alpha})`

  Draws a rectangle with the specified dimensions from (`x`, `y'). The `color` defaults to BLACK, and the `alpha` defaults to 255.

- `fill_rect(x, y, width, height {, color, alpha})`

  Fills a rectangle `width` by `height` starting at `x`, `y' with `color` optionally `alpha` blended with the background. The `color` defaults to BLACK, and `alpha` defaults to 255.

- `circle(x, y, r {, color, alpha})`

  Draws a circle with radius `r` centered at the (`x`, `y') coordinates in the given `color`. The `color` defaults to BLACK, and the `alpha` defaults to 255.

- `fill_circle(x, y, r {, color, alpha})`

  Draws a filled circle with radius `r` centered at the (`x`, `y') coordinates in the given `color`. The `color` defaults to BLACK, and the `alpha` defaults to 255.

- `blit_buffer(buffer, x, y, width, height {, alpha})`

  Copy bytes() or bytearray() content to the framebuffer. Note: every color requires 2 bytes in the array, the `alpha` defaults to 255.

- `text(font, s, x, y {, fg, bg, alpha})`

  Writes text to the framebuffer using the specified bitmap `font` with the coordinates as the upper-left corner of the text. The optional arguments `fg` and `bg` can set the foreground and background colors of the text; otherwise, the foreground color defaults to `WHITE`, and the background color defaults to `BLACK`. `alpha` defaults to 255. See the `README.md` in the `fonts/bitmap` directory, for example fonts.

- `write(bitmap_font, s, x, y {, fg, bg, alpha})`

  Writes text to the framebuffer using the specified proportional or Monospace bitmap font module starting with the coordinates as the upper-left corner of the text. The foreground and background colors of the text are set by the optional arguments `fg` and `bg`; otherwise, the foreground color defaults to `WHITE`, and the background color defaults to `BLACK`. The `alpha` defaults to 255.

  See the `README.md` in the `truetype/fonts` directory, for example fonts. Returns the width of the string as printed in pixels. This method accepts UTF8 encoded strings.

  The `font2bitmap` utility creates compatible 1-bit per pixel bitmap modules from Proportional or Monospaced True Type fonts. The character size, foreground, background colors, and characters in the bitmap module may be specified as parameters. Use the -h option for details.

- `write_len(bitap_font, s)`

  Returns the string's width in pixels if printed in the specified font.

- `draw(vector_font, s, x, y {, fg, scale, alpha})`

  Draws text to the framebuffer using the specified Hershey vector font with the coordinates as the lower-left corner of the text. The foreground color of the text can be set by the optional argument `fg`. Otherwise, the foreground color defaults to `WHITE`. The size of the text is modified by specifying a `scale` value. The `scale` value must be larger than 0 and can be a floating-point or an integer value. The `scale` value defaults to 1.0. The `alpha` defaults to 255. See the README.md in the `vector/fonts` directory, for example fonts and the utils directory for a font conversion program.

- `draw_len(vector_font, s {, scale})`

  Returns the string's width in pixels if drawn with the specified font.

- `jpg(jpg, x, y)`

  Draws a JPG file in the framebuffer at the given `x` and `y' coordinates as the upper left corner of the image. This method requires an additional 3100 bytes of memory for its work buffer. The jpg may be a filename or a bytes() or bytearray() object. The jpg will wil be clipped if is not able to fit fully in the framebuffer.

- `jpg_decode(jpg_filename {, x, y, width, height})`

  Decodes a jpg file and returns it or a portion of it as a tuple composed of (buffer, width, height). The buffer is a color565 blit_buffer compatible byte array. The buffer will require width * height * 2 bytes of memory.

  If the optional x, y, width, and height parameters are given, the buffer will only contain the specified area of the image. See examples/T-DISPLAY/clock/clock.py and examples/T-DISPLAY/toasters_jpg/toasters_jpg.py for examples.

- `png(png, x, y)`

  Draws a PNG file in the framebuffer with the upper left corner of the image at the given `x` and `y' coordinates. The png may be a filename or a bytes() or bytearray() object. The png will wil be clipped if it is not able to fit fully in the framebuffer. Transparency is supported; see the alien.py program in the examples/png folder.

- `png_write(file_name{ x, y, width, height})`

  Writes the framebuffer to a png file named `file_name` using PNGenc from https://github.com/bitbank2/PNGenc.

  #### optional parameters:
    - x: the first column of the framebuffer to start writing.
    - y: the first row of the framebuffer to start writing.
    - width: the width of the area to write
    - height: the height of the area to write

  Returns file size in bytes.

- `polygon_center(polygon)`

   Returns the center of the `polygon` as an (x, y) tuple. The `polygon` should consist of a list of (x, y) tuples forming a closed convex polygon.

- `fill_polygon(polygon, x, y, color {, alpha, angle, center_x, center_y})`

  Draws a filled `polygon` at the `x`, and `y' coordinates in the `color` given. The `alpha` defaults to 255. The polygon may be rotated `angle` radians about the `center_x` and `center_y` points. The polygon should consist of a list of (x, y) tuples forming a closed convex polygon.

  See the TWATCH-2020 `watch.py` demo for an example.

- `polygon(polygon, x, y, color {, alpha, angle, center_x, center_y)`

  Draws a `polygon` at the `x`, and `y' coordinates in the `color` given. The `alpha` defaults to 255. The polygon may be rotated `angle` radians about the `center_x` and `center_y` points. The polygon should consist of a list of (x, y) tuples forming a closed convex polygon.

  See the `roids.py` for an example.

- `bitmap(bitmap, x , y {, alpha, index})` or `bitmap((bitmap_as_bytes, w, h), x , y {, alpha})`

  Draws a bitmap using the specified `x`, `y' coordinates as the upper-left corner of the `bitmap`.

  - If the `bitmap` parameter is a bitmap module, the `index` parameter may be specified to select a specific bitmap from the module. The `index` parameter must be an integer value greater than or equal to 0 and less than the number of bitmaps in the module. The `index` value defaults to 0. 8-bit per pixel.

  - If the `bitmap_module` parameter is a tuple, the tuple must contain a bitmap as a byte array, the width of the bitmap in pixels, and the height of the bitmap in pixels. `alpha` defaults to 255.

  Using the Pillow Python Imaging Library, the `imgtobitmap.py` utility creates compatible 1 to 8-bit per-pixel bitmap modules from image files.

  The `monofont2bitmap.py` utility creates compatible 1 to 8-bit per-pixel bitmap modules from Monospaced True Type fonts. See the `inconsolata_16.py`, `inconsolata_32.py` and `inconsolata_64.py` files in the `examples/mono_fonts` folder for sample modules and the `mono_font.py` program for an example using the generated modules.

  You can specify the character sizes, foreground and background colors, bit per pixel, and characters to include in the bitmap module as parameters. To learn more, use the -h option. Using bit-per-pixel settings larger than one can create antialiased characters at the cost of increased memory usage.

- `width()`

  Returns the current width of the display in pixels. (i.e., a 135x240 display rotated 90 degrees is 240 pixels wide)

- `height()`

  Returns the current height of the display in pixels. (i.e., a 135x240 display rotated 90 degrees is 135 pixels high)

- `rotation(r)`

  Sets the rotation of the logical display in a counterclockwise direction. 0-Portrait (0 degrees), 1-Landscape (90 degrees), 2-Inverse Portrait (180 degrees), 3-Inverse Landscape (270 degrees)

- `scroll(xstep, ystep{, fill=0})`

  Scrolls the framebuffer using software in the given direction.

  ### Required parameters:

  - xstep: Number of pixels to scroll in the x direction. Negative values scroll left, positive values scroll right.
  - ystep: Number of pixels to scroll in the y direction. Negative values scroll up, positive values scroll down.

  ### Optional parameters:

  - fill: Fill color for the new pixels.

The module exposes predefined colors:
  `BLACK`, `BLUE`, `RED`, `GREEN`, `CYAN`, `MAGENTA`, `YELLOW`, and `WHITE`

## Hardware Scrolling

The st7789 display controller contains a 240 by 320-pixel frame buffer used to store the pixels for the display. For scrolling, the frame buffer consists of three separate areas: The (`tfa`) top fixed area, the (`height`) scrolling area, and the (`bfa`) bottom fixed area. The `tfa` is the upper portion of the frame buffer in pixels not to scroll. The `height` is the center portion of the frame buffer in pixels to scroll. The `bfa` is the lower portion of the frame buffer in pixels not to scroll. These values control the ability to scroll the entire or a part of the display.

For displays that are 320 pixels high, setting the `tfa` to 0, `height` to 320, and `bfa` to 0 will allow scrolling of the entire display. To scroll a portion of the display, you can set the `tfa` and `bfa` to a non-zero value. `tfa` + `height` + `bfa` = should equal 320; otherwise, the scrolling mode is undefined.

Displays less than 320 pixels high, the `tfa`, `height`, and `bfa` must be adjusted to compensate for the smaller LCD panel. The actual values will vary depending on the configuration of the LCD panel. For example, scrolling the entire 135x240 TTGO T-Display requires a `tfa` value of 40, `height` value of 240, and `bfa` value of 40 (40+240+40=320) because the T-Display LCD shows 240 rows starting at the 40th row of the frame buffer, leaving the last 40 rows of the frame buffer undisplayed.

Other displays, like the Waveshare Pico LCD 1.3-inch 240x240 display, require the `tfa` set to 0, `height` set to 240, and `bfa` set to 80 (0+240+80=320) to scroll the entire display. The Pico LCD 1.3 shows 240 rows starting at the 0th row of the frame buffer, leaving the last 80 rows undisplayed.

The `vscsad` method sets the (VSSA) Vertical Scroll Start Address. The VSSA sets the line in the frame buffer that will be the first line after the `tfa`.

    The ST7789 datasheet warns:

    The value of the vertical scrolling start address is absolute (with referenceto the frame memory), it must not enter the fixed area defined by Vertical Scrolling Definition, otherwise undesirable image will be displayed on the panel.

- `vscrdef(tfa, height, bfa)` Set the vertical scrolling parameters.

  `tfa` is the top fixed area in pixels. The top fixed area is the upper portion of the display frame buffer that will not be scrolled.

  `height` is the total height in pixels of the area scrolled.

  `bfa` is the bottom fixed area in pixels. The bottom fixed area is the lower portion of the display frame buffer that will not be scrolled.

- `vscsad(vssa)` Set the vertical scroll address.

  `vssa` is the vertical scroll start address in pixels. The vertical scroll start address is the line in the frame buffer that will be the first line shown after the TFA.

## Helper functions

- `color565(r, g, b)`

  Pack a color into 2-byte rgb565 format

- `map_bitarray_to_rgb565(bitarray, buffer, width {, color, bg_color})`

  Convert a `bitarray` to the rgb565 color `buffer` suitable for blitting. Bit
  1 in `bitarray` is a pixel with `color` and 0 - with `bg_color`.


# Building the firmware

See the MicroPython [Getting Started](https://docs.micropython.org/en/latest/develop/gettingstarted.html)
page for more detailed information on building the MicroPython firmware.

## Clone the Repositories


```
$ git clone git@github.com:micropython/micropython.git
$ git clone https://github.com/russhughes/s3lcd.git
```

Compile the cross compiler if you haven't already

```
$ make -C micropython/mpy-cross
```

## ESP32 MicroPython 1.14 thru 1.19

Change to the ESP32 port directory

```
$ cd micropython/ports/esp32
```

Compile the module with specified USER_C_MODULES dir

```
$ make USER_C_MODULES=../../../../s3lcd/src/micropython.cmake clean submodules all
```

Erase the target device if this is the first time uploading this
firmware

```
$ make USER_C_MODULES=../../../../s3lcd/src/micropython.cmake erase
```

Upload the new firmware

```
$ make USER_C_MODULES=../../../../s3lcd/src/micropython.cmake deploy
```

## ESP32 MicroPython 1.20 and later


Change to the ESP32 port directory, and build the firmware

```
$ cd micropython/ports/esp32

$ make \
    BOARD=ESP32_GENERIC \
    BOARD_VARIANT=SPIRAM \
    USER_C_MODULES=../../../../s3lcd/src/micropython.cmake \
    FROZEN_MANIFEST=../../../../s3lcd/manifest.py \
    clean submodules all
```

Erase the flash and deploy on your device

```
$ make \
    BOARD=ESP32_GENERIC \
    BOARD_VARIANT=SPIRAM \
    USER_C_MODULES=../../../../s3lcd/src/micropython.cmake \
    FROZEN_MANIFEST=../../../../s3lcd/manifest.py \
    erase deploy
```

## RP2040 MicroPython 1.20 and later

Change to the RP2 port directory, and build the firmware

```
$ cd micropython/ports/rp2
$ make \
    BOARD=RPI_PICO \
    FROZEN_MANIFEST=../../../../gc9a01c/manifest.py \
    USER_C_MODULES=../../../gc9a01c/src/micropython.cmake \
    clean submodules all
```

Flash the firmware.uf2 file from the build-${BOARD} directory to your device.

## Thanks go out to:

- https://github.com/devbis for the original driver this is based on.
- https://github.com/hklang10 for letting me know of the new mp_raise_ValueError().
- https://github.com/aleggon for finding the correct offsets for 240x240 displays and for discovering issues compiling STM32 ports.

-- Russ
