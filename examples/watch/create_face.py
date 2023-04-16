#!/usr/bin/env python3

"""
    Create a watch face_{width}x{height}.jpg file for a given width and height.
"""

import argparse
import math
from PIL import Image, ImageDraw, ImageFont

# get the width and height from the command line using argparse

parser = argparse.ArgumentParser(prog='create_face.py',
    description=("Create a watch face_{width}x{height}.jpg file for a given width and height.")
)
parser.add_argument("width", type=int, help="width of the display")
parser.add_argument("height", type=int, help="height of the display")
args = parser.parse_args()
width = args.width  # width of the display
height = args.height # height of the display

face = min(width, height)       # face is the smaller of the two
ofs_x = (width - face) // 2     # offset from the left side of the display
ofs_y = (height - face) // 2    # offset from the top of the display
font_size = 10 if height < 100 else 18

# create an image
out = Image.new("RGB", (width, height), (255, 255, 255))

fnt = ImageFont.truetype("./LibreBaskerville-Regular.ttf", font_size)  # get a font of an appropriate size
d = ImageDraw.Draw(out)          # get a drawing context
radius = int(face // 2  * 0.8)   # radius of the clock face
cx = int(face // 2)              # center x of the clock face

second = 0
for minute in range(1, 60):
    # get the angle of the minute hand
    angle = (minute*math.pi/30)+(second*math.pi/1800)
    cos_a = math.cos(angle)
    sin_a = math.sin(angle)

    # x and y coordinates of the outer minute tick
    y1 = -cx * cos_a * 0.76
    x1 = cx * sin_a * 0.76

    # x and y coordinates of the inner minute tick
    y2 = -cx * cos_a * 0.7
    x2 = cx * sin_a * 0.7

    # draw the minute tick
    d.line([ofs_x+x1+cx, ofs_y+y1+cx, ofs_x+x2+cx, ofs_y+y2+cx], width=1, fill="#000000")

for hour in range(1, 13):
    # get the angle of the hour hand
    angle = hour*math.pi/6
    cos_a = math.cos(angle)
    sin_a = math.sin(angle)

    # x and y coordinates of the outer hour tick
    y1 = -cx * cos_a * 0.76
    x1 = cx * sin_a * 0.76

    # x and y coordinates of the inner hour tick
    y2 = -cx * cos_a * 0.7
    x2 = cx * sin_a * 0.7

    # draw the hour tick
    d.line([ofs_x+x1+cx, ofs_y+y1+cx, ofs_x+x2+cx, ofs_y+y2+cx], width=5, fill="#ff0000")

    # x and y coordinates of the hour number
    y = -cx * cos_a * 0.9
    x = cx * sin_a * 0.9

    # draw the hour number in our previously selected font
    size = d.textbbox((0, 0), str(hour), font=fnt)
    d.text(
        (ofs_x+x+cx-((size[2]+size[0] >> 1)), ofs_y+y+cx-((size[3]+size[1]) >> 1)),
        str(hour),
        font=fnt,
        fill=(0, 0, 0),
        align="center")

# save the face as a jpeg file
out.save(f'face_{width}x{height}.jpg', "JPEG", quality=100, optimize=True, progressive=False)
# out.show()
