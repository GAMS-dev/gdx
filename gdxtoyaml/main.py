import copy
import json
import os
import struct
use_yaml = True
try:
    import yaml
except ImportError as err:
    print(f'Unable to find yaml module: {err.msg}!')
    use_yaml = False
import sys
import platform

sz_integer = 0
sz_byte = 1
sz_word = 2

vm_valund = 0
vm_valna = 1
vm_valpin = 2
vm_valmin = 3
vm_valeps = 4
vm_zero = 5
vm_one = 6
vm_mone = 7
vm_half = 8
vm_two = 9
vm_normal = 10

data_type_sizes = [1, 1, 5, 5, 0]


def get_integer_size(n):
    if n <= 0:
        return sz_integer
    elif n <= 255:
        return sz_byte
    elif n <= 65535:
        return sz_word
    else:
        return sz_integer


def read_gdx(fn):
    with open(fn, 'rb') as fp:
        def read_byte():
            return ord(fp.read(1))

        def read_word():
            return struct.unpack('H', fp.read(2))[0]

        def read_string():
            length = ord(fp.read(1))
            return str(fp.read(length), 'ascii')

        header = dict()
        header['size_of_word'] = read_byte()
        islittle = hex(int.from_bytes(fp.read(2), byteorder='little')) == '0x1234'
        header['byteorder'] = endian = 'little' if islittle else 'big'
        header['sizeof_int'] = intsize = read_byte()

        def read_int():
            return int.from_bytes(fp.read(intsize), byteorder=endian, signed=True)

        header['example_int'] = hex(read_int())
        header['sizeof_double'] = dblsize = read_byte()

        def read_dbl():
            return struct.unpack('d', fp.read(dblsize))[0]

        header['example_double'] = read_dbl()

        gdx_sig = {'gdx_header_nr': read_byte(), 'gdx_header_id': read_string()}
        gdx_sig['version'] = version = read_int()
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

        sym_to_data_offs = {}

        def read_symbol():
            sym = dict(name=read_string(),
                       data_pos=read_int64(),
                       dim=read_int(),
                       type=read_byte(),
                       user_info=read_int(),
                       num_records=read_int(),
                       num_errors=read_int(),
                       has_set=read_byte(),
                       explanatory_text=read_string(),
                       compressed=read_byte())
            sym_to_data_offs[sym['name']] = sym['data_pos']
            sym['domain_controlled'] = dc = read_byte()
            if dc != 0:
                sym['domain_symbols_per_dim'] = [read_int() for _ in range(sym['dim'])]
            if int(version) >= 7:
                sym['num_comments'] = ncomments = read_int()
                if ncomments > 0:
                    sym['comments'] = [read_string() for _ in range(ncomments)]
            return sym

        fp.seek(index_pos['symbols_pos'])
        syms = {'head': read_string()}
        syms['num_symbols'] = nsyms = read_int()
        syms['symbols'] = [read_symbol() for _ in range(nsyms)]
        syms['foot'] = read_string()

        fp.seek(index_pos['set_text_pos'])
        set_texts = dict(head=read_string())
        set_texts['num_set_texts'] = num_set_texts = read_int()
        if num_set_texts > 0:
            set_texts['set_texts'] = [read_string() for _ in range(num_set_texts)]
        set_texts['foot'] = read_string()

        fp.seek(index_pos['uel_pos'])
        uels = dict(head=read_string())
        uels['num_uels'] = num_uels = read_int()
        if num_uels > 0:
            uels['uels'] = [read_string() for _ in range(num_uels)]
        uels['foot'] = read_string()

        fp.seek(index_pos['acronym_pos'])
        acros = dict(head=read_string())
        acros['num_acronyms'] = num_acros = read_int()
        if num_acros > 0:
            acros['acronyms'] = [dict(name=read_string(), text=read_string(), index=read_int())
                                 for _ in range(num_acros)]
        acros['foot'] = read_string()

        fp.seek(index_pos['domain_pos'])
        domains = dict(head=read_string())
        domains['num_domains'] = num_domains = read_int()
        if num_domains > 0:
            domains['domains'] = [read_string() for _ in range(num_domains)]
        domains['midsep'] = read_string()

        def dim_of_sym(index):
            return int(syms['symbols'][index - 1]['dim'])

        domains['controlled_syms'] = []
        while True:
            sym_index = read_int()
            # -1 marks end of controlled symbol list
            if sym_index <= 0:
                break
            csym = dict(sym_index=sym_index, dom_indices=[read_int() for _ in range(dim_of_sym(sym_index))])
            domains['controlled_syms'].append(csym)
        domains['foot'] = read_string()

        data = {}
        for sym_index, (sym_name, data_offs) in enumerate(sym_to_data_offs.items()):
            fp.seek(data_offs)
            data_block = dict(head=read_string())
            data[sym_name] = data_block
            data_block['dim'] = sdim = read_byte()
            data_block['ignored_record_count'] = read_int()
            data_block['min_uel'], data_block['max_uel'], elem_type = [], [], []

            for d in range(sdim):
                min_elem, max_elem = read_int(), read_int()
                data_block['min_uel'].append(min_elem)
                data_block['max_uel'].append(max_elem)
                elem_type.append(get_integer_size(max_elem - min_elem + 1))

            elem_type_to_read_func = {
                sz_integer: read_int,
                sz_byte: read_byte,
                sz_word: read_word
            }

            data_block['records'] = []
            while True:
                b = read_byte()
                record = {}
                if b > sdim:
                    if b == 255: break
                    if sdim > 0:
                        mod_keys = copy.copy(data_block['records'][-1]['keys'])
                        mod_keys[sdim-1] += b-sdim
                        record['keys'] = mod_keys
                else:
                    record['keys'] = [ elem_type_to_read_func[elem_type[d]]() + data_block['min_uel'][d] for d in range(sdim - b + 1) ]

                record['values'] = []
                for dv in range(data_type_sizes[syms['symbols'][sym_index]['type']]):
                    bsv = read_byte()
                    record['values'].append(0 if bsv != vm_normal else read_dbl())

                data_block['records'].append(record)

            data_block['end_of_data_marker'] = b # should be 255

        obj = dict(header=header,
                   gdx_signature=gdx_sig,
                   major_index_positions=index_pos,
                   data_section=data,
                   symbol_section=syms,
                   set_text_section=set_texts,
                   uel_section=uels,
                   acronym_section=acros,
                   domains_section=domains)
        return obj


def to_yaml(obj):
    if use_yaml:
        return yaml.dump(obj, sort_keys=False, default_flow_style=False)
    else:
        return json.dumps(obj, sort_keys=False, indent=4)


def spit(s, fn):
    with open(fn, 'w') as fp:
        fp.write(s)


def convert_file(fn):
    out_fn = fn.replace('.gdx', '.yaml')
    spit(to_yaml(read_gdx(fn)), out_fn)
    return out_fn


if __name__ == '__main__':
    args = sys.argv
    if len(args) == 3:
        diff_path = 'diff' if 'Windows' in platform.system() else '/usr/bin/diff'
        cmd = f'{diff_path} {convert_file(args[1])} {convert_file(args[2])}'
        print(cmd)
        os.system(cmd)
    else:
        print(to_yaml(read_gdx('xptest.gdx' if len(sys.argv) <= 1 else sys.argv[1])))
