"""
bitarray.py

    An example using map_bitarray_to_rgb565 to draw sprites

"""

import time
import random
import s3lcd
import tft_config


tft = tft_config.config(tft_config.WIDE)


SPRITE_WIDTH = 16
SPRITE_HEIGHT = 16
SPRITE_STEPS = 3
SPRITE_BITMAPS = [
    bytearray(
        [
            0b00000000, 0b00000000,
            0b00000001, 0b11110000,
            0b00000111, 0b11110000,
            0b00001111, 0b11100000,
            0b00001111, 0b11000000,
            0b00011111, 0b10000000,
            0b00011111, 0b00000000,
            0b00011110, 0b00000000,
            0b00011111, 0b00000000,
            0b00011111, 0b10000000,
            0b00001111, 0b11000000,
            0b00001111, 0b11100000,
            0b00000111, 0b11110000,
            0b00000001, 0b11110000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
        ]
    ),
    bytearray(
        [
            0b00000000, 0b00000000,
            0b00000011, 0b11100000,
            0b00001111, 0b11111000,
            0b00011111, 0b11111100,
            0b00011111, 0b11111100,
            0b00111111, 0b11110000,
            0b00111111, 0b10000000,
            0b00111100, 0b00000000,
            0b00111111, 0b10000000,
            0b00111111, 0b11110000,
            0b00011111, 0b11111100,
            0b00011111, 0b11111100,
            0b00001111, 0b11111000,
            0b00000011, 0b11100000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
        ]
    ),
    bytearray(
        [
            0b00000000, 0b00000000,
            0b00000111, 0b11000000,
            0b00011111, 0b11110000,
            0b00111111, 0b11111000,
            0b00111111, 0b11111000,
            0b01111111, 0b11111100,
            0b01111111, 0b11111100,
            0b01111111, 0b11111100,
            0b01111111, 0b11111100,
            0b01111111, 0b11111100,
            0b00111111, 0b11111000,
            0b00111111, 0b11111000,
            0b00011111, 0b11110000,
            0b00000111, 0b11000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
        ]
    ),
    bytearray(
        [
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
        ]
    ),
]


def collide(sprite_a, sprite_b):
    """return true if two sprites overlap"""
    return (
        sprite_a.x <= sprite_b.x + sprite_b.width
        and sprite_a.x + sprite_a.width >= sprite_b.x
        and sprite_a.y <= sprite_b.y + sprite_b.height
        and sprite_a.y + sprite_a.height >= sprite_b.y
    )


class pacman:
    """
    pacman class to keep track of a sprites locaton and step
    """

    def __init__(self, sprites, width, height, steps):
        """create a new pacman sprite that does not overlap another sprite"""
        self.steps = steps
        self.step = random.randint(0, self.steps)
        self.width = width
        self.height = height

        # Big problem if there are too many sprites for the size of the display
        while True:
            self.x = random.randint(0, tft.width() - width)
            self.y = random.randint(0, tft.height() - height)
            # if this sprite does not overlap another sprite then break
            if not any(collide(self, sprite) for sprite in sprites):
                break

    def move(self):
        """move the sprite one step"""
        max_x = tft.width() - SPRITE_WIDTH
        self.step += 1
        self.step %= SPRITE_STEPS
        self.x += 1
        if self.x == max_x - 1:
            self.step = SPRITE_STEPS

        self.x %= max_x

    def draw(self, blitable):
        tft.blit_buffer(blitable, self.x, self.y, self.width, self.height)


def main():
    """
    Draw on screen using map_bitarray_to_rgb565
    """

    try:
        # enable display and clear screen
        tft.init()
        tft.fill(s3lcd.BLACK)

        # convert bitmaps into rgb565 blitable buffers
        blitable = []
        for sprite_bitmap in SPRITE_BITMAPS:
            sprite = bytearray(512)
            tft.map_bitarray_to_rgb565(
                sprite_bitmap, sprite, SPRITE_WIDTH, s3lcd.YELLOW, s3lcd.BLACK
            )
            blitable.append(sprite)

        sprite_count = tft.width() // SPRITE_WIDTH * tft.height() // SPRITE_HEIGHT // 4

        # create pacman spites in random positions
        sprites = []
        for _ in range(sprite_count):
            sprites.append(pacman(sprites, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_STEPS))

        # move and draw sprites
        while True:
            for sprite in sprites:
                sprite.move()
                sprite.draw(blitable[sprite.step])

            tft.show()
            time.sleep(0.05)

    finally:
        tft.deinit()


main()
