from pathlib import Path
from zlib import compress

from PIL import Image


def load(name, size):
    with Image.open(Path("artwork") / f"{name}.png") as source:
        if source.size != size:
            raise ValueError(f"{name} must be {size[0]} by {size[1]}")
        return source.convert("RGBA")


screen = load("blurselect", (320, 240))
screen.alpha_composite(load("No", (160, 120)), (0, 120))
screen.alpha_composite(load("Yes", (160, 120)), (160, 120))
pixels = screen.convert("RGB").load()
framebuffer = bytearray()
for x in range(320):
    for y in range(239, -1, -1):
        r, g, b = pixels[x, y]
        framebuffer.extend((b, g, r))

data = compress(framebuffer, 9)
with Path("blur_menu_art.h").open("w") as output:
    output.write("#pragma once\n#include <cstdint>\n\nconstexpr uint8_t kBlurMenuArt[] = {\n")
    for i in range(0, len(data), 24):
        output.write("    " + ", ".join(map(str, data[i:i + 24])) + ",\n")
    output.write("};\n")
