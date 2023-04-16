#!/bin/sh

for size in 64x64 80x160 128x128 160x128 160x80 240x135 240x240 240x320 320x170 320x240 480x320;  do
	convert Micropython-logo.svg -resize "$size" -gravity center -extent "$size" +repage -background white "logo-$size.jpg"
done
