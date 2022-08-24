#pragma once

#include "igdx.h"

struct gdxRec;
using gdxHandle_t = struct gdxRec *;

namespace xpwrap {

    class GDXFile : public igdx::IGDX {
        gdxHandle_t pgx {};
    public:
        GDXFile();
        ~GDXFile() override;

        int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) override;
        int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) override;
        int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) override;
        int gdxDataWriteStr(const igdx::TgdxStrIndex &KeyStr, const igdx::TgdxValues &Values) override;
        int gdxDataWriteDone() override;

        int gdxClose() override;
    };

}