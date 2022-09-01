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
        int gdxDataWriteStr(const gxdefs::TgdxStrIndex &KeyStr, const gxdefs::TgdxValues &Values) override;
        int gdxDataWriteDone() override;

        int gdxClose() override;

        int gdxOpenRead(const std::string &FileName, int &ErrNr) override;

        int gdxFileVersion(std::string &FileStr, std::string &ProduceStr) override;

        int gdxFindSymbol(const std::string &SyId, int &SyNr) override;

        int gdxDataReadStr(gxdefs::TgdxStrIndex &KeyStr, gxdefs::TgdxValues &Values, int &DimFrst) override;

        int gdxDataReadDone() override;

        int gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) override;

        int gdxDataReadStrStart(int SyNr, int &NrRecs) override;

        int gdxAddAlias(const std::string &Id1, const std::string &Id2) override;

        int gdxAddSetText(const std::string &Txt, int &TxtNr) override;

        int gdxDataErrorCount() override;

        int gdxDataErrorRecord(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values) override;

        int gdxDataErrorRecordX(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values) override;

        int gdxDataReadRaw(gxdefs::TgdxUELIndex &KeyInt, gxdefs::TgdxValues &Values, int &DimFrst) override;

        int gdxDataReadRawStart(int SyNr, int &NrRecs) override;

        int gdxDataWriteRaw(const gxdefs::TgdxUELIndex &KeyInt, const gxdefs::TgdxValues &Values) override;

        int gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                 int UserInfo) override;

        int gdxErrorCount() override;

        int gdxErrorStr(int ErrNr, std::string &ErrMsg) override;

        int gdxGetElemText(int TxtNr, std::string &Txt, int &Node) override;

        int gdxGetLastError() override;

        int gdxGetSpecialValues(gxdefs::TgdxSVals &Avals) override;

        int gdxSetSpecialValues(const gxdefs::TgdxSVals &AVals) override;

        int gdxSymbolGetDomain(int SyNr, gxdefs::TgdxUELIndex &DomainSyNrs) override;

        int gdxSymbolGetDomainX(int SyNr, gxdefs::TgdxStrIndex &DomainIDs) override;

        int gdxSymbolDim(int SyNr) override;

        int gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, std::string &ExplTxt) override;

        int gdxSymbolSetDomain(const gxdefs::TgdxStrIndex &DomainIDs) override;

        int gdxSymbolSetDomainX(int SyNr, const gxdefs::TgdxStrIndex &DomainIDs) override;

        int gdxSystemInfo(int &SyCnt, int &UelCnt) override;

        int gdxUELRegisterDone() override;

        int gdxUELRegisterRaw(const std::string &Uel) override;

        int gdxUELRegisterRawStart() override;

        int gdxUELRegisterStr(const std::string &Uel, int &UelNr) override;

        int gdxUELRegisterStrStart() override;

        int gdxUMUelGet(int UelNr, std::string &Uel, int &UelMap) override;

        int gdxUMUelInfo(int &UelCnt, int &HighMap) override;

        int gdxCurrentDim() override;

        int gdxRenameUEL(const std::string &OldName, const std::string &NewName) override;
    };

}