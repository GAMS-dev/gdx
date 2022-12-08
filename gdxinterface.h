#pragma once

#include <string>
#include <array>
#include <cstring>
#include <vector>
#include "expertapi/gclgms.h"

namespace gdxinterface {

    using TgdxStrIndex = std::array<std::string, GMS_MAX_INDEX_DIM>;
    using TgdxUELIndex = std::array<int, GMS_MAX_INDEX_DIM>;
    using TgdxValues = std::array<double, GMS_VAL_SCALE+ 1>;

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
        explicit StrIndexBuffers(const TgdxStrIndex *strIndex = nullptr) {
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

        TgdxStrIndex strs() const {
            TgdxStrIndex res;
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

        virtual int gdxFileVersion(char *FileStr, char *ProduceStr) = 0;
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
        virtual int gdxSymbolInfo(int SyNr, char *SyId, int &Dim, int &Typ) = 0;
        virtual int gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt) = 0;
        virtual int gdxSymbolDim(int SyNr) = 0;

        virtual int gdxAddAlias(const std::string &Id1, const std::string &Id2) = 0;
        virtual int gdxAddSetText(const std::string &Txt, int &TxtNr) = 0;
        virtual int gdxGetElemText(int TxtNr, char *Txt, int &Node) = 0;

        //region Error handling
        virtual int gdxDataErrorCount() = 0;
        virtual int gdxDataErrorRecord(int RecNr, int *KeyInt, double * Values) = 0;
        virtual int gdxDataErrorRecordX(int RecNr, int *KeyInt, double *Values) = 0;
        virtual int gdxErrorCount() = 0;
        virtual int gdxErrorStr(int ErrNr, char *ErrMsg) = 0;
        virtual int gdxGetLastError() = 0;
        // endregion

        virtual int gdxGetSpecialValues(std::array<double, GMS_SVIDX_MAX> &Avals) = 0;
        virtual int gdxSetSpecialValues(const std::array<double, GMS_SVIDX_MAX> &AVals) = 0;

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

        virtual int gdxGetUEL(int uelNr, char *Uel) = 0;
        virtual int gdxUMUelGet(int UelNr, char *Uel, int &UelMap) = 0;
        virtual int gdxUMUelInfo(int &UelCnt, int &HighMap) = 0;

        virtual int gdxCurrentDim() = 0;
        virtual int gdxRenameUEL(const std::string &OldName, const std::string &NewName) = 0;

        // region deprecated functions
        virtual int gdxAcronymCount() const = 0;
        virtual int gdxAcronymGetInfo(int N, char *AName, char *Txt, int &AIndx) const = 0;
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
        virtual int gdxAcronymName(double V, char *AName) = 0;
        virtual double gdxAcronymValue(int AIndx) const = 0;
        virtual int gdxSymbolAddComment(int SyNr, const std::string& Txt) = 0;
        virtual int gdxSymbolGetComment(int SyNr, int N, char *Txt) = 0;
        virtual int gdxStoreDomainSets() = 0;
        virtual void gdxStoreDomainSetsSet(int x) = 0;
        virtual int gdxOpenAppend(const std::string& FileName, const std::string& Producer, int& ErrNr) = 0;
        virtual int gdxSymbIndxMaxLength(int SyNr, int *LengthInfo) = 0;
        virtual int gdxUELMaxLength() = 0;
        // endregion

        virtual std::string getImplName() const = 0;
    };

}