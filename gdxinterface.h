#pragma once

#include <string>
#include <array>
#include "gxdefs.h"

namespace gdxinterface {
    /*const int MaxDim = 20;
    using TgdxValues = std::array<double, 5>;
    using TgdxStrIndex = std::array<std::string, MaxDim>;*/

    class GDXInterface {
    public:
        virtual ~GDXInterface() = default;

        virtual int gdxFileVersion(std::string &FileStr, std::string &ProduceStr) = 0;

        virtual int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) = 0;
        virtual int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) = 0;

        virtual int gdxOpenRead(const std::string &FileName, int &ErrNr) = 0;

        virtual int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) = 0;
        virtual int gdxDataWriteStr(const gxdefs::TgdxStrIndex &KeyStr, const gxdefs::TgdxValues &Values) = 0;
        virtual int gdxDataWriteDone() = 0;

        virtual int gdxDataReadStrStart(int SyNr, int &NrRecs) = 0;
        virtual int gdxDataReadStr(gxdefs::TgdxStrIndex &KeyStr, gxdefs::TgdxValues &Values, int &DimFrst) = 0;
        virtual int gdxDataReadDone() = 0;

        virtual int gdxFindSymbol(const std::string &SyId, int &SyNr) = 0;
        virtual int gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) = 0;

        virtual int gdxClose() = 0;

        virtual int gdxAddAlias(const std::string &Id1, const std::string &Id2) = 0;
        virtual int gdxAddSetText(const std::string &Txt, int &TxtNr) = 0;

        virtual int gdxDataErrorCount() = 0;

        virtual int gdxDataErrorRecord(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values) = 0;
        virtual int gdxDataErrorRecordX(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values) = 0;
    };

}