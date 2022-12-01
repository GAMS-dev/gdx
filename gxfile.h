#pragma once

// Description:
//  This unit defines the GDX Object as a C++ object.

#include "gdxinterface.h"
#include <memory>
#include <map>
#include <utility>
#include <vector>
#include <optional>
#include <unordered_set>
#include "global/gmsspecs.h"
#include "gxdefs.h"
#include "gdlib/gmsdata.h"
#include "gdlib/datastorage.h"
#include <cstring>

#include "utils.h"

#include "sparsehash/dense_hash_map"
#include "ankerl/unordered_dense.h"

#include "gdlib/strhash.h"

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

    template<typename K, typename V, typename H, typename E>
    //using umaptype = google::dense_hash_map<K, V, H, E>;
    //using umaptype = std::unordered_map<K,V, H, E>;
    using umaptype = ankerl::unordered_dense::map<K, V, H, E>;

    class NullBuffer : public std::streambuf {
    public:
        int overflow(int c) override { return c; }
    };
    extern NullBuffer null_buffer;

    const std::string   BADUEL_PREFIX = "?L__",
                        BADStr_PREFIX = "?Str__",
                        strGDXCOMPRESS = "GDXCOMPRESS",
                        strGDXCONVERT = "GDXCONVERT";

    using PUELIndex = gxdefs::TgdxUELIndex*;

    struct strCompCaseInsensitive {
        bool operator() (const std::string& lhs, const std::string& rhs) const {
            return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };

    struct TDFilter {
        int FiltNumber, FiltMaxUel;
        std::vector<bool> FiltMap{};
        bool FiltSorted{};

        TDFilter(int Nr, int UserHigh) :
            FiltNumber{Nr},
            FiltMaxUel{UserHigh}
        {}

        ~TDFilter() = default;

        int64_t MemoryUsed() const {
            return static_cast<int64_t>(FiltMap.capacity());
        }

        bool InFilter(int V) const {
            return V >= 0 && V <= FiltMaxUel && V < FiltMap.size() && FiltMap[V];
        }

        void SetFilter(int ix, bool v) {
            if (ix < 0) return;
            if (ix >= FiltMap.size()) FiltMap.resize(ix + 1);
            FiltMap[ix] = v;
        }
    };

    class TFilterList : public std::vector<TDFilter> {
    public:
        TDFilter *FindFilter(int Nr);
        void AddFilter(const TDFilter& F);
    };

    enum TgdxDAction {
        dm_unmapped,dm_strict,dm_filter,dm_expand
    };

    struct TDomain {
        TDFilter *DFilter;
        TgdxDAction DAction;
    };

    using TDomainList = std::array<TDomain, global::gmsspecs::MaxDim>;

    struct TgdxSymbRecord {
        int SSyNr;
        // since SSyNr is not set correctly for alias, but we want to mimic original Delphi GDX behavior as closely as possible
        int SSyNrActual;
        int64_t SPosition;
        int SDim, SDataCount, SErrors;
        global::gmsspecs::TgdxDataType SDataType;
        int SUserInfo;
        bool SSetText;
        std::string SExplTxt;
        bool SIsCompressed;
        // TODO: Maybe also use std::optional here instead of std::unique_ptr
        std::unique_ptr<std::vector<int>> SDomSymbols, // real domain info
                                          SDomStrings; //relaxed domain info
        std::vector<std::string> SCommentsList; // TODO: should this also become an optional entry?
        bool SScalarFrst; // not stored
        std::unique_ptr<std::vector<bool>> SSetBitMap; // for 1-dim sets only
    };
    using PgdxSymbRecord = TgdxSymbRecord*;

    enum TgdxIntlValTyp { // values stored internally via the indicator byte
        vm_valund ,
        vm_valna  ,
        vm_valpin ,
        vm_valmin ,
        vm_valeps ,
        vm_zero   ,
        vm_one    ,
        vm_mone   ,
        vm_half   ,
        vm_two    ,
        vm_normal ,
        vm_count
    };

    enum TgxFileMode {
        f_not_open   ,
        fr_init      ,
        fw_init      ,
        fw_dom_raw   ,
        fw_dom_map   ,
        fw_dom_str   ,
        fw_raw_data  ,
        fw_map_data  ,
        fw_str_data  ,
        f_raw_elem   ,
        f_map_elem   ,
        f_str_elem   ,
        fr_raw_data  ,
        fr_map_data  ,
        fr_mapr_data ,
        fr_str_data  ,
        fr_filter    ,
        fr_slice,
        tgxfilemode_count
    };

    using TgxModeSet = std::unordered_set<TgxFileMode>;

    const TgxModeSet    AnyWriteMode {fw_init,fw_dom_raw, fw_dom_map, fw_dom_str, fw_raw_data,fw_map_data,fw_str_data},
                        AnyReadMode {fr_init,fr_raw_data,fr_map_data,fr_mapr_data,fr_str_data};

    enum TgdxElemSize {
        sz_byte,
        sz_word,
        sz_integer
    };

    class TIntegerMapping {
        // FMAXCAPACITY == number of index positions required to store [0..high(integer)]
        int64_t FMAXCAPACITY {std::numeric_limits<int>::max() + static_cast<int64_t>(1)};
        std::vector<int> Map{};
    public:
        int GetHighestIndex() const;
        void SetMapping(int F, int T);
        int GetMapping(int F) const;
        int GetReverseMapping(int T) const;
        int MemoryUsed();
        void clear();
        int &operator[](int index);
        bool empty() const;
        void reserve(int n);
    };

    enum TUELUserMapStatus {map_unknown, map_unsorted, map_sorted, map_sortgrow, map_sortfull};

    struct IndexNumPair {
        int index, num;
        IndexNumPair() : index{}, num{} {}
        explicit IndexNumPair(int _index, int _num) : index(_index), num(_num) {}
        IndexNumPair(int _num) : index{}, num{_num} {}
    };

    //static IndexNumPair unmappedPair {-1};

    struct caseInsensitiveHasher {
        size_t operator()(const std::string& input) const {
            int res{};
            for(char c : input)
                res = 211 * res + std::toupper(c);
            return res & 0x7fffffff;
        }
    };

    struct caseInsensitiveStrEquality {
        bool operator() (const std::string& s1, const std::string& s2) const {
            return utils::sameText(s1, s2);
        }
    };

    using caseSensitiveHasher = ankerl::unordered_dense::hash<std::string>;
    using caseSensitiveStrEquality = std::equal_to<std::string>;

    // FIXME: Does this really reflect what TUELTable in Delphi is doing?

    using utablemaptype = umaptype<std::string, IndexNumPair, caseInsensitiveHasher, caseInsensitiveStrEquality>;

    class IUELTable {
    protected:
        TUELUserMapStatus FMapToUserStatus {map_unknown};
    public:
        TIntegerMapping UsrUel2Ent {}; // from user uelnr to table entry
        virtual ~IUELTable() = default;
        virtual void clear() = 0;
        virtual int size() const = 0;
        virtual bool empty() const = 0;
        virtual int IndexOf(const std::string &s) = 0;
        virtual int AddObject(const std::string &id, int mapping) = 0;
        virtual int StoreObject(const std::string& id, int mapping) = 0;
        virtual std::string operator[](int index) const = 0;
        virtual int GetUserMap(int i) = 0;
        virtual void SetUserMap(int EN, int N) = 0;
        virtual void ResetMapToUserStatus() = 0;
        virtual int NewUsrUel(int EN) = 0;
        virtual int AddUsrNew(const std::string& s) = 0;
        virtual int AddUsrIndxNew(const std::string &s, int UelNr) = 0;
        virtual TUELUserMapStatus GetMapToUserStatus();
        virtual void RenameEntry(int N, const std::string &s) = 0;
        virtual int GetMaxUELLength() const = 0;
        virtual void Reserve(int n) = 0;
    };

    class TUELTable : public IUELTable {
        utablemaptype nameToIndexNum{};
        utablemaptype::iterator nth(int index);
        utablemaptype::const_iterator cnth(int index) const;
    public:
        TUELTable();
        void clear() override;
        int size() const override;
        bool empty() const override;
        int IndexOf(const std::string &s) override;
        int AddObject(const std::string &id, int mapping) override;
        int StoreObject(const std::string& id, int mapping) override;
        std::string operator[](int index) const override;
        int GetUserMap(int i) override;
        void SetUserMap(int EN, int N) override;
        void ResetMapToUserStatus() override;
        int NewUsrUel(int EN) override;
        int AddUsrNew(const std::string& s) override;
        int AddUsrIndxNew(const std::string &s, int UelNr) override;
        void RenameEntry(int N, const std::string &s) override;
        int GetMaxUELLength() const override;
        void Reserve(int n) override;
    };

    class TUELTableLegacy : public IUELTable, public gdlib::strhash::TXStrHashList<IndexNumPair> {
    public:
        TUELTableLegacy();
        void clear() override;
        int size() const override;
        bool empty() const override;
        int GetUserMap(int i) override;
        void SetUserMap(int EN, int N) override;
        void ResetMapToUserStatus() override;
        int NewUsrUel(int EN) override;
        int AddUsrNew(const std::string &s) override;
        int AddUsrIndxNew(const std::string &s, int UelNr) override;
        int GetMaxUELLength() const override;
        void Reserve(int n) override;
        int IndexOf(const std::string &s) override;
        int AddObject(const std::string &id, int mapping) override;
        int StoreObject(const std::string& id, int mapping) override;
        std::string operator[](int index) const override;
        void RenameEntry(int N, const std::string &s) override;
    };

    std::string MakeGoodExplText(const std::string& s);

    struct TAcronym {
        std::string AcrName{}, AcrText{};
        int AcrMap{}, AcrReadMap{};
        bool AcrAutoGen{};

        TAcronym(std::string Name, const std::string& Text, int Map)
            : AcrName{std::move( Name )}, AcrText{ MakeGoodExplText(Text) }, AcrMap{ Map }, AcrReadMap{ -1 }, AcrAutoGen{} {
        }

        TAcronym() = default;
    };

    class TAcronymList : public std::vector<TAcronym> {
    public:
        int FindEntry(int Map) const;
        int FindEntry(const std::string &Name) const;
        int AddEntry(const std::string &Name, const std::string &Text, int Map);

    };

    using TIntlValueMapDbl = std::array<double, vm_count>;
    using TIntlValueMapI64 = std::array<int64_t, vm_count>;

    class TSetTextList : public std::vector<std::string> {
    public:
        std::map<std::string, int, strCompCaseInsensitive> strToNodeNr;
        bool mapContains(const std::string& s);
        int Add(const std::string &s);
    };

    using TDomainIndexProc_t = void(*)(int RawIndex, int MappedIndex, void* Uptr);
    using TDataStoreProc_t = void(*)(const int* Indx, const double* Vals);
    using TDataStoreFiltProc_t = int(*)(const int *Indx, const double *Vals, void *Uptr);

    // Description:
    //    Class for reading and writing gdx files
    class TGXFileObj : public gdxinterface::GDXInterface {
        std::unique_ptr<yaml::TYAMLFile> YFile;
        bool writeAsYAML{}, writeAsText{};
        std::unique_ptr<gdlib::gmsstrm::TMiBufferedStreamDelphi> FFile;
        TgxFileMode fmode {f_not_open}, fmode_AftReg {f_not_open};
        enum {stat_notopen, stat_read, stat_write} fstatus;
        int fComprLev{};
        std::unique_ptr<IUELTable> UELTable;
        std::unique_ptr<TSetTextList> SetTextList {};
        std::vector<int> MapSetText;
        int FCurrentDim{};
        global::gmsspecs::TIndex LastElem, PrevElem, MinElem, MaxElem;
        std::array<std::optional<std::string>, global::gmsspecs::MaxDim> LastStrElem;
        int DataSize{};
        global::gmsspecs::tvarvaltype LastDataField;
        std::map<std::string, PgdxSymbRecord, strCompCaseInsensitive> NameList;
        // symbol names in order of insertion, used for quick iteration
        std::vector<std::string> NameListOrdered;
        std::unique_ptr<std::vector<std::string>> DomainStrList;
        // FIXME: Make sure these match functionality/semantics AND performance of TLinkedData and TTblGamsData
        //std::map<global::gmsspecs::TIndex, gxdefs::TgdxValues> SortList;
        std::unique_ptr<gdlib::datastorage::TLinkedData<gxdefs::TgdxValues>> SortList;
        std::optional<gdlib::datastorage::TLinkedData<gxdefs::TgdxValues>::TLDStorageType::iterator> ReadPtr;
        gdlib::gmsdata::TTblGamsData ErrorList;
        PgdxSymbRecord CurSyPtr{};
        int ErrCnt{}, ErrCntTotal{};
        int LastError{}, LastRepError{};
        TFilterList FilterList;
        TDFilter *CurFilter{};
        TDomainList DomainList;
        bool StoreDomainSets{true};
        TIntlValueMapDbl intlValueMapDbl, readIntlValueMapDbl;
        TIntlValueMapI64  intlValueMapI64;
        enum class TraceLevels { trl_none, trl_errors, trl_some, trl_all } TraceLevel;
        std::string TraceStr;
        int VersionRead{};
        std::string FProducer, FProducer2, FileSystemID;
        int64_t MajorIndexPosition{};
        int64_t NextWritePosition{};
        int DataCount{}, NrMappedAdded{};
        std::array<TgdxElemSize, global::gmsspecs::MaxDim> ElemType;
        std::string MajContext;
        std::array<TIntegerMapping, global::gmsspecs::MaxDim> SliceIndxs, SliceRevMap;
        int SliceSyNr{};
        gxdefs::TgdxStrIndex SliceElems;
        //void *ReadPtr{};
        bool    DoUncompress{}, // when reading
                CompressOut{}; // when writing
        int DeltaForWrite{}; // delta for last dimension or first changed dimension
        int DeltaForRead{}; // first position indicating change
        double Zvalacr{}; // tricky
        TAcronymList AcronymList;
        std::array<std::vector<bool> *, global::gmsspecs::MaxDim> WrBitMaps;
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

        bool PrepareSymbolWrite(const std::string &Caller, const std::string &AName, const std::string &AText, int ADim,
                                int AType, int AUserInfo);

        int PrepareSymbolRead(const std::string &Caller, int SyNr, const int *ADomainNrs, TgxFileMode newmode);

        void InitErrors();
        void SetError(int N);
        void ReportError(int N);
        bool ErrorCondition(bool cnd, int N);
        bool MajorCheckMode(const std::string& Routine, TgxFileMode m);
        bool MajorCheckMode(const std::string &Routine, const TgxModeSet &MS);
        bool CheckMode(const std::string& Routine, TgxFileMode m);
        bool CheckMode(const std::string &Routine, const TgxModeSet &MS);
        void WriteTrace(const std::string &s);
        void InitDoWrite(int NrRecs);
        bool DoWrite(const int *AElements, const double *AVals);
        bool DoRead(double *AVals, int &AFDim);
        void AddToErrorListDomErrs(const std::array<int, global::gmsspecs::MaxDim>& AElements, const double * AVals);
        void AddToErrorList(const int *AElements, const double *AVals);
        void GetDefaultRecord(double *Avals);
        double AcronymRemap(double V);
        bool IsGoodNewSymbol(const std::string &s);
        bool ResultWillBeSorted(const int *ADomainNrs);

        // ...

        int gdxOpenReadXX(const std::string &Afn, int filemode, int ReadMode, int &ErrNr);

        std::optional<std::pair<const std::string, PgdxSymbRecord>> symbolWithIndex(int index);
        int symbolNameToIndex(const std::string &name);

    public:
        explicit TGXFileObj(std::string &ErrMsg);
        ~TGXFileObj() override;

        int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) override;
        int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) override;
        int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ,
                                 int UserInfo) override;
        int gdxDataWriteStr(const char **KeyStr, const double *Values) override;
        int gdxDataWriteDone() override;

        int gdxUELRegisterMapStart() override;

        int gdxUELRegisterMap(int UMap, const std::string &Uel) override;

        int gdxClose() override;

        int gdxResetSpecialValues();

        int gdxErrorStr(int ErrNr, std::string &ErrMsg) override;
        static int gdxErrorStrStatic(int ErrNr, std::string& ErrMsg);

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

        int gdxDataErrorRecord(int RecNr,  int *KeyInt, double * Values) override;

        int gdxDataErrorRecordX(int RecNr,  int *KeyInt,  double *Values) override;

        int gdxDataReadRaw(int *KeyInt, double *Values, int &DimFrst) override;

        int gdxDataReadRawStart(int SyNr, int &NrRecs) override;

        int gdxDataWriteRaw(const int* KeyInt, const double* Values) override;

        int gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                 int UserInfo) override;

        int gdxErrorCount() override;

        int gdxGetElemText(int TxtNr, std::string &Txt, int &Node) override;

        int gdxGetLastError() override;

        int gdxGetSpecialValues(gxdefs::TgdxSVals &Avals) override;

        int gdxSetSpecialValues(const gxdefs::TgdxSVals &AVals) override;

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

        int gdxDataReadMapStart(int SyNr, int &NrRecs) override;

        int gdxDataReadMap(int RecNr, int *KeyInt, double *Values, int &DimFrst) override;

        void SetTraceLevel(TraceLevels tl);

        void SetWriteModes(bool asYAML, bool asText);

        // region Acronym handling
        int gdxAcronymCount() const override;
        int gdxAcronymGetInfo(int N, std::string &AName, std::string &Txt, int &AIndx) const override;
        int gdxAcronymSetInfo(int N, const std::string &AName, const std::string &Txt, int AIndx) override;
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

        int gdxSetTextNodeNr(int TxtNr, int Node);
        int gdxGetDomainElements(int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int& NrElem, void* UPtr);
        int gdxSetTraceLevel(int N, const std::string &s);
        int gdxAcronymAdd(const std::string &AName, const std::string &Txt, int AIndx) override;
        int gdxAcronymIndex(double V) const override;
        int gdxAcronymName(double V, std::string &AName) override;
        double gdxAcronymValue(int AIndx) const override;
        int gdxAutoConvert(int nv);

        int gdxGetDLLVersion(std::string &V) const;
        int gdxFileInfo(int &FileVer, int &ComprLev) const;

        int gdxDataReadSliceStart(int SyNr, int* ElemCounts);
        int gdxDataReadSlice(const char** UelFilterStr, int& Dimen, TDataStoreProc_t DP);
        int gdxDataSliceUELS(const int* SliceKeyInt, char** KeyStr);
        int64_t gdxGetMemoryUsed();
        int gdxMapValue(double D, int& sv);
        int gdxOpenAppend(const std::string& FileName, const std::string& Producer, int& ErrNr) override;
        int gdxSetHasText(int SyNr);
        int gdxSetReadSpecialValues(const std::array<double, 7>& AVals);
        int gdxSymbIndxMaxLength(int SyNr, int* LengthInfo) override;
        int gdxSymbMaxLength();
        int gdxSymbolAddComment(int SyNr, const std::string& Txt) override;
        int gdxSymbolGetComment(int SyNr, int N, std::string& Txt) override;
        int gdxUELMaxLength() override;
        int gdxUMFindUEL(const std::string& Uel, int& UelNr, int& UelMap);
        int gdxStoreDomainSets() override;
        void gdxStoreDomainSetsSet(int x) override;

        int gdxDataReadRawFastFilt(int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP);
        int gdxDataReadRawFast(int SyNr, TDataStoreProc_t DP, int &NrRecs);

    };

    extern std::string DLLLoadPath; // can be set by loader, so the "dll" knows where it is loaded from

    std::string MakeGoodExplText(const std::string &s);
    bool IsGoodIdent(const std::string &S);

}
