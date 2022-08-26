# GDX to YAML script

A small standalone Python script that reads a GDX file and dumps its low-level detailed contents as YAML. Unlike `gdxdump` this is not meant for users but instead as a convenience utility to look at or compare stored GDX files written by the new work-in-progress C++ port of the old `gxfile` Delphi-class. It as a more convenient way to look at raw GDX data than doing a plain `hexdump`.

Example output for example input file `xptest.gdx`:
```
header:
  size_of_word: 2
  byteorder: little
  sizeof_int: 4
  example_int: '0x12345678'
  sizeof_double: 8
  example_double: 3.141592653589793
gdx_signature:
  gdx_header_nr: 123
  gdx_header_id: GAMSGDX
  version: 7
  compressed: 0
  audit_line: GDX Library      41.0.0 4a298526 Aug 16,...
  producer: xptests
major_index_positions:
  check_pos: 19510624
  symbols_pos: 253
  uel_pos: 341
  set_text_pos: 322
  acronym_pos: 381
  next_write_pos: 253
  domain_pos: 399
data_section:
  Demand:
    head: _DATA_
    dim: 1
    num_records: 3
    min_uel: 1
    max_uel: 3
    records:
    - key: 1
      key2: 0
      value_type: 10
      value: 324.0
    - key: 2
      value_type: 10
      value: 299.0
    - key: 2
      value_type: 10
      value: 274.0
    end_of_data_marker: 255
symbol_section:
  head: _SYMB_
  num_symbols: 1
  symbols:
  - name: Demand
    data_pos: 201
    dim: 1
    type: 1
    user_info: 0
    num_records: 3
    num_errors: 0
    has_set: 0
    explanatory_text: Demand data
    compressed: 0
    domain_controlled: 0
    num_comments: 0
  foot: _SYMB_
set_text_section:
  head: _SETT_
  num_set_texts: 1
  set_texts:
  - ''
  foot: _SETT_
uel_section:
  head: _UEL_
  num_uels: 3
  uels:
  - New-York
  - Chicago
  - Topeka
  foot: _UEL_
acronym_section:
  head: _ACRO_
  num_acronyms: 0
  foot: _ACRO_
domains_section:
  head: _DOMS_
  num_domains: 0
  midsep: _DOMS_
  check_minus_one: -1
  foot: _DOMS_
```