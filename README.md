# gdxnative

Native C++17 re-implementation of (proper) subset of classic expert-level GDX API. Should make "gdxio/gxfile.pas" obsolete and published as open source eventually.

## First steps
- [ ] CMake project with unit tests and informative README
- [ ] Getting `xp1example.c` and `xp2example.c` from the C-API running with drop-in replacement header

Writing demand data in `xp1example.c`
- [ ] `gdxOpenWrite`
- [ ] `gdxDataWriteStrStart`
- [ ] `gdxDataWriteStr`
- [ ] `gdxDataWriteDone`

## Long term
- [ ] Not only statically linked but also available as DLL
- [ ] Reading older GDX format versions
