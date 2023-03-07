#pragma once

// Description:
//  This unit defines the GDX Object as a C++ object.

// Quick settings:
// P3_COLLECTIONS: Use paul objects as much as possible and gmsheapnew for TLinkedData, most verbatim port
// MIXED_COLLECTIONS (default): Mix-and-match custom and builtin data structures for best performance
#if !defined(P3_COLLECTIONS) && !defined(MIX_COLLECTIONS)
#define MIX_COLLECTIONS
#endif

#include "gclgms.h"

#include "gmsdata.h"
#include "datastorage.h"
#include "strhash.h"
#include "gmsobj.h"

#include "utils.h"

#include <memory>
#include <map>
#include <utility>
#include <vector>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <cstring>
#include <variant>

//======================================================================================================================
// Various switches for container/data structure implementation choices:
//======================================================================================================================

// Only if no other C++-hashmap is chosen, switch to TXStrHash implementation as close as possible to original P3 one
#define TSH_LEGACY

// Use TXStrPool port for SetTextList
#define TXSPOOL_LEGACY

// Use verbatim port for TTblGamsData
#define TBL_GMSDATA_LEGACY

#if defined(MIX_COLLECTIONS)
    #undef TSH_LEGACY
    #undef TXSPOOL_LEGACY
    #undef TBL_GMSDATA_LEGACY
#endif

//======================================================================================================================

namespace gdlib::gmsstrm {
    class TMiBufferedStreamDelphi;
}

#if defined(_WIN32)
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#endif

namespace yaml {
    class TYAMLFile;
}

namespace gxfile {

    using TgdxUELIndex = std::array<int, GMS_MAX_INDEX_DIM>;
    using TgdxValues = std::array<double, GMS_VAL_SCALE+ 1>;

    using TDomainIndexProc_t = void(*)(int RawIndex, int MappedIndex, void* Uptr);
    using TDataStoreProc_t = void(*)(const int* Indx, const double* Vals);
    using TDataStoreFiltProc_t = int(*)(const int *Indx, const double *Vals, void *Uptr);
    using TDataStoreExProc_t = void (*)(const int *Indx, const double *Vals, const int afdim, void *Uptr);

    using TDataStoreExProc_F = void (*)(const int *Indx, const double *Vals, const int afdim, int64_t Uptr);
    using TDataStoreFiltProc_F = int(*)(const int *Indx, const double *Vals, int64_t Uptr);
    using TDomainIndexProc_F = void(*)(int RawIndex, int MappedIndex, int64_t Uptr);

    const std::array<int, GMS_DT_ALIAS+1> DataTypSize {1,1,5,5,0};

    const int   DOMC_UNMAPPED = -2, // indicator for unmapped index pos
                DOMC_EXPAND = -1, // indicator growing index pos
                DOMC_STRICT = 0; // indicator mapped index pos

    const std::string   BADUEL_PREFIX = "?L__",
                        BADStr_PREFIX = "?Str__",
                        strGDXCOMPRESS = "GDXCOMPRESS",
                        strGDXCONVERT = "GDXCONVERT";

    struct TDFilter {
        int FiltNumber{}, FiltMaxUel{};
        gdlib::gmsobj::TBooleanBitArray FiltMap{};
        bool FiltSorted{};

        TDFilter(int Nr, int UserHigh) :
            FiltNumber{Nr},
            FiltMaxUel{UserHigh}
        {
        }

        ~TDFilter() = default;

        [[nodiscard]] int MemoryUsed() const {
            return FiltMap.MemoryUsed();
        }

        [[nodiscard]] bool InFilter(int V) const {
            return V >= 0 && V <= FiltMaxUel && FiltMap.GetBit(V);
        }

        void SetFilter(int ix, bool v) {
            FiltMap.SetBit(ix, v);
        }
    };

    using TSetBitMap = gdlib::gmsobj::TBooleanBitArray;

    enum TgdxDAction {
        dm_unmapped,
        dm_strict,
        dm_filter,
        dm_expand
    };

    struct TDomain {
        TDFilter *DFilter;
        TgdxDAction DAction;
    };

    using TDomainList = std::array<TDomain, GLOBAL_MAX_INDEX_DIM>;

