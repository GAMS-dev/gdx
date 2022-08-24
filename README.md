# gdxnative

Native C++17 re-implementation of (proper) subset of classic expert-level GDX API. Should make "gdxio/gxfile.pas" obsolete and published as open source eventually.

## First steps
- Make writing demand data (parameter "demands") for `gamslib/trnsport` possible through three implementations of GDX-Interface IGDX
  1. just wrap expert level API
  2. slightly extend and use `simplegdx` from @busul
  3. port `gxfile` from Delphi to C++

## Long term
- Not only statically linked but also available as DLL
- Reading older GDX format versions
