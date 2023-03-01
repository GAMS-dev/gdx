#pragma once

#include <string>
#include <array>
#include <cstring>
#include "expertapi/gclgms.h"

namespace gdxinterface {

    using TgdxStrIndex = std::array<std::string, GMS_MAX_INDEX_DIM>;
    using TgdxUELIndex = std::array<int, GMS_MAX_INDEX_DIM>;
    using TgdxValues = std::array<double, GMS_VAL_SCALE+ 1>;

    using TDomainIndexProc_t = void(*)(int RawIndex, int MappedIndex, void* Uptr);
    using TDataStoreProc_t = void(*)(const int* Indx, const double* Vals);
    using TDataStoreFiltProc_t = int(*)(const int *Indx, const double *Vals, void *Uptr);
    using TDataStoreExProc_t = void (*)(const int *Indx, const double *Vals, const int afdim, void *Uptr);

    using TDataStoreExProc_F = void (*)(const int *Indx, const double *Vals, const int afdim, int64_t Uptr);
    using TDataStoreFiltProc_F = int(*)(const int *Indx, const double *Vals, int64_t Uptr);
    using TDomainIndexProc_F = void(*)(int RawIndex, int MappedIndex, int64_t Uptr);

    class CharBuf {
        std::array<char, GMS_SSSIZE> buf;

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
        std::array<std::array<char, GMS_SSSIZE>, GMS_MAX_INDEX_DIM> bufContents{};
        std::array<char*, GMS_MAX_INDEX_DIM> bufPtrs{};
    public:
        explicit StrIndexBuffers(const TgdxStrIndex *strIndex = nullptr) {
            for (int i{}; i < (int)bufPtrs.size(); i++) {
                bufPtrs[i] = bufContents[i].data();
                if (strIndex)
                    std::memcpy(bufPtrs[i], (*strIndex)[i].c_str(),(*strIndex)[i].length()+1);
            }
        }

        StrRef operator[](int index) {
            return {bufPtrs[index]};
        }

        char **ptrs() { return bufPtrs.data(); }
        const char** cptrs() { return (const char **)bufPtrs.data(); }

        TgdxStrIndex strs() const {
            TgdxStrIndex res;
            for (int i{}; i < (int)res.size(); i++) {
                res[i].assign(bufPtrs[i]);
            }
            return res;
        }

        void clear() {
            for (int i{}; i < (int)bufContents.size(); i++)
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
        virtual int gdxOpenWrite(const char *FileName, const char *Producer, int &ErrNr) = 0;
        virtual int gdxOpenWriteEx(const char *FileName, const char *Producer, int Compr, int &ErrNr) = 0;
        virtual int gdxOpenRead(const char *FileName, int &ErrNr) = 0;
        virtual int gdxOpenReadEx(const char *FileName, int ReadMode, int &ErrNr) = 0;
        // endregion
        virtual int gdxClose() = 0;
        virtual int gdxResetSpecialValues() = 0;

        // region Data write
        virtual int gdxDataWriteStrStart(const char *SyId, const char *ExplTxt, int Dim, int Typ, int UserInfo) = 0;
        virtual int gdxDataWriteRawStart(const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) = 0;
        virtual int gdxDataWriteMapStart(const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) = 0;
        virtual int gdxDataWriteStr(const char **KeyStr, const double *Values) = 0;

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

        virtual int gdxFindSymbol(const char *SyId, int &SyNr) = 0;
        virtual int gdxSymbolInfo(int SyNr, char *SyId, int &Dim, int &Typ) = 0;
        virtual int gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt) = 0;
        virtual int gdxSymbolDim(int SyNr) = 0;

        virtual int gdxAddAlias(const char *Id1, const char *Id2) = 0;
        virtual int gdxAddSetText(const char *Txt, int &TxtNr) = 0;
        virtual int gdxGetElemText(int TxtNr, char *Txt, int &Node) = 0;

        //region Error handling
        virtual int gdxDataErrorCount() = 0;
        virtual int gdxDataErrorRecord(int RecNr, int *KeyInt, double * Values) = 0;
        virtual int gdxDataErrorRecordX(int RecNr, int *KeyInt, double *Values) = 0;
        virtual int gdxErrorCount() = 0;
        virtual int gdxErrorStr(int ErrNr, char *ErrMsg) = 0;
        virtual int gdxGetLastError() = 0;
        // endregion

        virtual int gdxGetSpecialValues(double *Avals) = 0;
        virtual int gdxSetSpecialValues(const double *AVals) = 0;

