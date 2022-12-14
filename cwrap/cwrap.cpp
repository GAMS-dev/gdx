// TODO: Optional: Maybe also add variants of some heavily used functions that directly take Delphi short strings

#define GDX_INLINE
#include "cwrap.hpp"

extern "C" {
#ifdef PYGDX_EXPERIMENT
int gdx_set1d(TGXFileRec_t *pgx, const char *name, const char **elems) {
    auto obj = reinterpret_cast<gxfile::TGXFileObj *>(pgx);
    obj->gdxDataWriteStrStart(name, "A 1D set", 1, dt_set, 0);
    TgdxStrIndex keyStrs {};
    TgdxValues values {};
    int i;
    for(i=0; elems[i]; i++) {
        keyStrs[0].assign(elems[i]);
        obj->gdxDataWriteStr(&elems[i], values.data());
    }
    obj->gdxDataWriteDone();
    return i;
}

int create_gdx_file(const char *filename) {
    std::string ErrMsg;
    gxfile::TGXFileObj gdx{ErrMsg};
    if (!ErrMsg.empty()) {
        std::cout << "Unable to create GDX object. Error message:\n" << ErrMsg << '\n';
        return 1;
    }
    int ErrNr;
    if (!gdx.gdxOpenWrite(filename, "cwrap", ErrNr)) {
        std::cout << "Error opening " << filename << " for writing. Error code = " << ErrNr << '\n';
        return 1;
    }
    gdx.gdxDataWriteStrStart("i", "A simple set", 1, dt_set, 0);
    TgdxValues vals{};
    TgdxStrIndex keys{};
    for (int i{1}; i <= 5; i++) {
        keys[0] = "uel_" + std::to_string(i);
        const char *keyptrs[1];
        keyptrs[0] = keys[0].c_str();
        gdx.gdxDataWriteStr(keyptrs, vals.data());
    }
    gdx.gdxDataWriteDone();
    gdx.gdxClose();
    return 0;
}
#endif
}
