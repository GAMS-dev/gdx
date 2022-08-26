#pragma once

#include <string>
#include <array>
#include "gmsglobx1.h"

namespace global::gmsglobx2 {
    std::string Hostplatform();

    int SetConstantsLookup(const std::string &s);
    std::string SetConstantsText(int n);
    std::string SetConstantsKey(int n);
    
    int PlatformsLookup(const std::string& s);
    std::string PlatformText();

    extern std::array<std::array<std::string, 2>, global::gmsglobx1::maxPlatforms> platformKeysAndTexts;

    std::string PlatformsText(int n);
    std::string PlatformsText2(int n);
    std::string PlatformsKey(int n);

    std::string SolverNamesKey(int n);
    std::string SolverNamesText(int n);
    int SolverNamesLookup(const std::string &s);

    std::string ModelTypesXKey(int n);

    int SolverTypePlatformMapMap(int i, int j);

    int ComponentsLookup(const std::string& s);

    std::string ComponentsKey(int n);
    std::string ComponentsText(int n);
    int ComponentSolverMapMap(int i, int j);

    int SolverPlatformMapMap(int i, int j);

    std::string GamsLicenseTypesKey(int n);
    std::string GamsLicenseTypesText(int n);

    int GamsLicenseTypesLookup(const std::string& s);

    std::string ClipCodesKey(int n);
    std::string ClipCodesText(int n);

    int VendorsLookup(const std::string& s);
    std::string VendorsKey(int n);
    std::string VendorsText(int n);

    int TLLicenseLookup(const std::string& s);
    std::string TLLicenseText(int n);

}
