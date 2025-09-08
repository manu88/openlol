# from https://github.com/vardis/eye-of-the-beholder-2/blob/master/compression.py

def decode_format80_image(reader):
    size = reader.read_ushort()
    format_type = reader.read_ushort()
    uncompressed = reader.read_uint()
    palette_size = reader.read_ushort()

    if format_type != 0x4:
        raise TypeError("Expected format type 4 but got type %d" % format_type)

    # (uncompressed + palette_size + 1)
    data = [0] * (uncompressed + palette_size + 1)
    data[0] = palette_size

    if palette_size > 0:
        data[1:palette_size + 1] = reader.read_ubyte(palette_size)

    img_data = _decode_data(reader, data[palette_size + 1:])
    data[palette_size + 1:] = img_data

    return data


def decode_format80(reader):
    """
    Output format:
    <nr. of palette colors>, [<red channel for entry1, blue channel for entry 1, green channel for entry 1, ...>]*, <palette index>+

    If there's no inline palette definition then the number of palette colors is zero
    and the palette indices follow immediately.

    :param reader:
    :return:
    """
    size = reader.read_ushort()
    format_type = reader.read_ushort()
    uncompressed = reader.read_uint()
    palette_size = reader.read_ushort()

    if format_type != 0x4:
        raise TypeError("Expected format type 4 but got type %d" % format_type)

    data = [0] * uncompressed
    _decode_data(reader, data)

    return data


def _decode_data(reader, dst_data):
    dst_offset = 0

    while True:
        code = reader.read_ubyte()

        # end code
        if code == 0x80:
            return dst_data

        # 0b10######
        elif code & 0xC0 == 0x80:
            count = code & 0x3F

            if count == 0:
                return dst_data

            for i in range(count):
                dst_data[dst_offset] = reader.read_ubyte()
                dst_offset += 1

        # 0b11######
        elif code & 0xC0 == 0xC0:
            count = code & 0x3F

            # Large copy
            if count < 0x3E:
                # command 2 (11cccccc p p): copy
                count += 3
                offset = reader.read_ushort()
                for i in range(offset, offset + count):
                    dst_data[dst_offset] = dst_data[i]
                    dst_offset += 1

            # Very large copy
            elif count == 0x3E:
                # pass
                # command 3 (11111110 c c v): fill
                count = reader.read_ushort()
                value = reader.read_ubyte()
                for i in range(count):
                    dst_data[dst_offset] = value
                    dst_offset += 1
            else:
                # command 4 (copy 11111111 c c p p): copy
                count = reader.read_ushort()
                offset = reader.read_ushort()
                for i in range(offset, offset + count):
                    dst_data[dst_offset] = dst_data[i]
                    dst_offset += 1

        # 0b0#######
        # command 0 (0cccpppp p): copy
        elif code & 0x80 == 0:
            count = ((code & 0x70) >> 4) + 3
            offset = reader.read_ubyte() + ((code & 0x0F) << 8)
            offset = dst_offset - offset
            for i in range(offset, offset + count):
                dst_data[dst_offset] = dst_data[i]
                dst_offset += 1
