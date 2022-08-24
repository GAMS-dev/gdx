#pragma once

#include "igdx.h"
#include <memory>

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

    class TGXFileObj : public igdx::IGDX {
        std::unique_ptr<gdlib::gmsstrm::TMiBufferedStreamDelphi> FFile;
        TgxFileMode fmode;
        int LastError;
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