    using TCommentsList = gdlib::gmsobj::TXStrings;

    struct TgdxSymbRecord {
        int SSyNr;
        int64_t SPosition;
        int SDim, SDataCount, SErrors;
        gdxSyType SDataType;
        int SUserInfo;
        bool SSetText;
        std::string SExplTxt;
        bool SIsCompressed;
        std::unique_ptr<int[]>  SDomSymbols, // real domain info
                                SDomStrings; // relaxed domain info
        std::optional<TCommentsList> SCommentsList;
        bool SScalarFrst; // not stored
        std::unique_ptr<TSetBitMap> SSetBitMap; // for 1-dim sets only
    };
    using PgdxSymbRecord = TgdxSymbRecord*;

    enum TgdxIntlValTyp { // values stored internally via the indicator byte
        vm_valund,
        vm_valna,
        vm_valpin,
        vm_valmin,
        vm_valeps,
        vm_zero,
        vm_one,
        vm_mone,
        vm_half,
        vm_two,
        vm_normal,
        vm_count
    };

    enum TgxFileMode {
        f_not_open,
        fr_init,
        fw_init,
        fw_dom_raw,
        fw_dom_map,
        fw_dom_str,
        fw_raw_data,
        fw_map_data,
        fw_str_data,
        f_raw_elem,
        f_map_elem,
        f_str_elem,
        fr_raw_data,
        fr_map_data,
        fr_mapr_data,
        fr_str_data,
        fr_filter,
        fr_slice,
        tgxfilemode_count
    };

    class TgxModeSet : public utils::IContainsPredicate<TgxFileMode> {
        std::array<bool, tgxfilemode_count> modeActive{};
        uint8_t count{};
    public:
        TgxModeSet(const std::initializer_list<TgxFileMode> &modes);
        explicit TgxModeSet(TgxFileMode mode);
        ~TgxModeSet() override = default;
        [[nodiscard]] bool contains(const TgxFileMode& mode) const override;
        [[nodiscard]] bool empty() const;
    };

    const TgxModeSet    AnyWriteMode {fw_init,fw_dom_raw, fw_dom_map, fw_dom_str, fw_raw_data,fw_map_data,fw_str_data},
                        AnyReadMode {fr_init,fr_raw_data,fr_map_data,fr_mapr_data,fr_str_data};

    enum class TgdxElemSize {
        sz_byte,
        sz_word,
        sz_integer
    };

    // N.B.: we store integers in [0..high(integer)] in TIntegerMapping, so
    // FMAXCAPACITY = high(integer) + 1 is all we will ever need, and we will
    // never get a request to grow any larger.  The checks and code
    // in growMapping reflect this
    class TIntegerMapping {
        int64_t FCapacity {}, FMapBytes {};
        int64_t FMAXCAPACITY {std::numeric_limits<int>::max() + static_cast<int64_t>(1)};
        int FHighestIndex{};
        int *PMap {};

        void growMapping(int F);
    public:
        TIntegerMapping() {}
        ~TIntegerMapping();
        [[nodiscard]] int MemoryUsed() const;
        [[nodiscard]] int GetHighestIndex() const;
        [[nodiscard]] int GetMapping(int F) const;
        void SetMapping(int F, int T);
        [[nodiscard]] int size() const;
        [[nodiscard]] bool empty() const;
    };

    enum TUELUserMapStatus {map_unknown, map_unsorted, map_sorted, map_sortgrow, map_sortfull};

    template<typename T>
#ifdef TSH_LEGACY
    using TXStrHashListImpl = gdlib::strhash::TXStrHashListLegacy<T>;
#else
    using TXStrHashListImpl = gdlib::strhash::TXStrHashList<T>;
#endif

    template<typename T>
#ifdef TSH_LEGACY
    using TXCSStrHashListImpl = gdlib::strhash::TXCSStrHashListLegacy<T>;
#else
    using TXCSStrHashListImpl = gdlib::strhash::TXCSStrHashList<T>;
#endif

