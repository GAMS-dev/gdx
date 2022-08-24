#pragma once

#include "igdx.h"
#include <memory>
#include <map>
#include "global/gmsspecs.h"

namespace gdlib::gmsstrm {
    class TMiBufferedStreamDelphi;
}

namespace gxfile {
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

    enum TUELUserMapStatus {map_unknown, map_unsorted, map_sorted, map_sortgrow, map_sortfull};

    class TUELTable {
        std::map<int, int> UsrUel2Ent;
        std::vector<std::string> uelNames;
        // ...
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

    class TGXFileObj : public igdx::IGDX {
        std::unique_ptr<gdlib::gmsstrm::TMiBufferedStreamDelphi> FFile;
        TgxFileMode fmode;
        int ErrCnt, ErrCntTotal;
        int LastError, LastRepError, fComprLev;
        bool CompressOut;
        void *ReadPtr;
        std::string MajContext;
        enum { trl_none, trl_errors, trl_some, trl_all } TraceLevel;

        std::map<std::string, PgdxSymbRecord> NameList;
        TUELTable UELTable;
        std::vector<TAcronym> AcronymList;
        std::vector<TDFilter> FilterList;
        int VersionRead;
        std::string FileSystemID, FProducer, FProducer2;
        int64_t MajorIndexPosition;

        void InitErrors();

    public:
        int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) override;
        int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) override;
        int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ,
                                 int UserInfo) override;
        int gdxDataWriteStr(const igdx::TgdxStrIndex &KeyStr, const igdx::TgdxValues &Values) override;
        int gdxDataWriteDone() override;

        int gdxClose() override;
    };

    extern std::string DLLLoadPath;

}
