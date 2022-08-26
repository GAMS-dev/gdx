#pragma once

namespace global::gmsglobx1 {
    enum tsycSetConstants {
        sycModelTypes,
        sycGamsParameters,
        sycGamsParameterSynonyms,
        sycGamsParameterSynonymMap,
        sycDollarOptions,
        sycGamsFunctions,
        sycSystemSuffixes,
        sycEmpty,
        sycPredefinedSymbols,
        sycGUSSModelAttributes,
        sycSetConstants,
        sycSolverNames,
        sycPlatforms,
        sycVendors,
        sycTLLicense,
        sycComponents,
        sycClipCodes,
        sycGamsLicenses,
        sycGamsLicenseTypes,
        sycComponentSolverMap,
        sycClipComponentMap,
        sycSolverPlatformMap,
        sycSolverTypePlatformMap
    };

    #include "../generated/gmsglobx1.inc.h"
        
    //maxGamsLicenses = 7,
    //maxClipComponentMap = 55,

    const int
        maxSetConstantsLength = 23,
        maxSolverNamesLength = 12,
        maxPlatformsLength = 3,
        maxVendorsLength = 1,
        maxTLLicenseLength = 15,
        maxComponentsLength = 14,
        maxClipCodesLength = 2,
        maxGamsLicensesLength = 2,
        maxGamsLicenseTypesLength = 1,
        maxModelTypesXLength = 6;
}