    class TUELTable : public TXStrHashListImpl<int> {
        TUELUserMapStatus FMapToUserStatus {map_unknown};
    public:
        std::unique_ptr<TIntegerMapping> UsrUel2Ent {}; // from user uelnr to table entry
        TUELTable();
        ~TUELTable() override = default;
        [[nodiscard]] int size() const;
        [[nodiscard]] bool empty() const;
        int GetUserMap(int i);
        void SetUserMap(int EN, int N);
        int NewUsrUel(int EN);
        int AddUsrNew(const char *s, size_t slen);
        int AddUsrIndxNew(const char *s, size_t slen, int UelNr);
        [[nodiscard]] int GetMaxUELLength() const;
        int IndexOf(const char *s);
        int AddObject(const char *id, size_t idlen, int mapping);
        int StoreObject(const char *id, size_t idlen, int mapping);
        const char *operator[](int index) const;
        void RenameEntry(int N, const char *s);
        [[nodiscard]] int MemoryUsed() const;
        void SaveToStream(gdlib::gmsstrm::TXStreamDelphi &S);
        void LoadFromStream(gdlib::gmsstrm::TXStreamDelphi &S);
        TUELUserMapStatus GetMapToUserStatus();
        void ResetMapToUserStatus();
    };

    std::string MakeGoodExplText(std::string_view s);

    struct TAcronym {
        std::string AcrName{}, AcrText{};
        int AcrMap{}, AcrReadMap{};
        bool AcrAutoGen{};

        TAcronym(std::string Name, const std::string& Text, int Map);
        explicit TAcronym(gdlib::gmsstrm::TXStreamDelphi &S);
        void FillFromStream(gdlib::gmsstrm::TXStreamDelphi &S);
        TAcronym() = default;
        [[nodiscard]] int MemoryUsed() const;
        void SaveToStream(gdlib::gmsstrm::TXStreamDelphi &S) const;
    };

    class TAcronymList {
        gdlib::gmsobj::TXList<TAcronym> FList;
    public:
        TAcronymList() = default;
        ~TAcronymList();
        int FindEntry(int Map);
        int FindName(const char *Name);
        int AddEntry(const std::string& Name, const std::string& Text, int Map);
        void CheckEntry(int Map);
        void SaveToStream(gdlib::gmsstrm::TXStreamDelphi& S);
        void LoadFromStream(gdlib::gmsstrm::TXStreamDelphi& S);
        int MemoryUsed();
        [[nodiscard]] int size() const;
        TAcronym &operator[](int Index);
    };

    class TFilterList {
        gdlib::gmsobj::TXList<TDFilter> FList;
    public:
        TFilterList() = default;
        ~TFilterList();
        void AddFilter(TDFilter *F);
        void DeleteFilter(int ix);
        TDFilter *FindFilter(int Nr);
        [[nodiscard]] size_t MemoryUsed() const;
    };

    using TIntlValueMapDbl = std::array<double, vm_count>;
    using TIntlValueMapI64 = std::array<int64_t, vm_count>;

    using LinkedDataType = gdlib::datastorage::TLinkedData<int, double>;
    using LinkedDataIteratorType = gdlib::datastorage::TLinkedDataRec<int, double> *;

    #if defined(TXSPOOL_LEGACY)
        using TSetTextList = gdlib::gmsobj::TXStrPool<uint8_t>;
    #else
        using TSetTextList = TXCSStrHashListImpl<uint8_t>;
    #endif

    using TNameList = TXStrHashListImpl<PgdxSymbRecord>;

#ifdef TBL_GMSDATA_LEGACY
    template<typename T>
    using TTblGamsDataImpl = gdlib::gmsdata::TTblGamsDataLegacy<T>;
#else
    template<typename T>
    using TTblGamsDataImpl = gdlib::gmsdata::TTblGamsData<T>;
#endif

    // FIXME: It appears the object field is not actually needed
    // Find a way to use TXStrHashList anyways (w/out wasting a byte per entry as it is right now)
    using TDomainStrList = TXStrHashListImpl<uint8_t>;

    enum tvarvaltype {
        vallevel, // 1
        valmarginal, // 2
        vallower, // 3
        valupper, // 4
        valscale // 5
    };

