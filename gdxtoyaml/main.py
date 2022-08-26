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
            return int.from_bytes(fp.read(intsize), byteorder=endian, signed=True)

        header['example_int'] = hex(read_int())
        header['sizeof_double'] = dblsize = read_byte()

        def read_dbl():
            return struct.unpack('d', fp.read(dblsize))[0]

        header['example_double'] = read_dbl()

        gdx_sig = {}
        gdx_sig['gdx_header_nr'] = read_byte()
        gdx_sig['gdx_header_id'] = read_string()
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
                       compressed=read_byte(),
                       )
            sym['domain_controlled'] = dc = read_byte()
            if dc != 0:
                sym['domain_symbols_per_dim'] = [read_int() for i in range(sym['dim'])]
            if int(version) >= 7:
                sym['num_comments'] = ncomments = read_int()
                if ncomments > 0:
                    sym['comments'] = [read_string() for _ in range(ncomments)]
            return sym

        fp.seek(index_pos['symbols_pos'])
        syms = {}
        syms['head'] = read_string()
        syms['num_symbols'] = nsyms = read_int()
        syms['symbols'] = [read_symbol() for i in range(nsyms)]
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
            return int(syms['symbols'][index]['dim'])

        def read_controlled_sym():
            sym_index = read_int()
            return dict(sym_index=sym_index,
                        dom_indices=[read_int() for _ in range(dim_of_sym(sym_index))])

        if num_domains > 0:
            domains['controlled_syms'] = { read_controlled_sym() for _ in range(num_domains) }

        domains['check_minus_one'] = read_int()
        domains['foot'] = read_string()

        obj = dict(header=header,
                   gdx_signature=gdx_sig,
                   major_index_positions=index_pos,
                   data_section={},
                   symbol_section=syms,
                   set_text_section=set_texts,
                   uel_section=uels,
                   acronym_section=acros,
                   domains_section=domains)
        print(yaml.dump(obj, sort_keys=False, default_flow_style=False))


if __name__ == '__main__':
    read_gdx('xptest.gdx')
