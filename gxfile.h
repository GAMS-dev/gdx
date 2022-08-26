#pragma once

#include "gdxinterface.h"
#include <memory>
#include <map>
#include <vector>
#include "global/gmsspecs.h"

namespace gdlib::gmsstrm {
    class TMiBufferedStreamDelphi;
}

namespace gxfile {
    const std::string   BADUEL_PREFIX = "?L__",
                        BADStr_PREFIX = "?Str__",
                        strGDXCOMPRESS = "GDXCOMPRESS",
                        strGDXCONVERT = "GDXCONVERT";

    // TODO: Also port gxdefs.pas as gxdefs.h
    using PUELIndex = global::gmsspecs::TIndex*;

    enum TgxFileMode {
        f_not_open   ,
        fr_init      ,
        fw_init      ,
        fw_dom_raw   ,
        fw_dom_map   ,
        fw_dom_str   ,
        fw_raw_data  ,
        fw_Map_data  ,
        fw_str_data  ,
        f_raw_elem   ,
        f_Map_elem   ,
        f_str_elem   ,
        fr_raw_data  ,
        fr_Map_data  ,
        fr_MapR_data ,
        fr_str_data  ,
        fr_filter    ,
        fr_slice
    };

    using TgxModeSet = std::set<TgxFileMode>;

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

    enum TUELUserMapStatus {map_unknown, map_unsorted, map_sorted, map_sortgrow, map_sortfull};

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

    class TDFilter {
        int FiltNumber, FiltMaxUel;
        std::vector<bool> FiltMap;
        bool FiltSorted;

        // ...
    };

    using TIntlValueMapDbl = std::array<double, 11>;
    using TIntlValueMapI64 = std::array<int64_t, 11>;

    class TGXFileObj : public gdxinterface::GDXInterface {
        std::unique_ptr<gdlib::gmsstrm::TMiBufferedStreamDelphi> FFile;
        TgxFileMode fmode {f_not_open};
        int ErrCnt, ErrCntTotal;
        int LastError, LastRepError, fComprLev;
        bool CompressOut;
        void *ReadPtr;
        std::string MajContext;
        enum { trl_none, trl_errors, trl_some, trl_all } TraceLevel;
        enum {stat_notopen, stat_read, stat_write} fstatus;
        int AutoConvert{1};

        std::map<std::string, PgdxSymbRecord> NameList;
        std::vector<std::string> DomainStrList;
        TUELTable UELTable;
        std::vector<TAcronym> AcronymList;
        std::vector<TDFilter> FilterList;
        int VersionRead;
        std::string FileSystemID, FProducer, FProducer2;
        int64_t MajorIndexPosition;
        int NextAutoAcronym{};
        bool AppendActive{}, StoreDomainSets{true};
        TIntlValueMapDbl intlValueMapDbl, readIntlValueMapDbl;
        TIntlValueMapI64  intlValueMapI64;
        double Zvalacr;

        int64_t NextWritePosition;
        int FCurrentDim;
        std::vector<std::string> LastStrElem;

        std::vector<std::string> SetTextList;

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
