#include "gxfile.h"

namespace gxfile {


    int TGXFileObj::gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) {
        return 0;
    }

    int TGXFileObj::gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) {
        return 0;
    }

    int TGXFileObj::gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Typ, int UserInfo) {
        return 0;
    }

    int TGXFileObj::gdxDataWriteStr(const TgdxStrIndex &KeyStr, const TgdxValues &Values) {
        return 0;
    }

    int TGXFileObj::gdxDataWriteDone() {
        return 0;
    }
}