#include "cwrap.h"
#include "../gxfile.h"
#include <iostream>

using namespace gxfile;

extern "C" {

void *gdx_create(char *errBuf, int bufSize) {
    printf("create called\n");
    std::string ErrMsg;
    auto *pgx = new TGXFileObj {ErrMsg};
    if(!ErrMsg.empty())
        memcpy(errBuf, ErrMsg.c_str(), std::min<int>((int)ErrMsg.length()+1, bufSize));
    else
        errBuf[0] = '\0';
    return pgx;
}

void gdx_destroy(void **pgx) {
    printf("destroy called\n");
    delete (TGXFileObj *)*pgx;
    *pgx = nullptr;
}

int gdx_open_write(void *pgx, const char *filename, int *ec) {
    return static_cast<TGXFileObj *>(pgx)->gdxOpenWrite(filename, "gdxnative", *ec);
}

int gdx_open_read(void *pgx, const char *filename, int *ec) {
    return static_cast<TGXFileObj *>(pgx)->gdxOpenRead(filename, *ec);
}

void gdx_close(void *pgx) {
    static_cast<TGXFileObj *>(pgx)->gdxClose();
}

int create_gdx_file(const char *filename) {
    std::string ErrMsg;
    gxfile::TGXFileObj gdx{ErrMsg};
    if (!ErrMsg.empty()) {
        std::cout << "Unable to create GDX object. Error message:\n" << ErrMsg << std::endl;
        return 1;
    }
    int ErrNr;
    if (!gdx.gdxOpenWrite(filename, "cwrap", ErrNr)) {
        std::cout << "Error opening " << filename << " for writing. Error code = " << ErrNr << std::endl;
        return 1;
    }
    gdx.gdxDataWriteStrStart("i", "A simple set", 1, global::gmsspecs::dt_set, 0);
    gxdefs::TgdxValues vals{};
    gxdefs::TgdxStrIndex keys{};
    for (int i{1}; i <= 5; i++) {
        keys[0] = "uel_" + std::to_string(i);
        gdx.gdxDataWriteStr(keys, vals);
    }
    gdx.gdxDataWriteDone();
    gdx.gdxClose();
    return 0;
}

}