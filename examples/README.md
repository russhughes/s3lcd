# Example Programs


## bitarray.py

    An example using map_bitarray_to_rgb565 to draw sprites


## bitmap_fonts.py

    Cycles through all characters of four bitmap fonts on the display

## chango.py

    Proportional font test for font2bitmap converter.


## clock.py

    Displays a clock over a background image on the display.

    The buttons on the module can be used to set the time.

    Background images courtesy of the NASA image and video gallery available at
    https://images.nasa.gov/

    The Font is Copyright 2018 The Pacifico Project Authors (https://github.com/googlefonts/Pacifico)
    This Font Software is licensed under the SIL Open Font License, Version 1.1.
    This license is copied below, and is also available with a FAQ at:
    http://scripts.sil.org/OFL


## feathers.py

    Smoothly scroll rainbow-colored mirrored random curves across the display.


## hello.py

    Writes "Hello!" in random colors at random locations on the display.


## hershey.py

    Demo program that draws greetings on display cycling thru hershey fonts and colors.

## jpg.py

    Draw a full screen jpg using the slower but less memory intensive method of blitting
    each Minimum Coded Unit (MCU) block. Usually 8×8pixels but can be other multiples of 8.

    bigbuckbunny.jpg (c) copyright 2008, Blender Foundation / www.bigbuckbunny.org


### mono_fonts.py
    mono_fonts.py test for monofont2bitmap converter and bitmap method. This is the older method of
    converting monofonts to bitmaps.  See the newer method in prop_fonts/chango.py that works with
    mono and proportional fonts using the write method.


## noto_fonts.py

    Writes the names of three Noto fonts centered on the display
    using the font. The fonts were converted from True Type fonts using
    the font2bitmap utility.


## proverbs.py

    Displays what I hope are chinese proverbs in simplified chinese to test UTF-8 font support.


## roids.py

    Asteroids style game demo using polygons.


## scroll.py

    Smoothly scroll all characters of a font up the display.
    Fonts heights must be even multiples of the screen height (i.e. 8 or 16 pixels high).


## tiny_toasters.py

    Flying Tiny Toasters for smaller displays (like the ST7735)

    Uses spritesheet from CircuitPython_Flying_Toasters pendant project
    https://learn.adafruit.com/circuitpython-sprite-animation-pendant-mario-clouds-flying-toasters

    Convert spritesheet bmp to tft.bitmap() method compatible python module using:
        python3 ./sprites2bitmap.py ttoasters.bmp 32 32 4 > ttoast_bitmaps.py


## toasters.py

    Flying Toasters

    Uses spritesheet from CircuitPython_Flying_Toasters pendant project
    https://learn.adafruit.com/circuitpython-sprite-animation-pendant-mario-clouds-flying-toasters

    Convert spritesheet bmp to tft.bitmap() method compatible python module using:
        python3 ./sprites2bitmap.py toasters.bmp 64 64 4 > toast_bitmaps.py


## toasters_jpg.py

    An example using a jpg sprite map to draw sprites on T-Display.  This is an older version of the
    toasters.py and tiny_toasters example.  It uses the jpg_decode() method to grab a bitmap of each
    sprite from the toaster.jpg sprite sheet.

    Youtube video: https://youtu.be/0uWsjKQmCpU

    spritesheet from CircuitPython_Flying_Toasters
    https://learn.adafruit.com/circuitpython-sprite-animation-pendant-mario-clouds-flying-toasters


## watch.py

    Analog Watch Display using jpg for the face and filled polygons for the hands
    Requires face_{width}x{height}.jpg in the same directory as this script. See the create_face.py
    script for creating a face image for a given sized display.

    Previous version video: https://youtu.be/NItKb6umMc4
