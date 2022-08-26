import struct
import yaml


def read_gdx(fn):
    with open(fn, 'rb') as fp:
        def read_byte():
            return ord(fp.read(1))

        def read_string():
            len = ord(fp.read(1))
            return str(fp.read(len), 'ascii')

        header = dict()
        header['size_of_word'] = read_byte()
        islittle = hex(int.from_bytes(fp.read(2), byteorder='little')) == '0x1234'
        header['byteorder'] = endian = 'little' if islittle else 'big'
        header['sizeof_int'] = intsize = read_byte()

        def read_int():
            return int.from_bytes(fp.read(intsize), byteorder=endian)

        header['example_int'] = hex(read_int())
        header['sizeof_double'] = dblsize = read_byte()

        def read_dbl():
            return struct.unpack('d', fp.read(dblsize))[0]

        header['example_double'] = read_dbl()

        gdx_sig = {}
        gdx_sig['gdx_header_nr'] = read_byte()
        gdx_sig['gdx_header_id'] = read_string()
        gdx_sig['version'] = read_int()
        gdx_sig['compressed'] = read_int()

        def trunc(s, maxlen):
            return s if len(s) <= maxlen else s[:maxlen] + '...'

        gdx_sig['audit_line'] = trunc(read_string(), 40)
        gdx_sig['producer'] = read_string()

        index_pos = {}

        def read_int64():
            return int.from_bytes(fp.read(8), byteorder=endian)

        index_pos['check_pos'] = read_int()  # should be 19510624
        pos_names = ['symbols', 'uel', 'set_text', 'acronym', 'next_write', 'domain']
        for pn in pos_names:
            index_pos[pn + '_pos'] = read_int64()

        obj = dict(header=header, gdx_signature=gdx_sig, major_index_positions=index_pos)
        print(yaml.dump(obj))


if __name__ == '__main__':
    read_gdx('xptest.gdx')
