#include "cwrap.h"
#include "../gxfile.h"
#include <iostream>

extern "C" int create_gdx_file(const char *filename) {
    std::string ErrMsg;
    gxfile::TGXFileObj gdx {ErrMsg};
    if(!ErrMsg.empty()) {
        std::cout << "Unable to create GDX object. Error message:\n" << ErrMsg << std::endl;
        return 1;
    }
    int ErrNr;
    if(!gdx.gdxOpenWrite(filename, "cwrap", ErrNr)) {
        std::cout << "Error opening " << filename << " for writing. Error code = " << ErrNr << std::endl;
        return 1;
    }
    gdx.gdxDataWriteStrStart("i", "A simple set", 1, global::gmsspecs::dt_set, 0);
    gxdefs::TgdxValues vals {};
    gxdefs::TgdxStrIndex keys {};
    for(int i{1}; i<=5; i++) {
        keys[0] = "uel_" + std::to_string(i);
        gdx.gdxDataWriteStr(keys, vals);
    }
    gdx.gdxDataWriteDone();
    gdx.gdxClose();
    return 0;
}