    // Description:
    //    Class for reading and writing gdx files
    class TGXFileObj {
    public:
        enum class TraceLevels { trl_none, trl_errors, trl_some, trl_all };
    private:
#ifdef YAML
        std::unique_ptr<yaml::TYAMLFile> YFile;
#endif
        bool writeAsYAML{};
        std::unique_ptr<gdlib::gmsstrm::TMiBufferedStreamDelphi> FFile;
        TgxFileMode fmode {f_not_open}, fmode_AftReg {f_not_open};
        enum {stat_notopen, stat_read, stat_write} fstatus {stat_notopen};
        int fComprLev{};
        std::unique_ptr<TUELTable> UELTable;
        std::unique_ptr<TSetTextList> SetTextList {};
        std::unique_ptr<int[]> MapSetText{};
        int FCurrentDim{};
        std::array<int, GLOBAL_MAX_INDEX_DIM> LastElem{}, PrevElem{}, MinElem{}, MaxElem{};
        std::array<std::array<char, GLOBAL_UEL_IDENT_SIZE>, GLOBAL_MAX_INDEX_DIM> LastStrElem{};
        int DataSize{};
        tvarvaltype LastDataField{};
        std::unique_ptr<TNameList> NameList;
        std::unique_ptr<TDomainStrList> DomainStrList;
        std::unique_ptr<LinkedDataType> SortList;
        std::optional<LinkedDataIteratorType> ReadPtr;
        std::unique_ptr<TTblGamsDataImpl<double>> ErrorList;
        PgdxSymbRecord CurSyPtr{};
        int ErrCnt{}, ErrCntTotal{};
        int LastError{}, LastRepError{};
        std::unique_ptr<TFilterList> FilterList;
        TDFilter *CurFilter{};
        TDomainList DomainList{};
        bool StoreDomainSets{true};
        TIntlValueMapDbl intlValueMapDbl{}, readIntlValueMapDbl{};
        TIntlValueMapI64 intlValueMapI64{};
        TraceLevels TraceLevel {TraceLevels::trl_all};
        std::string TraceStr;
        int VersionRead{};
        std::string FProducer, FProducer2, FileSystemID;
        int64_t MajorIndexPosition{};
        int64_t NextWritePosition{};
        int DataCount{}, NrMappedAdded{};
        std::array<TgdxElemSize, GLOBAL_MAX_INDEX_DIM> ElemType{};
        std::string MajContext;
        std::array<std::optional<TIntegerMapping>, GLOBAL_MAX_INDEX_DIM> SliceIndxs, SliceRevMap;
        int SliceSyNr{};
        std::array<std::string, GMS_MAX_INDEX_DIM> SliceElems;
        bool    DoUncompress{}, // when reading
                CompressOut{}; // when writing
        int DeltaForWrite{}; // delta for last dimension or first changed dimension
        int DeltaForRead{}; // first position indicating change
        double Zvalacr{}; // tricky
        std::unique_ptr<TAcronymList> AcronymList;
        std::array<TSetBitMap *, GLOBAL_MAX_INDEX_DIM> WrBitMaps{};
        bool ReadUniverse{};
        int UniverseNr{}, UelCntOrig{}; // original uel count when we open the file
        int AutoConvert{1};
        int NextAutoAcronym{};
        bool AppendActive{};

#ifndef VERBOSE_TRACE
        const TraceLevels defaultTraceLevel {TraceLevels::trl_none};
        const bool verboseTrace {};
#else
        const TraceLevels defaultTraceLevel {TraceLevels::trl_all};
        const bool verboseTrace {true};
#endif

        //api wrapper magic for Fortran
        TDataStoreFiltProc_t gdxDataReadRawFastFilt_DP{};
        TDomainIndexProc_t gdxGetDomainElements_DP{};

        bool PrepareSymbolWrite(std::string_view Caller, const char *AName, std::string_view AText, int ADim, int AType, int AUserInfo);
        int PrepareSymbolRead(std::string_view Caller, int SyNr, const int *ADomainNrs, TgxFileMode newmode);

        void InitErrors();
        void SetError(int N);
        void ReportError(int N);
        bool ErrorCondition(bool cnd, int N);

        bool MajorCheckMode(std::string_view Routine, TgxFileMode m);
        bool MajorCheckMode(std::string_view Routine, const TgxModeSet &MS);

