import sys
from PIL import Image
import PIL

im = Image.open(sys.argv[1])
im = im.transpose(Image.TRANSPOSE)
im = im.convert(mode="RGB")

w, h = im.size

print("w {0}\nh {1}".format(w, h))

pxlmap = im.load()

for y in range(0, h):
    for x in range(0, w):
        px = pxlmap[x, y]
        print("{0} {1} {2} ".format(px[0], px[1], px[2]), end='')
    print('')
