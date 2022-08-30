#pragma once

#include "gdxinterface.h"

struct gdxRec;
using gdxHandle_t = struct gdxRec *;

namespace xpwrap {

    class GDXFile : public gdxinterface::GDXInterface {
        gdxHandle_t pgx {};
    public:
        GDXFile();
        ~GDXFile() override;

        int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) override;
        int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) override;
        int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) override;
        int gdxDataWriteStr(const gdxinterface::TgdxStrIndex &KeyStr, const gdxinterface::TgdxValues &Values) override;
        int gdxDataWriteDone() override;

        int gdxClose() override;

        int gdxOpenRead(const std::string &FileName, int &ErrNr) override;

        int gdxFileVersion(std::string &FileStr, std::string &ProduceStr) override;

        int gdxFindSymbol(const std::string &SyId, int &SyNr) override;

        int gdxDataReadStr(gdxinterface::TgdxStrIndex &KeyStr, gdxinterface::TgdxValues &Values, int &DimFrst) override;

        int gdxDataReadDone() override;

        int gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) override;

        int gdxDataReadStrStart(int SyNr, int &NrRecs) override;
    };

}