#include "gdxinterface.h"
#include <vector>

namespace gdxinterface {

    int GDXInterface::gdxDataWriteStr(const std::vector<std::string> &KeyStr, const double *Values) {
        std::vector<std::array<char, 256>> bufs(KeyStr.size());
        for (int i{}; i < (int)KeyStr.size(); i++) {
            std::memcpy(bufs[i].data(), KeyStr[i].c_str(), KeyStr[i].length() + 1);
        }
        return gdxDataWriteStr((const char **) bufs.data(), Values);
    }

}
