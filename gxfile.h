#pragma once

#include "gdxinterface.h"
#include <memory>
#include <map>
#include <vector>
#include <optional>
#include "global/gmsspecs.h"
#include "gxdefs.h"
#include "gdlib/gmsdata.h"
#include <cstring>

namespace gdlib::gmsstrm {
    class TMiBufferedStreamDelphi;
}

#if defined(_WIN32)
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#endif

namespace gxfile {
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
        std::vector<bool> FiltMap;
        bool FiltSorted;
        // ...

        bool InFilter(int V) const {
            return V >= 0 && V >= FiltMaxUel && FiltMap[V];
        }
    };

    class TFilterList : public std::vector<TDFilter> {
    public:
        TDFilter *FindFilter(int Nr);
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
        std::vector<int> SDomSymbols, SDomStrings;
        std::vector<std::string> SCommentsList;
        bool SScalarFrst;
        std::vector<bool> SSetBitMap;
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

    class TIntegerMapping : public std::map<int, int> {
        // ...
    };

    enum TUELUserMapStatus {map_unknown, map_unsorted, map_sorted, map_sortgrow, map_sortfull};

    class TUELTable {
        std::vector<std::string> uelNames;
        // ...
        TUELUserMapStatus FMapToUserStatus;

    public:
        std::map<int, int> UsrUel2Ent;

        void clear();

        int size() const;

        const std::vector<std::string> &getNames();

        int IndexOf(const std::string &s) const;

        int AddObject(const std::string &id, int mapping);

        std::string operator[](int index) {
            return uelNames[index];
        }

        int GetUserMap(int i) const {
            return UsrUel2Ent.at(i);
        }

        void ResetMapToUserStatus() {
            FMapToUserStatus = map_unknown;
        }

        int NewUsrUel(int EN) {
            auto maxKey = (*UsrUel2Ent.rbegin()).first;
            UsrUel2Ent[maxKey+1] = EN;
            ResetMapToUserStatus();
            return maxKey+1;
        }

        int AddUsrNew(const std::string& s) {
            // ...
            // FIXME: Implement correctly!
            ResetMapToUserStatus();
            return 0;
        }

        TUELUserMapStatus GetMapToUserStatus() {
            if(FMapToUserStatus == map_unknown) {
                int LV {-1};
                bool C {true};
                FMapToUserStatus = map_sortgrow;
                for(int N{}; N<size(); N++) {
                    int V = GetUserMap(N);
                    if(V < 0) C = false;
                    else if(V > LV) {
                        LV = V;
                        if(!C) FMapToUserStatus = map_sorted;
                    } else {
                        FMapToUserStatus = map_unsorted;
                        break;
                    }
                }
                if(FMapToUserStatus == map_sortgrow && C)
                    FMapToUserStatus = map_sortfull;
            }
            return FMapToUserStatus;
        }
    };

    struct TAcronym {
        std::string AcrName, AcrText;
        int AcrMap, AcrReadMap;
        bool AcrAutoGen;

        // ...
    };

    class TAcronymList : public std::vector<TAcronym> {
    public:
        int FindEntry(int Map) const;
        int FindEntry(const std::string &Name) const;
        int AddEntry(const std::string &Name, const std::string &Text, int Map);
    };

    using TIntlValueMapDbl = std::array<double, 11>;
    using TIntlValueMapI64 = std::array<int64_t, 11>;

    class TGXFileObj : public gdxinterface::GDXInterface {
        std::unique_ptr<gdlib::gmsstrm::TMiBufferedStreamDelphi> FFile;
        TgxFileMode fmode {f_not_open}, fmode_AftReg {f_not_open};
        enum {stat_notopen, stat_read, stat_write} fstatus;
        int fComprLev{};
        TUELTable UELTable;
        std::vector<std::string> SetTextList;
        std::vector<int> MapSetText;
        int FCurrentDim{};
        global::gmsspecs::TIndex LastElem, PrevElem, MinElem, MaxElem;
        std::array<std::string, global::gmsspecs::MaxDim> LastStrElem;
        int DataSize{};
        global::gmsspecs::tvarvaltype LastDataField;
        std::map<std::string, PgdxSymbRecord, strCompCaseInsensitive> NameList;
        std::vector<std::string> DomainStrList;
        // FIXME: Make sure these match functionality/semantics AND performance of TLinkedData and TTblGamsData
        std::map<global::gmsspecs::TIndex, gxdefs::TgdxValues> SortList;
        gdlib::gmsdata::TTblGamsData ErrorList;
        PgdxSymbRecord CurSyPtr{};
        int ErrCnt{}, ErrCntTotal{};
        int LastError{}, LastRepError{};
        TFilterList FilterList;
        TDFilter CurFilter;
        TDomainList DomainList;
        bool StoreDomainSets{true};
        TIntlValueMapDbl intlValueMapDbl, readIntlValueMapDbl;
        TIntlValueMapI64  intlValueMapI64;
        enum { trl_none, trl_errors, trl_some, trl_all } TraceLevel;
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
        void *ReadPtr{};
        bool DoUncompress{}, CompressOut{};
        int DeltaForWrite{};
        int DeltaForRead{};
        double Zvalacr{};
        TAcronymList AcronymList;
        std::array<std::vector<bool>, global::gmsspecs::MaxDim> WrBitMaps;
        bool ReadUniverse{};
        int UniverseNr{}, UelCntOrig{};
        int AutoConvert{1};
        int NextAutoAcronym{};
        bool AppendActive{};

        bool PrepareSymbolWrite(const std::string &Caller, const std::string &AName, const std::string &AText, int ADim,
                                int AType, int AUserInfo);

        int PrepareSymbolRead(const std::string &Caller, int SyNr, const gxdefs::TgdxUELIndex &ADomainNrs, TgxFileMode newmode);

        void InitErrors();
        void SetError(int N);
        void ReportError(int N);
        bool ErrorCondition(bool cnd, int N);
        bool MajorCheckMode(const std::string &Routine, const TgxModeSet &MS);
        bool CheckMode(const std::string &Routine, const TgxModeSet &MS);
        void WriteTrace(const std::string &s);
        void InitDoWrite(int NrRecs);
        bool DoWrite(const gxdefs::TgdxUELIndex &AElements, const gxdefs::TgdxValues &AVals);
        bool DoRead(gxdefs::TgdxValues &AVals, int &AFDim);
        void AddToErrorListDomErrs(const gxdefs::TgdxUELIndex &AElements, const gxdefs::TgdxValues &AVals);
        void AddToErrorList(const gxdefs::TgdxUELIndex &AElements, const gxdefs::TgdxValues &AVals);
        void GetDefaultRecord(gxdefs::TgdxValues &Avals);
        double AcronymRemap(double V);
        bool IsGoodNewSymbol(const std::string &s);
        bool ResultWillBeSorted(const gxdefs::TgdxUELIndex &ADomainNrs);

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
        int gdxDataWriteStr(const gxdefs::TgdxStrIndex &KeyStr, const gxdefs::TgdxValues &Values) override;
        int gdxDataWriteDone() override;

        int gdxClose() override;

        int gdxResetSpecialValues();

        int gdxErrorStr(int ErrNr, std::string &ErrMsg) override;

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

    extern std::string DLLLoadPath;

}
