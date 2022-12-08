#pragma once

#include "gdxinterface.h"

struct gdxRec;
using gdxHandle_t = struct gdxRec *;

namespace xpwrap {

    class GDXFile : public gdxinterface::GDXInterface {
        gdxHandle_t pgx {};
    public:
        explicit GDXFile(std::string &ErrMsg);
        ~GDXFile() override;

        int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) override;
        int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) override;
        int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) override;
        int gdxDataWriteStr(const char **KeyStr, const double *Values) override;
        int gdxDataWriteDone() override;

        int gdxClose() override;

        int gdxOpenRead(const std::string &FileName, int &ErrNr) override;

        int gdxFileVersion(std::string &FileStr, std::string &ProduceStr) override;

        int gdxFindSymbol(const std::string &SyId, int &SyNr) override;

        int gdxDataReadStr(char **KeyStr, double *Values, int &DimFrst) override;

        int gdxDataReadDone() override;

        int gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) override;

        int gdxDataReadStrStart(int SyNr, int &NrRecs) override;

        int gdxAddAlias(const std::string &Id1, const std::string &Id2) override;

        int gdxAddSetText(const std::string &Txt, int &TxtNr) override;

        int gdxDataErrorCount() override;

        int gdxDataErrorRecord(int RecNr, int * KeyInt, double * Values) override;

        int gdxDataErrorRecordX(int RecNr, int * KeyInt, double * Values) override;

        int gdxDataReadRaw(int *KeyInt, double *Values, int &DimFrst) override;

        int gdxDataReadRawStart(int SyNr, int &NrRecs) override;

        int gdxDataWriteRaw(const int *KeyInt, const double *Values) override;

        int gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                 int UserInfo) override;

        int gdxErrorCount() override;

        int gdxErrorStr(int ErrNr, char *ErrMsg) override;

        int gdxGetElemText(int TxtNr, std::string &Txt, int &Node) override;

        int gdxGetLastError() override;

        int gdxGetSpecialValues(std::array<double, GMS_SVIDX_MAX> &Avals) override;

        int gdxSetSpecialValues(const std::array<double, GMS_SVIDX_MAX> &AVals) override;

        int gdxSymbolGetDomain(int SyNr, int *DomainSyNrs) override;

        int gdxSymbolGetDomainX(int SyNr, char **DomainIDs) override;

        int gdxSymbolDim(int SyNr) override;

        int gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, std::string &ExplTxt) override;

        int gdxSymbolSetDomain(const char **DomainIDs) override;

        int gdxSymbolSetDomainX(int SyNr, const char **DomainIDs) override;

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

        int gdxOpenReadEx(const std::string &FileName, int ReadMode, int &ErrNr) override;

        int gdxGetUEL(int uelNr, std::string &Uel) override;

        int gdxDataWriteMapStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                 int UserInfo) override;

        int gdxDataWriteMap(const int *KeyInt, const double *Values) override;

        int gdxUELRegisterMapStart() override;

        int gdxUELRegisterMap(int UMap, const std::string &Uel) override;

        int gdxDataReadMapStart(int SyNr, int &NrRecs) override;

        int gdxDataReadMap(int RecNr, int *KeyInt, double *Values, int &DimFrst) override;

        int gdxAcronymCount() const override;

        int gdxAcronymGetInfo(int N, char *AName, char *Txt, int &AIndx) const override;

        int gdxAcronymSetInfo(int N, const std::string &AName, const std::string &Txt, int AIndx) override;

        int gdxAcronymNextNr(int nv) override;

        int gdxAcronymGetMapping(int N, int &orgIndx, int &newIndx, int &autoIndex) override;

        int gdxFilterExists(int FilterNr) override;

        int gdxFilterRegisterStart(int FilterNr) override;

        int gdxFilterRegister(int UelMap) override;

        int gdxFilterRegisterDone() override;

        int gdxDataReadFilteredStart(int SyNr, const int *FilterAction, int &NrRecs) override;

        int gdxAcronymAdd(const std::string &AName, const std::string &Txt, int AIndx) override;

        int gdxAcronymIndex(double V) const override;

        int gdxAcronymName(double V, char *AName) override;

        double gdxAcronymValue(int AIndx) const override;

        int gdxSymbolAddComment(int SyNr, const std::string &Txt) override;

        int gdxSymbolGetComment(int SyNr, int N, std::string &Txt) override;

        int gdxStoreDomainSets() override;

        void gdxStoreDomainSetsSet(int x) override;

        int gdxOpenAppend(const std::string &FileName, const std::string &Producer, int &ErrNr) override;

        int gdxSymbIndxMaxLength(int SyNr, int *LengthInfo) override;

        int gdxUELMaxLength() override;

        std::string getImplName() const override;
    };

}