import sys
import math
from PIL import Image
from binary_reader import BinaryReader, BinaryArrayData
from compression import decode_format80
import os

BLOCKS_ROWS = 15
BLOCKS_COLUMNS = 22
BLOCKS_SIZE = 8


class VcnAsset:
    kBlocksPerRow = 32

    def __init__(self):
        self.name = None
        self.blocks = []
        self.blocksPalettePosTable = None
        self.posPaletteTables = None
        self._palette = None

    def make_image(self, palette):
        num_blocks = len(self.blocks)
        width_blocks = 32
        height_blocks = math.ceil(num_blocks / 32)

        img_width = width_blocks * BLOCKS_SIZE
        img_height = height_blocks * BLOCKS_SIZE
        print(f"write image w={img_width} h={img_height}")

        img = Image.new('RGBA', (img_width, img_height))

        for i in range(num_blocks):
            block = self.blocks[i]
            block_x = i % width_blocks
            block_y = i // width_blocks
            blit_block(self, i, img, img_width, block_x * BLOCKS_SIZE,
                       block_y * BLOCKS_SIZE, block, False)

        return img

    def export_image(self, outpath: str):

        bg_img = self.make_image(self._palette)
        bg_img.convert('RGB').save(outpath)

    @staticmethod
    def load(vcn_file: str):
        with BinaryReader(vcn_file) as vcn_reader:
            vcn_data = decode_format80(vcn_reader)

        vcn_reader = BinaryArrayData(vcn_data)

        num_blocks = vcn_reader.read_ushort()

        blocksPalettePosTable = vcn_reader.read_ubyte(num_blocks)
        posPaletteTables = vcn_reader.read_ubyte(8*16)

        palette = vcn_reader.read_ubyte(3*128)

        blocks = []
        for _ in range(num_blocks):
            raw = vcn_reader.read_ubyte(32)
            blocks.append(raw)

        vcn = VcnAsset()
        vcn.blocks = blocks
        vcn.blocksPalettePosTable = blocksPalettePosTable
        vcn.posPaletteTables = posPaletteTables
        vcn._palette = palette

        return vcn


def vga_6_to_8(v):
    return (v * 255) // 63


def blit_block(vcn: VcnAsset, block_id, image, img_width, x, y, vcn_block, flip):
    s = -1 if flip else 1
    p = 7 if flip else 0

    numPalette = vcn.blocksPalettePosTable[block_id]

    for w in range(8):
        for v in range(4):
            word = vcn_block[v + w * 4]
            p0 = (word & 0xf0) >> 4
            p1 = word & 0x0f
            idx0 = vcn.posPaletteTables[numPalette+p0]
            assert (idx0 < 128)
            idx1 = vcn.posPaletteTables[numPalette+p1]
            assert (idx1 < 128)
            coords = x + p + s * 2 * v + (y + w) * img_width

            r = vga_6_to_8(vcn._palette[idx0*3])
            g = vga_6_to_8(vcn._palette[1+idx0*3])
            b = vga_6_to_8(vcn._palette[2+idx0*3])
            image.putpixel((x + p + s * 2 * v, y + w), (r, g, b))

            r = vga_6_to_8(vcn._palette[idx1*3])
            g = vga_6_to_8(vcn._palette[1+idx1*3])
            b = vga_6_to_8(vcn._palette[2+idx1*3])
            image.putpixel((x + p + s+s * 2 * v, y + w), (r, g, b))


def main():
    vcn_path = sys.argv[1]
    print(vcn_path)
    vcn = VcnAsset.load(vcn_path)
    vcn.export_image("out.png")


if __name__ == "__main__":
    main()