        bool CheckMode(std::string_view Routine);
        bool CheckMode(std::string_view Routine, TgxFileMode m);
        bool CheckMode(std::string_view Routine, const TgxModeSet &MS);


        void WriteTrace(std::string_view s) const;
        void InitDoWrite(int NrRecs);
        bool DoWrite(const int *AElements, const double *AVals);
        bool DoRead(double *AVals, int &AFDim);
        void AddToErrorListDomErrs(const std::array<int, GLOBAL_MAX_INDEX_DIM>& AElements, const double * AVals);
        void AddToErrorList(const int *AElements, const double *AVals);
        void GetDefaultRecord(double *Avals) const;
        double AcronymRemap(double V);
        bool IsGoodNewSymbol(const char *s);
        bool ResultWillBeSorted(const int *ADomainNrs);

        int gdxOpenReadXX(const char *Afn, int filemode, int ReadMode, int &ErrNr);

        // This one is a helper function for a callback from a Fortran client
        void gdxGetDomainElements_DP_FC(int RawIndex, int MappedIndex, void* Uptr);
        int gdxDataReadRawFastFilt_DP_FC(const int* Indx, const double* Vals, void* Uptr);

    public:
        bool    gdxGetDomainElements_DP_CallByRef{},
                gdxDataReadRawFastFilt_DP_CallByRef{},
                gdxDataReadRawFastEx_DP_CallByRef{};

        explicit TGXFileObj(std::string &ErrMsg);
        ~TGXFileObj();

        int gdxOpenWrite(const char *FileName, const char *Producer, int &ErrNr);
        int gdxOpenWriteEx(const char *FileName, const char *Producer, int Compr, int &ErrNr);
        int gdxDataWriteStrStart(const char *SyId, const char *ExplTxt, int Dim, int Typ, int UserInfo);
        int gdxDataWriteStr(const char **KeyStr, const double *Values);
        int gdxDataWriteDone();
        int gdxUELRegisterMapStart();
        int gdxUELRegisterMap(int UMap, const char *Uel);
        int gdxClose();
        int gdxResetSpecialValues();
        int gdxErrorStr(int ErrNr, char *ErrMsg);
        static int gdxErrorStrStatic(int ErrNr, char *ErrMsg);
        int gdxOpenRead(const char *FileName, int &ErrNr);
        int gdxFileVersion(char *FileStr, char *ProduceStr);
        int gdxFindSymbol(const char *SyId, int &SyNr);
        int gdxDataReadStr(char **KeyStr, double *Values, int &DimFrst);
        int gdxDataReadDone();
        int gdxSymbolInfo(int SyNr, char *SyId, int &Dim, int &Typ);
        int gdxDataReadStrStart(int SyNr, int &NrRecs);
        int gdxAddAlias(const char *Id1, const char *Id2);
        int gdxAddSetText(const char *Txt, int &TxtNr);
        int gdxDataErrorCount();
        int gdxDataErrorRecord(int RecNr,  int *KeyInt, double * Values);
        int gdxDataErrorRecordX(int RecNr,  int *KeyInt,  double *Values);
        int gdxDataReadRaw(int *KeyInt, double *Values, int &DimFrst);
        int gdxDataReadRawStart(int SyNr, int &NrRecs);
        int gdxDataWriteRaw(const int* KeyInt, const double* Values);
        int gdxDataWriteRawStart(const char *SyId, const char *ExplTxt, int Dimen, int Typ,
                                 int UserInfo);
        [[nodiscard]] int gdxErrorCount() const;
        int gdxGetElemText(int TxtNr, char *Txt, int &Node);
        int gdxGetLastError();
        int gdxGetSpecialValues(double *Avals);
        int gdxSetSpecialValues(const double *AVals);
        int gdxSymbolGetDomain(int SyNr, int *DomainSyNrs);
        int gdxSymbolGetDomainX(int SyNr, char **DomainIDs);
        int gdxSymbolDim(int SyNr);
        int gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt);
        int gdxSymbolSetDomain(const char **DomainIDs);
        int gdxSymbolSetDomainX(int SyNr, const char **DomainIDs);
        int gdxSystemInfo(int &SyCnt, int &UelCnt);
        int gdxUELRegisterDone();
        int gdxUELRegisterRaw(const char *Uel);
        int gdxUELRegisterRawStart();
        int gdxUELRegisterStr(const char *Uel, int &UelNr);
        int gdxUELRegisterStrStart();
        int gdxUMUelGet(int UelNr, char *Uel, int &UelMap);
        int gdxUMUelInfo(int &UelCnt, int &HighMap);
        [[nodiscard]] int gdxCurrentDim() const;
        int gdxRenameUEL(const char *OldName, const char *NewName);
        int gdxOpenReadEx(const char *FileName, int ReadMode, int &ErrNr);
        int gdxGetUEL(int uelNr, char *Uel);
        int gdxDataWriteMapStart(const char *SyId, const char *ExplTxt, int Dimen, int Typ,
                                 int UserInfo);
        int gdxDataWriteMap(const int *KeyInt, const double *Values);
        int gdxDataReadMapStart(int SyNr, int &NrRecs);
        int gdxDataReadMap(int RecNr, int *KeyInt, double *Values, int &DimFrst);

