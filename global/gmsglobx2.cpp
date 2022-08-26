#include "gmsglobx2.h"
#include "gmsglobx1.h"

#include "../rtl/p3platform.h"
#include "../utils.h"
#include "unit.h"

#include <map>

using namespace std::literals::string_literals;
using namespace global::gmsglobx1;

namespace global::gmsglobx2 {

#include "../generated/gmsglobx2.inc.cpp"

    std::string Hostplatform() {
        const auto pf = rtl::p3platform::OSPlatform();
        const std::map<rtl::p3platform::tOSPlatform, std::string> platformToName = {
                {rtl::p3platform::OSWindows64EMT, "WEX"},
                {rtl::p3platform::OSLinux86_64, "LEX"},
                {rtl::p3platform::OSDarwin_x64, "DEX"}
        };
        return utils::in(pf, platformToName) ? platformToName.at(pf) : "XXX";
    }

    std::string PlatformText() {
        const auto hp = Hostplatform();
        for(const auto &pair : platformKeysAndTexts)
            if(utils::sameText(pair[0], hp)) return pair[1];
        return "Platform text unknown!"s;
    }

    int SetConstantsLookup(const std::string &s) {
        const auto it = std::find_if(SetConstantsKeyS.begin(), SetConstantsKeyS.end(), [&s](const std::string &s2) { return utils::sameText(s, s2); });
        return it != SetConstantsKeyS.end() ? static_cast<int>(it - SetConstantsKeyS.begin()) + 1 : 0;
    }

    std::string SetConstantsKey(int n) {
        return n >= 0 && n < maxSetConstants ? SetConstantsKeyS[n] : ""s;
    }

    const std::string FAILSTR = "**** should never happen"s;

    std::string SetConstantsText(int n) {
        return n >= 0 && n < SetConstantsKeyS.size() ? SetConstantsKeyS[n] : FAILSTR;
    }

    int PlatformsLookup(const std::string& s)
    {
        const auto it = std::find_if(platformKeysAndTexts.begin(), platformKeysAndTexts.end(), [&](const auto& pair) { return utils::sameText(s, pair[0]);  });
        return it != platformKeysAndTexts.end() ? static_cast<int>(it - platformKeysAndTexts.begin()) : -1;
    }

    std::string PlatformsText(int n)
    {
        return n < 0 || n >= platformKeysAndTexts.size() ? FAILSTR : platformKeysAndTexts[n][1];
    }

    std::string PlatformsText2(int n)
    {
        return n < 0 || n >= platformsTexts2.size() ? FAILSTR : platformsTexts2[n];
    }

    std::string PlatformsKey(int n) {
        return n < 0 || n >= platformKeysAndTexts.size() ? FAILSTR : platformKeysAndTexts[n][0];
    }

    std::string SolverNamesKey(int n) {
        return n < 0 || n >= solverNamesKeyS.size() ? FAILSTR : solverNamesKeyS[n];
    }

    std::string SolverNamesText(int n) {
        return n < 0 || n >= solverNamesKeyS.size() ? FAILSTR : solverNamesKeyS[n];
    }

    int SolverNamesLookup(const std::string &s) {
        const auto it = std::find_if(solverNamesKeyS.begin(), solverNamesKeyS.end(), [&](const std::string& s2) { return utils::sameText(s, s2);  });
        return it != solverNamesKeyS.end() ? static_cast<int>(it - solverNamesKeyS.begin()) + 1 : 0;
    }

    std::string ModelTypesXKey(int n) {
        return n < 0 || n >= maxModelTypesX ? FAILSTR : ModelTypesXKeyS[n];
    }

    int SolverTypePlatformMapMap(int i, int j) {
        if (i < 0 || i >= maxSolverTypePlatformMap || j < 0 || j >= 3) return -1;
        return slvTypePlatformMapTriples[i][j];
    }



    int SolverPlatformMapMap(int i, int j) {
        if (i < 0 || i >= maxSolverPlatformMap || j < 0 || j >= 2) return -1;
        const auto& o = SolverPlatformMapTuple[i];
        return o[j];
    }

    std::string ClipCodesKey(int n) {
        return n < 0 || n >= ClipCodesKeyS.size() ? FAILSTR : ClipCodesKeyS[n];
    }

    std::string ClipCodesText(int n) {
        return n < 0 || n >= ClipCodesTextS.size() ? FAILSTR : ClipCodesTextS[n];
    }

    int TLLicenseLookup(const std::string& s) {
        int i{};
        for (const auto & s2 : TLLicenseKeyS) {
            if (utils::sameText(s, s2)) return i;
            i++;
        }
        return -1;
    }

    std::string TLLicenseText(int n) {
        return n < 0 || n >= TLLicenseTextS.size() ? FAILSTR : TLLicenseTextS[n];
    }

    void initialization() {
    }

    void finalization() {

    }

    UNIT_INIT_FINI();
}