        virtual int gdxSymbolSetDomain(const char **DomainIDs) = 0;
        virtual int gdxSymbolSetDomainX(int SyNr, const char **DomainIDs) = 0;
        virtual int gdxSymbolGetDomain(int SyNr, int *DomainSyNrs) = 0;
        virtual int gdxSymbolGetDomainX(int SyNr, char **DomainIDs) = 0;

        // region UEL register
        virtual int gdxUELRegisterRawStart() = 0;
        virtual int gdxUELRegisterRaw(const char *Uel) = 0;
        virtual int gdxUELRegisterStrStart() = 0;
        virtual int gdxUELRegisterStr(const char *Uel, int &UelNr) = 0;
        virtual int gdxUELRegisterMapStart() = 0;
        virtual int gdxUELRegisterMap(int UMap, const char *Uel) = 0;
        virtual int gdxUELRegisterDone() = 0;
        // endregion

        virtual int gdxGetUEL(int uelNr, char *Uel) = 0;
        virtual int gdxUMUelGet(int UelNr, char *Uel, int &UelMap) = 0;
        virtual int gdxUMUelInfo(int &UelCnt, int &HighMap) = 0;
        virtual int gdxUMFindUEL(const char *Uel, int& UelNr, int& UelMap) = 0;

        virtual int gdxCurrentDim() = 0;
        virtual int gdxRenameUEL(const char *OldName, const char *NewName) = 0;

        // region deprecated functions
        virtual int gdxAcronymCount() const = 0;
        virtual int gdxAcronymGetInfo(int N, char *AName, char *Txt, int &AIndx) const = 0;
        virtual int gdxAcronymSetInfo(int N, const char *AName, const char *Txt, int AIndx) = 0;
        virtual int gdxAcronymNextNr(int nv) = 0;
        virtual int gdxAcronymGetMapping(int N, int &orgIndx, int &newIndx, int &autoIndex) = 0;
        virtual int gdxFilterExists(int FilterNr) = 0;
        virtual int gdxFilterRegisterStart(int FilterNr) = 0;
        virtual int gdxFilterRegister(int UelMap) = 0;
        virtual int gdxFilterRegisterDone() = 0;
        virtual int gdxDataReadFilteredStart(int SyNr, const int *FilterAction, int &NrRecs) = 0;
        virtual int gdxSetTraceLevel(int N, const char *s) = 0;
        virtual int gdxSetTextNodeNr(int TxtNr, int Node) = 0;
        virtual int gdxAcronymAdd(const char *AName, const char *Txt, int AIndx) = 0;
        virtual int gdxAcronymIndex(double V) const = 0;
        virtual int gdxAcronymName(double V, char *AName) = 0;
        virtual double gdxAcronymValue(int AIndx) const = 0;
        virtual int gdxSymbolAddComment(int SyNr, const char* Txt) = 0;
        virtual int gdxSymbolGetComment(int SyNr, int N, char *Txt) = 0;
        virtual int gdxStoreDomainSets() = 0;
        virtual void gdxStoreDomainSetsSet(int x) = 0;
        virtual int gdxOpenAppend(const char * FileName, const char * Producer, int& ErrNr) = 0;
        virtual int gdxSymbIndxMaxLength(int SyNr, int *LengthInfo) = 0;
        virtual int gdxUELMaxLength() = 0;
        // endregion

        virtual int gdxGetDomainElements(int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int& NrElem, void* UPtr) = 0;
        virtual int gdxAutoConvert(int nv) = 0;

        virtual int gdxGetDLLVersion(char *V) const = 0;
        virtual int gdxFileInfo(int &FileVer, int &ComprLev) const = 0;

        virtual int gdxDataReadSliceStart(int SyNr, int* ElemCounts) = 0;
        virtual int gdxDataReadSlice(const char** UelFilterStr, int& Dimen, TDataStoreProc_t DP) = 0;
        virtual int gdxDataSliceUELS(const int* SliceKeyInt, char** KeyStr) = 0;
        virtual int64_t gdxGetMemoryUsed() = 0;
        virtual int gdxMapValue(double D, int& sv) = 0;
        virtual int gdxSetHasText(int SyNr) = 0;
        virtual int gdxSetReadSpecialValues(const double *AVals) = 0;
        virtual int gdxSymbMaxLength() const = 0;
        virtual int gdxDataReadRawFastFilt(int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP) = 0;
        virtual int gdxDataReadRawFast(int SyNr, TDataStoreProc_t DP, int &NrRecs) = 0;
        virtual int gdxDataReadRawFastEx(int SyNr, TDataStoreExProc_t DP, int &NrRecs, void *Uptr) = 0;

        virtual std::string getImplName() const = 0;
    };

}