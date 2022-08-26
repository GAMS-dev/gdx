import struct
import yaml

def read_gdx(fn):
    with open(fn, 'rb') as fp:
        header = {}
        def read_byte(): return ord(fp.read(1))
        header['size_of_word'] = read_byte()
        header['byteorder'] = endian = 'little' if hex(int.from_bytes(fp.read(2), byteorder='little')) == '0x1234' else 'big'
        header['sizeof_int'] = intsize = read_byte()
        def read_int(): return int.from_bytes(fp.read(intsize), byteorder=endian)
        header['example_int'] = hex(read_int())
        header['sizeof_double'] = dblsize = read_byte()
        def read_dbl(): return struct.unpack('d', fp.read(dblsize))[0]
        header['example_double'] = read_dbl()
        obj = dict(header=header)
        print(yaml.dump(obj))


if __name__ == '__main__':
    read_gdx('xptest.gdx')
