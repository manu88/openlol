# from https://github.com/vardis/eye-of-the-beholder-2/blob/master/binary_reader.py
import array
import struct


class BinaryReader:
    """

    """

    def __init__(self, filename):

        self.filename = filename
        self.handle = None

    def __enter__(self):
        """

        :return:
        """
        self.handle = open(self.filename, 'rb')

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """

        :param exc_type:
        :param exc_val:
        :param exc_tb:
        :return:
        """
        if self.handle:
            self.handle.close()
            self.handle = None

    @property
    def offset(self):
        """

        :return:
        """
        if not self.handle:
            return None

        return self.handle.tell()

    @property
    def offset_hex(self):
        """

        :return:
        """
        return hex(self.offset)

    def read_ubyte(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}B'.format(
            count=count), self.handle.read(count))

        if count == 1:
            return val[0]
        return val

    def read_byte(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}b'.format(
            count=count), self.handle.read(count))
        if count == 1:
            return val[0]
        return val

    def peek_ubyte(self, count=1):
        """

        :param count:
        :return:
        """
        s = struct.unpack('{count}B'.format(
            count=count), self.handle.read(count))
        self.handle.seek(-count, 1)
        if count == 1:
            return s[0]

        return list(s[:count])

    def peek_byte(self, count=1):
        """

        :param count:
        :return:
        """
        value = struct.unpack('{count}b'.format(
            count=count), self.handle.read(count))[0]
        self.handle.seek(-count, 1)
        return value

    def read_ushort(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}H'.format(
            count=count), self.handle.read(count * 2))
        if count == 1:
            return val[0]
        return val

    def read_short(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}h'.format(
            count=count), self.handle.read(count * 2))
        if count == 1:
            return val[0]
        return val

    def peek_ushort(self, count=1):
        """

        :param count:
        :return:
        """
        value = struct.unpack('{count}H'.format(
            count=count), self.handle.read(count * 2))[0]
        self.handle.seek(-count * 2, 1)
        return value

    def peek_short(self, count=1):
        """

        :param count:
        :return:
        """
        value = struct.unpack('{count}h'.format(
            count=count), self.handle.read(count * 2))[0]
        self.handle.seek(-count * 2, 1)
        return value

    def read_uint(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}I'.format(
            count=count), self.handle.read(count * 4))
        if count == 1:
            return val[0]
        return val

    def read_int(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}i'.format(count=count),
                            self.handle.read(count * 4))[0]
        if count == 1:
            return val[0]
        return val

    def read_string(self, length=0):
        """

        :param length:
        :return:
        """

        buff = struct.unpack('{length}s'.format(
            length=length), self.handle.read(length))[0]

        string = ''
        for c in buff:
            if c == 0:
                break
            string += chr(c)

        return string

    def peek_string(self, length=0):
        str = self.read_string(length)
        self.rewind(length)
        return str

    def search_string(self):
        """

        :return:
        """

        string = ''

        while True:
            b = self.read_ubyte()

            if b == 0x0:
                return string

            string += chr(b)

    def rewind(self, offset):
        """
        Move offset backward
        :param offset:
        :return:
        """

        self.handle.seek(-offset, 1)

    def seek(self, offset):
        """
        Set offset
        :param offset:
        :return:
        """

        self.handle.seek(offset)


class BinaryArrayData:
    """

    """

    def __init__(self, array_data):
        self.data = array.array('B')
        self.data.fromlist(array_data)
        self.pos = 0
        self.limit = len(self.data)

    @property
    def offset(self):
        """

        :return:
        """
        return self.pos

    def read_ubyte(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}B'.format(count=count),
                            self.data[self.pos:self.pos + count])
        self.pos += count

        if count == 1:
            return val[0]
        return val

    def read_byte(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}b'.format(count=count),
                            self.data[self.pos:self.pos + count])
        self.pos += count

        if count == 1:
            return val[0]
        return val

    def peek_ubyte(self, count=1):
        """

        :param count:
        :return:
        """
        val = self.read_ubyte(count)
        self.pos -= count
        return val

    def peek_byte(self, count=1):
        """

        :param count:
        :return:
        """
        value = self.read_byte(count)
        self.pos -= count
        return value

    def read_ushort(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}H'.format(count=count),
                            self.data[self.pos:self.pos + count * 2])
        self.pos += 2 * count

        if count == 1:
            return val[0]
        return val

    def read_short(self, count=1):
        """

        :param count:
        :return:
        """
        val = struct.unpack('{count}h'.format(count=count),
                            self.data[self.pos:self.pos + count * 2])
        self.pos += 2 * count

        if count == 1:
            return val[0]
        return val

    def peek_ushort(self, count=1):
        """

        :param count:
        :return:
        """
        value = self.read_ushort(count)
        self.pos -= 2 * count

        return value

    def peek_short(self, count=1):
        """

        :param count:
        :return:
        """
        value = self.read_short(count)
        self.pos -= 2 * count
        return value

    def read_uint(self, count=1):
        """

        :param count:
        :return:
        """
        value = struct.unpack('{count}I'.format(
            count=count), self.data[self.pos:self.pos + count * 4])
        self.pos += 4 * count
        return value[0] if count == 1 else value

    def read_int(self, count=1):
        """

        :param count:
        :return:
        """
        value = struct.unpack('{count}i'.format(
            count=count), self.data[self.pos:self.pos + count * 4])
        self.pos += 4 * count
        return value[0] if count == 1 else value

    def read_string(self, length=0):
        """

        :param length:
        :return:
        """

        buff = struct.unpack('{length}s'.format(
            length=length), self.data[self.pos:self.pos + length])
        self.pos += length
        string = ''
        for c in buff:
            if c == 0:
                break
            string += chr(c)

        return string

    def peek_string(self, length=0):
        str = self.read_string(length)
        self.pos -= length
        return str

    def search_string(self):
        """

        :return:
        """

        string = ''

        while True:
            b = self.read_ubyte()

            if b == 0x0:
                return string

            string += chr(b)

    def rewind(self, offset):
        """
        Move offset backward
        :param offset:
        :return:
        """
        self.pos -= offset

    def seek(self, offset):
        """
        Set offset
        :param offset:
        :return:
        """
        self.pos = offset
