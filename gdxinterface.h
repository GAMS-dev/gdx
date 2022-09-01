#pragma once

#include <string>
#include <array>
#include "gxdefs.h"

namespace gdxinterface {
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

        virtual int gdxDataReadRaw(gxdefs::TgdxUELIndex &KeyInt, gxdefs::TgdxValues &Values, int &DimFrst) = 0;
        virtual int gdxDataReadRawStart(int SyNr, int &NrRecs) = 0;

        virtual int gdxDataWriteRaw(const gxdefs::TgdxUELIndex &KeyInt, const gxdefs::TgdxValues &Values) = 0;
        virtual int gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ, int UserInfo) = 0;

        virtual int gdxErrorCount() = 0;
        virtual int gdxErrorStr(int ErrNr, std::string &ErrMsg) = 0;

        virtual int gdxGetElemText(int TxtNr, std::string &Txt, int &Node) = 0;
        virtual int gdxGetLastError() = 0;

        virtual int gdxGetSpecialValues(gxdefs::TgdxSVals &Avals) = 0;
        virtual int gdxSetSpecialValues(const gxdefs::TgdxSVals &AVals) = 0;

        virtual int gdxSymbolGetDomain(int SyNr, gxdefs::TgdxUELIndex &DomainSyNrs) = 0;
        virtual int gdxSymbolGetDomainX(int SyNr, gxdefs::TgdxStrIndex &DomainIDs) = 0;

        virtual int gdxSymbolDim(int SyNr) = 0;

        virtual int gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, std::string &ExplTxt) = 0;

        virtual int gdxSymbolSetDomain(const gxdefs::TgdxStrIndex &DomainIDs) = 0;
        virtual int gdxSymbolSetDomainX(int SyNr, const gxdefs::TgdxStrIndex &DomainIDs) = 0;

        virtual int gdxSystemInfo(int &SyCnt, int &UelCnt) = 0;

        virtual int gdxUELRegisterDone() = 0;
        virtual int gdxUELRegisterRaw(const std::string &Uel) = 0;
        virtual int gdxUELRegisterRawStart() = 0;
        virtual int gdxUELRegisterStr(const std::string &Uel, int &UelNr) = 0;
        virtual int gdxUELRegisterStrStart() = 0;

        virtual int gdxUMUelGet(int UelNr, std::string &Uel, int &UelMap) = 0;
        virtual int gdxUMUelInfo(int &UelCnt, int &HighMap) = 0;

        virtual int gdxCurrentDim() = 0;
        virtual int gdxRenameUEL(const std::string &OldName, const std::string &NewName) = 0;
    };

}