        void SetTraceLevel(TraceLevels tl);
        void SetWriteAsYAML(bool asYAML);

        // region Acronym handling
        [[nodiscard]] int gdxAcronymCount() const;
        int gdxAcronymGetInfo(int N, char *AName, char *Txt, int &AIndx) const;
        int gdxAcronymSetInfo(int N, const char *AName, const char *Txt, int AIndx);
        int gdxAcronymNextNr(int nv);
        int gdxAcronymGetMapping(int N, int &orgIndx, int &newIndx, int &autoIndex);
        // endregion

        // region Filter handling
        int gdxFilterExists(int FilterNr);
        int gdxFilterRegisterStart(int FilterNr);
        int gdxFilterRegister(int UelMap);
        int gdxFilterRegisterDone();
        int gdxDataReadFilteredStart(int SyNr, const int* FilterAction, int& NrRecs);
        // endregion

        int gdxSetTextNodeNr(int TxtNr, int Node);
        int gdxGetDomainElements(int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int& NrElem, void* UPtr);
        int gdxSetTraceLevel(int N, const char *s);
        int gdxAcronymAdd(const char *AName, const char *Txt, int AIndx);
        [[nodiscard]] int gdxAcronymIndex(double V) const;
        int gdxAcronymName(double V, char *AName);
        [[nodiscard]] double gdxAcronymValue(int AIndx) const;
        int gdxAutoConvert(int nv);
        int gdxGetDLLVersion(char *V) const;
        int gdxFileInfo(int &FileVer, int &ComprLev) const;
        int gdxDataReadSliceStart(int SyNr, int* ElemCounts);
        int gdxDataReadSlice(const char** UelFilterStr, int& Dimen, TDataStoreProc_t DP);
        int gdxDataSliceUELS(const int* SliceKeyInt, char** KeyStr);
        int64_t gdxGetMemoryUsed();
        int gdxMapValue(double D, int& sv);
        int gdxOpenAppend(const char * FileName, const char * Producer, int& ErrNr);
        int gdxSetHasText(int SyNr);
        int gdxSetReadSpecialValues(const double *AVals);
        int gdxSymbIndxMaxLength(int SyNr, int* LengthInfo);
        [[nodiscard]] int gdxSymbMaxLength() const;
        int gdxSymbolAddComment(int SyNr, const char* Txt);
        int gdxSymbolGetComment(int SyNr, int N, char *Txt);
        int gdxUELMaxLength();
        int gdxUMFindUEL(const char *Uel, int& UelNr, int& UelMap);
        [[nodiscard]] int gdxStoreDomainSets() const;
        void gdxStoreDomainSetsSet(int x);
        int gdxDataReadRawFastFilt(int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP);
        int gdxDataReadRawFast(int SyNr, TDataStoreProc_t DP, int &NrRecs);
        int gdxDataReadRawFastEx(int SyNr, TDataStoreExProc_t DP, int &NrRecs, void *Uptr);
    };

    extern std::string DLLLoadPath; // can be set by loader, so the "dll" knows where it is loaded from

    union uInt64 {
        int64_t i;
        void *p;
    };

    bool IsGoodIdent(std::string_view S);

    int ConvertGDXFile(const std::string &fn, const std::string &MyComp);

}
