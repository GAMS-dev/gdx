#pragma once

#include "gdxinterface.h"
#include <memory>
#include <map>
#include <vector>
#include "global/gmsspecs.h"
#include "gxdefs.h"

namespace gdlib::gmsstrm {
    class TMiBufferedStreamDelphi;
}

namespace gxfile {
    const std::string   BADUEL_PREFIX = "?L__",
                        BADStr_PREFIX = "?Str__",
                        strGDXCOMPRESS = "GDXCOMPRESS",
                        strGDXCONVERT = "GDXCONVERT";

    using PUELIndex = gxdefs::TgdxUELIndex*;

    class TDFilter {
        int FiltNumber, FiltMaxUel;
        std::vector<bool> FiltMap;
        bool FiltSorted;
        // ...
    };

    enum TgdxDAction {
        dm_unmapped,dm_strict,dm_filter,dm_expand
    };

    // ...

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
        fr_slice
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
        // ...
    };

    enum TUELUserMapStatus {map_unknown, map_unsorted, map_sorted, map_sortgrow, map_sortfull};

    // ...

    struct TDomain {
        TDFilter DFilter;
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


    class TUELTable {
        std::map<int, int> UsrUel2Ent;
        std::vector<std::string> uelNames;
        // ...

    public:
        void clear() {
            UsrUel2Ent.clear();
            uelNames.clear();
        }
    };

    class TAcronym {
        // ...
    };

    using TIntlValueMapDbl = std::array<double, 11>;
    using TIntlValueMapI64 = std::array<int64_t, 11>;

    class TGXFileObj : public gdxinterface::GDXInterface {
        std::unique_ptr<gdlib::gmsstrm::TMiBufferedStreamDelphi> FFile;
        TgxFileMode fmode {f_not_open}, fmode_AftReg;
        enum {stat_notopen, stat_read, stat_write} fstatus;
        int fComprLev;
        TUELTable UELTable;
        std::vector<std::string> SetTextList;
        std::vector<int> MapSetText;
        int FCurrentDim;
        global::gmsspecs::TIndex LastElem, PrevElem, MinElem, MaxElem;
        std::vector<std::string> LastStrElem;
        int DataSize;
        global::gmsspecs::tvarvaltype LastDataField;
        std::map<std::string, PgdxSymbRecord> NameList;
        std::vector<std::string> DomainStrList;
        std::map<global::gmsspecs::TIndex, gdxinterface::TgdxValues> SortList, ErrorList;
        PgdxSymbRecord CurSyPtr;
        int ErrCnt, ErrCntTotal;
        int LastError, LastRepError;
        std::vector<TDFilter> FilterList;
        TDFilter CurFilter;
        TDomainList DomainList;
        bool StoreDomainSets{true};
        TIntlValueMapDbl intlValueMapDbl, readIntlValueMapDbl;
        TIntlValueMapI64  intlValueMapI64;
        enum { trl_none, trl_errors, trl_some, trl_all } TraceLevel;
        std::string TraceStr;
        int VersionRead;
        std::string FProducer, FProducer2, FileSystemID;
        int64_t MajorIndexPosition;
        int64_t NextWritePosition;
        int DataCount, NrMappedAdded;
        std::array<TgdxElemSize, global::gmsspecs::MaxDim> ElemType;
        std::string MajContext;
        std::array<TIntegerMapping, global::gmsspecs::MaxDim> SliceIndxs, SliceRevMap;
        int SliceSyNr;
        gxdefs::TgdxStrIndex SliceElems;
        void *ReadPtr;
        bool DoUncompress, CompressOut;
        int DeltaForWrite;
        int DeltaForRead;
        double Zvalacr;
        std::vector<TAcronym> AcronymList;
        std::array<std::vector<bool>, global::gmsspecs::MaxDim> WrBitMaps;
        bool ReadUniverse;
        int UniverseNr, UelCntOrig;
        int AutoConvert{1};
        int NextAutoAcronym{};
        bool AppendActive{};

        void InitErrors();

        int gdxResetSpecialValues();
        bool PrepareSymbolWrite(const std::string &Caller,
                                const std::string &AName,
                                const std::string &AText,
                                int ADim,
                                int AType,
                                int AUserType);

    public:
        TGXFileObj(std::string &ErrMsg);
        ~TGXFileObj();

        int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) override;
        int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) override;
        int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ,
                                 int UserInfo) override;
        int gdxDataWriteStr(const gdxinterface::TgdxStrIndex &KeyStr, const gdxinterface::TgdxValues &Values) override;
        int gdxDataWriteDone() override;

        int gdxClose() override;


    };

    extern std::string DLLLoadPath;

}
