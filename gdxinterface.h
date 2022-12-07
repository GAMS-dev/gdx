#pragma once

#include <string>
#include <array>
#include <cstring>
#include <vector>
#include "gxdefs.h"

namespace gdxinterface {
    class CharBuf {
        std::array<char, 256> buf;

    public:
        char* get() { return buf.data(); }
        std::string str() const {
            return buf.data();
        };

        operator std::string() const {
            return str();
        }

        int size() const { return (int)buf.size(); }
    };

    class StrRef {
        char *s;
    public:
        StrRef(char *_s) : s(_s) {}

        StrRef &operator=(const std::string &other) {
            std::memcpy(s, other.c_str(), sizeof(char)*(other.length()+1));
            return *this;
        }

        const char *c_str() {
            return s;
        }

        bool empty() const {
            return s[0] == '\0';
        }

        operator std::string() const {
            std::string res;
            res.assign(s);
            return res;
        }

        std::string str() const {
            std::string res;
            res.assign(s);
            return res;
        }

        bool operator==(const std::string &other) {
            return !std::strcmp(other.c_str(), s);
        }
    };

    class StrIndexBuffers {
        std::array<std::array<char, 256>, 20> bufContents{};
        std::array<char*, 20> bufPtrs{};
    public:
        explicit StrIndexBuffers(const gxdefs::TgdxStrIndex *strIndex = nullptr) {
            for (int i{}; i < bufPtrs.size(); i++) {
                bufPtrs[i] = bufContents[i].data();
                if (strIndex)
                    memcpy(bufPtrs[i], (*strIndex)[i].c_str(),(*strIndex)[i].length()+1);
            }
        }

        StrRef operator[](int index) {
            return {bufPtrs[index]};
        }

        char **ptrs() { return bufPtrs.data(); }
        const char** cptrs() { return (const char **)bufPtrs.data(); }

        gxdefs::TgdxStrIndex strs() const {
            gxdefs::TgdxStrIndex res;
            for (int i{}; i < res.size(); i++) {
                res[i].assign(bufPtrs[i]);
            }
            return res;
        }

        void clear() {
            for (int i{}; i < bufContents.size(); i++)
                bufContents[i].fill(0);
        }

        StrRef front() {
            return {bufPtrs[0]};
        }
    };

    class GDXInterface {
    public:
        virtual ~GDXInterface() = default;

        virtual int gdxFileVersion(std::string &FileStr, std::string &ProduceStr) = 0;
        virtual int gdxSystemInfo(int &SyCnt, int &UelCnt) = 0;

