#!/bin/sh

function menu() {
    echo -ne "
    Which device are you using:
        1) T-Dongle-S3
        2) T-Display-S3
        3) T-Embed
        4) T-HMI
        5) ESP32-S3-BOX or ESP32-S3-BOX-LITE
        6) WT32-SC01 Plus
        7) M5STACK ATOM-S3
        8) M5STACK CORES3
        0) Quit

    Choose an option (0-6): "
    read a
    case $a in


        1)  #   T-Dongle-S3

            FIRMWARE="S3LCD_16M"
            CONFIG="t-dongle-s3"
            WIDE_SCREEN="160x80"
            TALL_SCREEN="80x160"
            CLOCK_FONT="45"
            PROVERBS_FONT="20"
            ;;

        2)  #   T-Display-S3

            FIRMWARE="S3LCD_OCT_16M"
            CONFIG="t-display-s3"
            WIDE_SCREEN="320x170"
            TARL_SCREEN="170x320"
            CLOCK_FONT="100"
            PROVERBS_FONT="45"
            ;;

        3)  #   T-Embed

            FIRMWARE="S3LCD_OCT_16M"
            CONFIG="t-embed"
            WIDE_SCREEN="320x170"
            TALL_SCREEN="170x320"
            CLOCK_FONT="100"
            PROVERBS_FONT="45"
            ;;

        4)  #   T-HMI

            FIRMWARE="S3LCD_OCT_16M"
            CONFIG="t-hmi"
            WIDE_SCREEN="320x240"
            TALL_SCREEN="240x320"
            CLOCK_FONT="100"
            PROVERBS_FONT="45"
            ;;

        5)  #   ESP32-S3-BOX or ESP32-S3-BOX-LITE

            FIRMWARE="S3LCD_OCT_16M"
            CONFIG="esp32_s3_box"
            WIDE_SCREEN="320x240"
            TARL_SCREEN="240x320"
            CLOCK_FONT="80"
            PROVERBS_FONT="45"
            ;;

        6)  #   WT32-SC01 Plus

            FIRMWARE="S3LCD_QUAD_16M"
            CONFIG="wt32-sc01-plus"
            WIDE_SCREEN="480x320"
            TALL_SCREEN="320x480"
            CLOCK_FONT="150"
            PROVERBS_FONT="45"
            ;;

        7)  #   M5STACK ATOM-S3

            FIRMWARE="S3LCD_8M"
            CONFIG="m5stack-atom-s3"
            WIDE_SCREEN="128x128"
            TALL_SCREEN="128x128"
            CLOCK_FONT="45"
            PROVERBS_FONT="20"
            ;;

        8)  #   M5STACK CORES3

            FIRMWARE="S3LCD_QUAD_16M"
            CONFIG="m5stack-cores3"
            WIDE_SCREEN="320x240"
            TALL_SCREEN="240x320"
            CLOCK_FONT="100"
            PROVERBS_FONT="45"
            ;;

        0)  exit 0
            ;;

        *) echo -e "Invalid option."
            menu
    esac
}

menu

echo
echo -n "Erase and flash firmware (y/N): "
read -e flash

if [ "${flash:0:1}" = "Y" ] || [ "${flash:0:1}" = "y" ]; then
    echo -e "Erasing and flashing firmware..."
    esptool.py --chip esp32s3 --port /dev/ttyACM0 erase_flash
    esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash -z 0 ../firmware/${FIRMWARE}/firmware.bin
    read -p "Flashing done. Reset device then press any key to continue"
fi

echo "Uploading examples..."

mpremote cp configs/${CONFIG}/*.py :

mpremote cp bitarray.py :
mpremote cp bitmap_fonts.py :
mpremote cp blit_tests/logo-64x64.jpg :
mpremote cp blit_tests/blit_bounce.py :

mpy-cross chango/chango_16.py
mpremote cp chango/chango_16.mpy :
mpy-cross chango/chango_32.py
mpremote cp chango/chango_32.mpy :
mpy-cross chango/chango_64.py
mpremote cp chango/chango_64.mpy :
mpremote cp chango/chango.py :

mpremote cp color_test.py :
mpremote cp feathers.py :
mpremote cp font_decode.py :
mpremote cp hello.py :
mpremote cp hershey.py :

mpremote cp jpg_tests/jpg_bounce.py :
mpremote cp jpg_tests/jpg_logo.py :
mpremote cp jpg_tests/jpg_tests.py :
mpremote cp jpg_tests/logo-64x64.jpg :
mpremote cp jpg_tests/logo-${WIDE_SCREEN}.jpg :

mpy-cross mono_fonts/inconsolata_16.py
mpremote cp mono_fonts/inconsolata_16.mpy :
mpy-cross mono_fonts/inconsolata_32.py
mpremote cp mono_fonts/inconsolata_32.mpy :
mpy-cross mono_fonts/inconsolata_64.py
mpremote cp mono_fonts/inconsolata_64.mpy :
mpremote cp mono_fonts/mono_fonts.py :

cd nasa
mpremote cp nasa_clock.py :
mpremote cp nasa_images.py :
mpy-cross pacifico${CLOCK_FONT}.py
mpremote cp pacifico${CLOCK_FONT}.mpy :pacifico.mpy

mpremote cp -r nasa_${WIDE_SCREEN} :
cd ..

mpremote cp noto_fonts.py :
mpremote cp pinball.py :
mpremote cp png_tests/alien.png :
mpremote cp png_tests/alien.py :
mpremote cp png_tests/png_bounce.py :

cd proverbs
mpy-cross notosanssc${PROVERBS_FONT}.py
mpremote cp notosanssc${PROVERBS_FONT}.mpy :notosanssc.mpy
mpremote cp proverbs.py :
cd ..

mpremote cp roids.py :
mpremote cp scroll.py :
mpremote cp tiny_hello.py :

mpremote cp tiny_toasters/tiny_toasters.py :
mpy-cross tiny_toasters/ttoast_bitmaps.py
mpremote cp tiny_toasters/ttoast_bitmaps.mpy :

mpremote cp toasters_jpg/toaster.jpg :
mpremote cp toasters_jpg/toasters_jpg.py :

mpy-cross toasters/toast_bitmaps.py
mpremote cp toasters/toast_bitmaps.mpy :
mpremote cp toasters/toasters.py :

mpremote cp watch/face_${WIDE_SCREEN}.jpg :
mpremote cp watch/watch.py :
