#pragma once

#include "igdx.h"

namespace gxfile {
    class TGXFileObj : public igdx::IGDX {
        //gdlib::gmsstrm::TMiBufferedStream FFile;
    public:
        int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) override;
        int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) override;
        int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Typ, int UserInfo) override;
        int gdxDataWriteStr(const igdx::TgdxStrIndex &KeyStr, const igdx::TgdxValues &Values) override;
        int gdxDataWriteDone() override;
    };

}
