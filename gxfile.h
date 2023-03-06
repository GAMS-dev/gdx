#pragma once

// Description:
//  This unit defines the GDX Object as a C++ object.

// Quick settings:
// P3_COLLECTIONS: Use paul objects as much as possible and gmsheapnew for TLinkedData, most verbatim port
// MIXED_COLLECTIONS (default): Mix-and-match custom and builtin data structures for best performance
#if !defined(P3_COLLECTIONS) && !defined(MIX_COLLECTIONS)
#define MIX_COLLECTIONS
#endif

#include "gdxinterface.h"

#include "expertapi/gclgms.h"

#include "gdlib/gmsdata.h"
#include "gdlib/datastorage.h"
#include "gdlib/strhash.h"
#include "gdlib/gmsobj.h"

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

    const std::array<int, GMS_DT_ALIAS+1> DataTypSize {1,1,5,5,0};

    const int   DOMC_UNMAPPED = -2, // indicator for unmapped index pos
                DOMC_EXPAND = -1, // indicator growing index pos
                DOMC_STRICT = 0; // indicator mapped index pos

    class NullBuffer : public std::streambuf {
    public:
        int overflow(int c) override { return c; }
    };
    extern NullBuffer null_buffer;

    const std::string   BADUEL_PREFIX = "?L__",
                        BADStr_PREFIX = "?Str__",
                        strGDXCOMPRESS = "GDXCOMPRESS",
                        strGDXCONVERT = "GDXCONVERT";

    // Uses std::vector of booleans internally
    struct TDFilterBoolVec {
        int FiltNumber, FiltMaxUel;
        std::vector<bool> FiltMap{};
        bool FiltSorted{};

        TDFilterBoolVec(int Nr, int UserHigh) :
            FiltNumber{Nr},
            FiltMaxUel{UserHigh}
        {}

        ~TDFilterBoolVec() = default;

        int64_t MemoryUsed() const {
            return static_cast<int64_t>(FiltMap.capacity());
        }

        bool InFilter(int V) const {
            return V >= 0 && V <= FiltMaxUel && V < (int)FiltMap.size() && FiltMap[V];
        }

        void SetFilter(int ix, bool v) {
            if (ix < 0) return;
            if (ix >= (int)FiltMap.size()) FiltMap.resize(ix + 1);
            FiltMap[ix] = v;
        }
    };

    // Uses gdlib::gmsobj::TBooleanBitArray internally
    struct TDFilterLegacy {
        int FiltNumber{}, FiltMaxUel{};
        gdlib::gmsobj::TBooleanBitArray FiltMap{};
        bool FiltSorted{};

        TDFilterLegacy(int Nr, int UserHigh) :
            FiltNumber{Nr},
            FiltMaxUel{UserHigh}
        {
        }

        ~TDFilterLegacy() = default;

        int MemoryUsed() const {
            return FiltMap.MemoryUsed();
        }

        bool InFilter(int V) const {
            return V >= 0 && V <= FiltMaxUel && FiltMap.GetBit(V);
        }

        void SetFilter(int ix, bool v) {
            FiltMap.SetBit(ix, v);
        }
    };

    using TDFilter = TDFilterLegacy;
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
        bool contains(const TgxFileMode& mode) const override;
        bool empty() const;
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
    class TIntegerMappingLegacy {
        int64_t FCapacity {}, FMapBytes {};
        int64_t FMAXCAPACITY {std::numeric_limits<int>::max() + static_cast<int64_t>(1)};
        int FHighestIndex{};
        int *PMap {};

        void growMapping(int F);
    public:
        TIntegerMappingLegacy() {}
        ~TIntegerMappingLegacy();
        int MemoryUsed() const;
        int GetHighestIndex() const;
        int GetMapping(int F) const;
        void SetMapping(int F, int T);
        int size() const;
        bool empty() const;
    };

    using TIntegerMappingImpl = TIntegerMappingLegacy;

    enum TUELUserMapStatus {map_unknown, map_unsorted, map_sorted, map_sortgrow, map_sortfull};

    class IUELTable {
    protected:
        TUELUserMapStatus FMapToUserStatus {map_unknown};
    public:
        std::unique_ptr<TIntegerMappingImpl> UsrUel2Ent {}; // from user uelnr to table entry
        virtual ~IUELTable() = default;
        virtual int size() const = 0;
        virtual bool empty() const = 0;
        virtual int IndexOf(const char *s) = 0;
        virtual int AddObject(const char *id, size_t idlen, int mapping) = 0;
        virtual int StoreObject(const char *id, size_t idlen, int mapping) = 0;
        virtual const char *operator[](int index) const = 0;
        virtual int GetUserMap(int i) = 0;
        virtual void SetUserMap(int EN, int N) = 0;
        void ResetMapToUserStatus();
        virtual int NewUsrUel(int EN) = 0;
        virtual int AddUsrNew(const char *s, size_t slen) = 0;
        virtual int AddUsrIndxNew(const char *s, size_t slen, int UelNr) = 0;
        virtual TUELUserMapStatus GetMapToUserStatus();
        virtual void RenameEntry(int N, const char *s) = 0;
        virtual int GetMaxUELLength() const = 0;
        virtual int MemoryUsed() const = 0;
        virtual void SaveToStream(gdlib::gmsstrm::TXStreamDelphi &S) = 0;
        virtual void LoadFromStream(gdlib::gmsstrm::TXStreamDelphi &S) = 0;
    };

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

    class TUELTableLegacy : public IUELTable, public TXStrHashListImpl<int> {
    public:
        TUELTableLegacy();
        ~TUELTableLegacy() override = default;
        int size() const override;
        bool empty() const override;
        int GetUserMap(int i) override;
        void SetUserMap(int EN, int N) override;
        int NewUsrUel(int EN) override;
        int AddUsrNew(const char *s, size_t slen) override;
        int AddUsrIndxNew(const char *s, size_t slen, int UelNr) override;
        int GetMaxUELLength() const override;
        int IndexOf(const char *s) override;
        int AddObject(const char *id, size_t idlen, int mapping) override;
        int StoreObject(const char *id, size_t idlen, int mapping) override;
        const char *operator[](int index) const override;
        void RenameEntry(int N, const char *s) override;
        int MemoryUsed() const override;
        void SaveToStream(gdlib::gmsstrm::TXStreamDelphi &S) override;
        void LoadFromStream(gdlib::gmsstrm::TXStreamDelphi &S) override;
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
        int MemoryUsed() const;
        void SaveToStream(gdlib::gmsstrm::TXStreamDelphi &S) const;
    };

    class TAcronymListLegacy {
        gdlib::gmsobj::TXList<TAcronym> FList;
    public:
        TAcronymListLegacy() = default;
        ~TAcronymListLegacy();
        int FindEntry(int Map);
        int FindName(const char *Name);
        int AddEntry(const std::string& Name, const std::string& Text, int Map);
        void CheckEntry(int Map);
        void SaveToStream(gdlib::gmsstrm::TXStreamDelphi& S);
        void LoadFromStream(gdlib::gmsstrm::TXStreamDelphi& S);
        int MemoryUsed();
        int size() const;
        TAcronym &operator[](int Index);
    };

    class TFilterListLegacy {
        gdlib::gmsobj::TXList<TDFilter> FList;
    public:
        TFilterListLegacy() = default;
        ~TFilterListLegacy();
        void AddFilter(TDFilter *F);
        void DeleteFilter(int ix);
        TDFilter *FindFilter(int Nr);
        size_t MemoryUsed() const;
    };

    using TAcronymListImpl = TAcronymListLegacy;
    using TFilterListImpl = TFilterListLegacy;

    using TIntlValueMapDbl = std::array<double, vm_count>;
    using TIntlValueMapI64 = std::array<int64_t, vm_count>;

    using LinkedDataType = gdlib::datastorage::TLinkedDataLegacy<int, double>;
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
    class TGXFileObj : public gdxinterface::GDXInterface {
    public:
        enum class TraceLevels { trl_none, trl_errors, trl_some, trl_all };
    private:
#ifdef YAML
        std::unique_ptr<yaml::TYAMLFile> YFile;
#endif
        bool writeAsYAML{}, writeAsText{};
        std::unique_ptr<gdlib::gmsstrm::TMiBufferedStreamDelphi> FFile;
        TgxFileMode fmode {f_not_open}, fmode_AftReg {f_not_open};
        enum {stat_notopen, stat_read, stat_write} fstatus {stat_notopen};
        int fComprLev{};
        std::unique_ptr<IUELTable> UELTable;
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
        std::unique_ptr<TFilterListImpl> FilterList;
        TDFilter *CurFilter{};
        TDomainList DomainList{};
        bool StoreDomainSets{true};
        TIntlValueMapDbl intlValueMapDbl{}, readIntlValueMapDbl{};
        TIntlValueMapI64 intlValueMapI64{};
        TraceLevels TraceLevel {TraceLevels::trl_none};
        std::string TraceStr;
        int VersionRead{};
        std::string FProducer, FProducer2, FileSystemID;
        int64_t MajorIndexPosition{};
        int64_t NextWritePosition{};
        int DataCount{}, NrMappedAdded{};
        std::array<TgdxElemSize, GLOBAL_MAX_INDEX_DIM> ElemType{};
        std::string MajContext;
        std::array<std::optional<TIntegerMappingImpl>, GLOBAL_MAX_INDEX_DIM> SliceIndxs, SliceRevMap;
        int SliceSyNr{};
        gdxinterface::TgdxStrIndex SliceElems;
        bool    DoUncompress{}, // when reading
                CompressOut{}; // when writing
        int DeltaForWrite{}; // delta for last dimension or first changed dimension
        int DeltaForRead{}; // first position indicating change
        double Zvalacr{}; // tricky
        std::unique_ptr<TAcronymListImpl> AcronymList;
        std::array<TSetBitMap *, GLOBAL_MAX_INDEX_DIM> WrBitMaps{};
        bool ReadUniverse{};
        int UniverseNr{}, UelCntOrig{}; // original uel count when we open the file
        int AutoConvert{1};
        int NextAutoAcronym{};
        bool AppendActive{};

#ifndef VERBOSE_TRACE
        const TraceLevels defaultTraceLevel {TraceLevels::trl_none};
        const bool verboseTrace {false};
#else
        const TraceLevels defaultTraceLevel {TraceLevels::trl_all};
        const bool verboseTrace {true};
#endif

        //api wrapper magic for Fortran
        gdxinterface::TDataStoreFiltProc_t gdxDataReadRawFastFilt_DP{};
        gdxinterface::TDomainIndexProc_t gdxGetDomainElements_DP{};

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

        // ...

        int gdxOpenReadXX(const char *Afn, int filemode, int ReadMode, int &ErrNr);

        // This one is a helper function for a callback from a Fortran client
        void gdxGetDomainElements_DP_FC(int RawIndex, int MappedIndex, void* Uptr);
        int gdxDataReadRawFastFilt_DP_FC(const int* Indx, const double* Vals, void* Uptr);

    public:
        bool    gdxGetDomainElements_DP_CallByRef{},
                gdxDataReadRawFastFilt_DP_CallByRef{},
                gdxDataReadRawFastEx_DP_CallByRef{};

        explicit TGXFileObj(std::string &ErrMsg);
        ~TGXFileObj() override;

        int gdxOpenWrite(const char *FileName, const char *Producer, int &ErrNr) override;
        int gdxOpenWriteEx(const char *FileName, const char *Producer, int Compr, int &ErrNr) override;
        int gdxDataWriteStrStart(const char *SyId, const char *ExplTxt, int Dim, int Typ, int UserInfo) override;
        int gdxDataWriteStr(const char **KeyStr, const double *Values) override;
        int gdxDataWriteDone() override;
        int gdxUELRegisterMapStart() override;
        int gdxUELRegisterMap(int UMap, const char *Uel) override;
        int gdxClose() override;
        int gdxResetSpecialValues() override;
        int gdxErrorStr(int ErrNr, char *ErrMsg) override;
        static int gdxErrorStrStatic(int ErrNr, char *ErrMsg);
        int gdxOpenRead(const char *FileName, int &ErrNr) override;
        int gdxFileVersion(char *FileStr, char *ProduceStr) override;
        int gdxFindSymbol(const char *SyId, int &SyNr) override;
        int gdxDataReadStr(char **KeyStr, double *Values, int &DimFrst) override;
        int gdxDataReadDone() override;
        int gdxSymbolInfo(int SyNr, char *SyId, int &Dim, int &Typ) override;
        int gdxDataReadStrStart(int SyNr, int &NrRecs) override;
        int gdxAddAlias(const char *Id1, const char *Id2) override;
        int gdxAddSetText(const char *Txt, int &TxtNr) override;
        int gdxDataErrorCount() override;
        int gdxDataErrorRecord(int RecNr,  int *KeyInt, double * Values) override;
        int gdxDataErrorRecordX(int RecNr,  int *KeyInt,  double *Values) override;
        int gdxDataReadRaw(int *KeyInt, double *Values, int &DimFrst) override;
        int gdxDataReadRawStart(int SyNr, int &NrRecs) override;
        int gdxDataWriteRaw(const int* KeyInt, const double* Values) override;
        int gdxDataWriteRawStart(const char *SyId, const char *ExplTxt, int Dimen, int Typ,
                                 int UserInfo) override;
        int gdxErrorCount() override;
        int gdxGetElemText(int TxtNr, char *Txt, int &Node) override;
        int gdxGetLastError() override;
        int gdxGetSpecialValues(double *Avals) override;
        int gdxSetSpecialValues(const double *AVals) override;
        int gdxSymbolGetDomain(int SyNr, int *DomainSyNrs) override;
        int gdxSymbolGetDomainX(int SyNr, char **DomainIDs) override;
        int gdxSymbolDim(int SyNr) override;
        int gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt) override;
        int gdxSymbolSetDomain(const char **DomainIDs) override;
        int gdxSymbolSetDomainX(int SyNr, const char **DomainIDs) override;
        int gdxSystemInfo(int &SyCnt, int &UelCnt) override;
        int gdxUELRegisterDone() override;
        int gdxUELRegisterRaw(const char *Uel) override;
        int gdxUELRegisterRawStart() override;
        int gdxUELRegisterStr(const char *Uel, int &UelNr) override;
        int gdxUELRegisterStrStart() override;
        int gdxUMUelGet(int UelNr, char *Uel, int &UelMap) override;
        int gdxUMUelInfo(int &UelCnt, int &HighMap) override;
        int gdxCurrentDim() override;
        int gdxRenameUEL(const char *OldName, const char *NewName) override;
        int gdxOpenReadEx(const char *FileName, int ReadMode, int &ErrNr) override;
        int gdxGetUEL(int uelNr, char *Uel) override;
        int gdxDataWriteMapStart(const char *SyId, const char *ExplTxt, int Dimen, int Typ,
                                 int UserInfo) override;
        int gdxDataWriteMap(const int *KeyInt, const double *Values) override;
        int gdxDataReadMapStart(int SyNr, int &NrRecs) override;
        int gdxDataReadMap(int RecNr, int *KeyInt, double *Values, int &DimFrst) override;

        void SetTraceLevel(TraceLevels tl);

        void SetWriteModes(bool asYAML, bool asText);

        // region Acronym handling
        int gdxAcronymCount() const override;
        int gdxAcronymGetInfo(int N, char *AName, char *Txt, int &AIndx) const override;
        int gdxAcronymSetInfo(int N, const char *AName, const char *Txt, int AIndx) override;
        int gdxAcronymNextNr(int nv) override;
        int gdxAcronymGetMapping(int N, int &orgIndx, int &newIndx, int &autoIndex) override;
        // endregion

        // region Filter handling
        int gdxFilterExists(int FilterNr) override;
        int gdxFilterRegisterStart(int FilterNr) override;
        int gdxFilterRegister(int UelMap) override;
        int gdxFilterRegisterDone() override;
        int gdxDataReadFilteredStart(int SyNr, const int* FilterAction, int& NrRecs) override;
        // endregion

        int gdxSetTextNodeNr(int TxtNr, int Node) override;
        int gdxGetDomainElements(int SyNr, int DimPos, int FilterNr, gdxinterface::TDomainIndexProc_t DP, int& NrElem, void* UPtr) override;
        int gdxSetTraceLevel(int N, const char *s) override;
        int gdxAcronymAdd(const char *AName, const char *Txt, int AIndx) override;
        int gdxAcronymIndex(double V) const override;
        int gdxAcronymName(double V, char *AName) override;
        double gdxAcronymValue(int AIndx) const override;
        int gdxAutoConvert(int nv) override;
        int gdxGetDLLVersion(char *V) const override;
        int gdxFileInfo(int &FileVer, int &ComprLev) const override;
        int gdxDataReadSliceStart(int SyNr, int* ElemCounts) override;
        int gdxDataReadSlice(const char** UelFilterStr, int& Dimen, gdxinterface::TDataStoreProc_t DP) override;
        int gdxDataSliceUELS(const int* SliceKeyInt, char** KeyStr) override;
        int64_t gdxGetMemoryUsed() override;
        int gdxMapValue(double D, int& sv) override;
        int gdxOpenAppend(const char * FileName, const char * Producer, int& ErrNr) override;
        int gdxSetHasText(int SyNr) override;
        int gdxSetReadSpecialValues(const double *AVals) override;
        int gdxSymbIndxMaxLength(int SyNr, int* LengthInfo) override;
        int gdxSymbMaxLength() const override;
        int gdxSymbolAddComment(int SyNr, const char* Txt) override;
        int gdxSymbolGetComment(int SyNr, int N, char *Txt) override;
        int gdxUELMaxLength() override;
        int gdxUMFindUEL(const char *Uel, int& UelNr, int& UelMap) override;
        int gdxStoreDomainSets() override;
        void gdxStoreDomainSetsSet(int x) override;
        int gdxDataReadRawFastFilt(int SyNr, const char **UelFilterStr, gdxinterface::TDataStoreFiltProc_t DP) override;
        int gdxDataReadRawFast(int SyNr, gdxinterface::TDataStoreProc_t DP, int &NrRecs) override;
        int gdxDataReadRawFastEx(int SyNr, gdxinterface::TDataStoreExProc_t DP, int &NrRecs, void *Uptr) override;

        std::string getImplName() const override;
    };

    extern std::string DLLLoadPath; // can be set by loader, so the "dll" knows where it is loaded from

    union uInt64 {
        int64_t i;
        void *p;
    };

    bool IsGoodIdent(std::string_view S);

    int ConvertGDXFile(const std::string &fn, const std::string &MyComp);

}