        // region Open file for reading or writing with/without additional options
        virtual int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) = 0;
        virtual int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) = 0;
        virtual int gdxOpenRead(const std::string &FileName, int &ErrNr) = 0;
        virtual int gdxOpenReadEx(const std::string &FileName, int ReadMode, int &ErrNr) = 0;
        // endregion
        virtual int gdxClose() = 0;

        // region Data write
        virtual int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) = 0;
        virtual int gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ, int UserInfo) = 0;
        virtual int gdxDataWriteMapStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ, int UserInfo) = 0;
        virtual int gdxDataWriteStr(const char **KeyStr, const double *Values) = 0;

        int gdxDataWriteStr(const std::vector<std::string> &KeyStr, const double *Values);

        virtual int gdxDataWriteRaw(const int *KeyInt, const double *Values) = 0;
        virtual int gdxDataWriteMap(const int *KeyInt, const double *Values) = 0;
        virtual int gdxDataWriteDone() = 0;
        // endregion

        // region Data read
        virtual int gdxDataReadRawStart(int SyNr, int &NrRecs) = 0;
        virtual int gdxDataReadRaw(int *KeyInt, double *Values, int &DimFrst) = 0;
        virtual int gdxDataReadStrStart(int SyNr, int &NrRecs) = 0;
        virtual int gdxDataReadStr(char **KeyStr, double *Values, int &DimFrst) = 0;
        virtual int gdxDataReadMapStart(int SyNr, int &NrRecs) = 0;
        virtual int gdxDataReadMap(int RecNr, int *KeyInt, double *Values, int &DimFrst) = 0;

        virtual int gdxDataReadDone() = 0;
        // endregion

        virtual int gdxFindSymbol(const std::string &SyId, int &SyNr) = 0;
        virtual int gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) = 0;
        virtual int gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, std::string &ExplTxt) = 0;
        virtual int gdxSymbolDim(int SyNr) = 0;

        virtual int gdxAddAlias(const std::string &Id1, const std::string &Id2) = 0;
        virtual int gdxAddSetText(const std::string &Txt, int &TxtNr) = 0;
        virtual int gdxGetElemText(int TxtNr, std::string &Txt, int &Node) = 0;

        //region Error handling
        virtual int gdxDataErrorCount() = 0;
        virtual int gdxDataErrorRecord(int RecNr, int *KeyInt, double * Values) = 0;
        virtual int gdxDataErrorRecordX(int RecNr, int *KeyInt, double *Values) = 0;
        virtual int gdxErrorCount() = 0;
        virtual int gdxErrorStr(int ErrNr, std::string &ErrMsg) = 0;
        virtual int gdxGetLastError() = 0;
        // endregion

        virtual int gdxGetSpecialValues(gxdefs::TgdxSVals &Avals) = 0;
        virtual int gdxSetSpecialValues(const gxdefs::TgdxSVals &AVals) = 0;

        virtual int gdxSymbolSetDomain(const char **DomainIDs) = 0;
        virtual int gdxSymbolSetDomainX(int SyNr, const char **DomainIDs) = 0;
        virtual int gdxSymbolGetDomain(int SyNr, int *DomainSyNrs) = 0;
        virtual int gdxSymbolGetDomainX(int SyNr, char **DomainIDs) = 0;

        // region UEL register
        virtual int gdxUELRegisterRawStart() = 0;
        virtual int gdxUELRegisterRaw(const std::string &Uel) = 0;
        virtual int gdxUELRegisterStrStart() = 0;
        virtual int gdxUELRegisterStr(const std::string &Uel, int &UelNr) = 0;
        virtual int gdxUELRegisterMapStart() = 0;
        virtual int gdxUELRegisterMap(int UMap, const std::string &Uel) = 0;
        virtual int gdxUELRegisterDone() = 0;
        // endregion

        virtual int gdxGetUEL(int uelNr, std::string &Uel) = 0;
        virtual int gdxUMUelGet(int UelNr, std::string &Uel, int &UelMap) = 0;
        virtual int gdxUMUelInfo(int &UelCnt, int &HighMap) = 0;

        virtual int gdxCurrentDim() = 0;
        virtual int gdxRenameUEL(const std::string &OldName, const std::string &NewName) = 0;

        // region deprecated functions
        virtual int gdxAcronymCount() const = 0;
        virtual int gdxAcronymGetInfo(int N, std::string &AName, std::string &Txt, int &AIndx) const = 0;
        virtual int gdxAcronymSetInfo(int N, const std::string &AName, const std::string &Txt, int AIndx) = 0;
        virtual int gdxAcronymNextNr(int nv) = 0;
        virtual int gdxAcronymGetMapping(int N, int &orgIndx, int &newIndx, int &autoIndex) = 0;
        virtual int gdxFilterExists(int FilterNr) = 0;
        virtual int gdxFilterRegisterStart(int FilterNr) = 0;
        virtual int gdxFilterRegister(int UelMap) = 0;
        virtual int gdxFilterRegisterDone() = 0;
        virtual int gdxDataReadFilteredStart(int SyNr, const int *FilterAction, int &NrRecs) = 0;
        virtual int gdxAcronymAdd(const std::string &AName, const std::string &Txt, int AIndx) = 0;
        virtual int gdxAcronymIndex(double V) const = 0;
        virtual int gdxAcronymName(double V, std::string &AName) = 0;
        virtual double gdxAcronymValue(int AIndx) const = 0;
        virtual int gdxSymbolAddComment(int SyNr, const std::string& Txt) = 0;
        virtual int gdxSymbolGetComment(int SyNr, int N, std::string& Txt) = 0;
        virtual int gdxStoreDomainSets() = 0;
        virtual void gdxStoreDomainSetsSet(int x) = 0;
        virtual int gdxOpenAppend(const std::string& FileName, const std::string& Producer, int& ErrNr) = 0;
        virtual int gdxSymbIndxMaxLength(int SyNr, int *LengthInfo) = 0;
        virtual int gdxUELMaxLength() = 0;
        // endregion

        virtual std::string getImplName() const = 0;
    };

}