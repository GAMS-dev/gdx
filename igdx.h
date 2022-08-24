#pragma once

#include <string>
#include <array>

namespace igdx {
    const int MaxDim = 20;
    using TgdxValues = std::array<double, 5>;
    using TgdxStrIndex = std::array<std::string, MaxDim>;

    class IGDX {
    public:
        virtual ~IGDX() = default;
        virtual int gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) = 0;
        virtual int gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) = 0;
        virtual int gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Typ, int UserInfo) = 0;
        virtual int gdxDataWriteStr(const TgdxStrIndex &KeyStr, const TgdxValues &Values) = 0;
        virtual int gdxDataWriteDone() = 0;
    };

}