#pragma once

// Description:
//  This unit defines the GDX Object as a C++ object.

#include "gdxinterface.h"
#include <memory>
#include <map>
#include <vector>
#include <optional>
#include "global/gmsspecs.h"
#include "gxdefs.h"
#include "gdlib/gmsdata.h"
#include "gdlib/datastorage.h"
#include <cstring>

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
        std::optional<std::vector<bool>> SSetBitMap; // for 1-dim sets only
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
        vm_normal
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

    using TgxModeSet = std::set<TgxFileMode>;

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
    };

    enum TUELUserMapStatus {map_unknown, map_unsorted, map_sorted, map_sortgrow, map_sortfull};

    // FIXME: Does this really reflect what TUELTable in Delphi is doing?
    class TUELTable {
        std::vector<std::string> uelNames {};
        std::map<std::string, int, strCompCaseInsensitive> nameToNum {};
        // ...
        TUELUserMapStatus FMapToUserStatus {};

    public:
        TIntegerMapping UsrUel2Ent {}; // from user uelnr to table entry

        void clear();

        int size() const;
        bool empty() const;

        const std::vector<std::string> &getNames();

        int IndexOf(const std::string &s) const;

        int AddObject(const std::string &id, int mapping);

        std::string operator[](int index);

        int GetUserMap(int i) const;

        void SetUserMap(int EN, int N);

        void ResetMapToUserStatus();

        int NewUsrUel(int EN);

        int AddUsrNew(const std::string& s);

        int AddUsrIndxNew(const std::string &s, int UelNr);

        TUELUserMapStatus GetMapToUserStatus();

        void RenameEntry(int N, const std::string &s);

        int GetMaxUELLength() const;
    };

    std::string MakeGoodExplText(const std::string& s);

    struct TAcronym {
        std::string AcrName{}, AcrText{};
        int AcrMap{}, AcrReadMap{};
        bool AcrAutoGen{};

        TAcronym(const std::string& Name, const std::string& Text, int Map)
            : AcrName{ Name }, AcrText{ MakeGoodExplText(Text) }, AcrMap{ Map }, AcrReadMap{ -1 }, AcrAutoGen{} {
        }

        TAcronym() {}
    };

    class TAcronymList : public std::vector<TAcronym> {
    public:
        int FindEntry(int Map) const;
        int FindEntry(const std::string &Name) const;
        int AddEntry(const std::string &Name, const std::string &Text, int Map);

    };

    using TIntlValueMapDbl = std::array<double, 11>;
    using TIntlValueMapI64 = std::array<int64_t, 11>;

    class TSetTextList : public std::vector<std::string> {
    public:
        std::map<std::string, int, strCompCaseInsensitive> strToNodeNr;
        bool mapContains(const std::string& s);
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
        TUELTable UELTable;
        std::unique_ptr<TSetTextList> SetTextList {};
        std::vector<int> MapSetText;
        int FCurrentDim{};
        global::gmsspecs::TIndex LastElem, PrevElem, MinElem, MaxElem;
        std::array<std::string, global::gmsspecs::MaxDim> LastStrElem;
        int DataSize{};
        global::gmsspecs::tvarvaltype LastDataField;
        std::map<std::string, PgdxSymbRecord, strCompCaseInsensitive> NameList;
        // symbol names in order of insertion, needed since SSyNr is not set correctly for alias (in add alias)
        std::vector<std::string> NameListOrdered;
        std::vector<std::string> DomainStrList;
        // FIXME: Make sure these match functionality/semantics AND performance of TLinkedData and TTblGamsData
        //std::map<global::gmsspecs::TIndex, gxdefs::TgdxValues> SortList;
        std::unique_ptr<gdlib::datastorage::TLinkedData<gxdefs::TgdxValues>> SortList;
        int ReadPtr;
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
        std::array<std::optional<std::vector<bool>>, global::gmsspecs::MaxDim> WrBitMaps;
        bool ReadUniverse{};
        int UniverseNr{}, UelCntOrig{}; // original uel count when we open the file
        int AutoConvert{1};
        int NextAutoAcronym{};
        bool AppendActive{};

        const TraceLevels defaultTraceLevel {TraceLevels::trl_none};
        const bool verboseTrace {false};

        bool PrepareSymbolWrite(const std::string &Caller, const std::string &AName, const std::string &AText, int ADim,
                                int AType, int AUserInfo);

        int PrepareSymbolRead(const std::string &Caller, int SyNr, const int *ADomainNrs, TgxFileMode newmode);

        void InitErrors();
        void SetError(int N);
        void ReportError(int N);
        bool ErrorCondition(bool cnd, int N);
        bool MajorCheckMode(const std::string &Routine, const TgxModeSet &MS);
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
        int gdxOpenAppend(const std::string& FileName, const std::string& Producer, int& ErrNr);
        int gdxSetHasText(int SyNr);
        int gdxSetReadSpecialValues(const std::array<double, 7>& AVals);
        int gdxSymbIndxMaxLength(int SyNr, int* LengthInfo);
        int gdxSymbMaxLength();
        int gdxSymbolAddComment(int SyNr, const std::string& Txt);
        int gdxSymbolGetComment(int SyNr, int N, std::string& Txt);
        int gdxUELMaxLength();
        int gdxUMFindUEL(const std::string& Uel, int& UelNr, int& UelMap);
        int gdxStoreDomainSets();
        void gdxStoreDomainSetsSet(int x);

        int gdxDataReadRawFastFilt(int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP);
        int gdxDataReadRawFast(int SyNr, TDataStoreProc_t DP, int &NrRecs);

    };

    extern std::string DLLLoadPath; // can be set by loader, so the "dll" knows where it is loaded from

    std::string MakeGoodExplText(const std::string &s);
    bool IsGoodIdent(const std::string &S);

}
