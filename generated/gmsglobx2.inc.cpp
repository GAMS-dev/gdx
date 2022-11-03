std::array<std::array<std::string, 2>, maxPlatforms> platformKeysAndTexts = {
    std::array<std::string, 2> {"WEX"s,"x86 64bit MS Windows"s},
    {"LEX"s,"x86 64bit Linux"s},
    {"DEX"s,"x86 64bit macOS"s},
    {"GEN"s,"Generic platforms"s}
};

const std::array<std::string, maxSetConstants> SetConstantsKeyS = {
    "ModelTypes"s,
    "GamsParameters"s,
    "GamsParameterSynonyms"s,
    "GamsParameterSynonymMap"s,
    "DollarOptions"s,
    "GamsFunctions"s,
    "SystemSuffixes"s,
    "Empty"s,
    "PredefinedSymbols"s,
    "GUSSModelAttributes"s,
    "SetConstants"s,
    "SolverNames"s,
    "Platforms"s,
    "Vendors"s,
    "TLLicense"s,
    "Components"s,
    "ClipCodes"s,
    "GamsLicenses"s,
    "GamsLicenseTypes"s,
    "ComponentSolverMap"s,
    "ClipComponentMap"s,
    "SolverPlatformMap"s,
    "SolverTypePlatformMap"s
};

const std::array<std::string, maxSolverNames> solverNamesKeyS = {
    "DE"s,
    "ALPHAECP"s,
    "ANTIGONE"s,
    "BARON"s,
    "CBC"s,
    "CONOPTD"s,
    "CONOPT4"s,
    "CONVERTOLD"s,
    "CONVERT"s,
    "COPT"s,
    "CPLEXCLASSIC"s,
    "CPLEX"s,
    "CPOPTIMIZER"s,
    "DECIS"s,
    "DECISC"s,
    "DECISM"s,
    "DICOPT"s,
    "EXAMINER"s,
    "EXAMINER2"s,
    "GAMSCHK"s,
    "GUROBI"s,
    "SCENSOLVER"s,
    "HIGHS"s,
    "IPOPT"s,
    "JAMS"s,
    "KESTREL"s,
    "KNITRO"s,
    "LINDO"s,
    "LINDOGLOBAL"s,
    "MILES"s,
    "MINOS55"s,
    "MOSEK"s,
    "NLPEC"s,
    "OCTERACT"s,
    "ODHCPLEX"s,
    "OsiCplex"s,
    "OsiGurobi"s,
    "OsiMosek"s,
    "OsiXpress"s,
    "PATHNLP"s,
    "PATHC"s,
    "PATHVI"s,
    "QUADMINOS"s,
    "SBB"s,
    "SCIP"s,
    "SELKIE"s,
    "SHOT"s,
    "SNOPT"s,
    "Soplex"s,
    "XPRESS"s,
    "ASK"s,
    "CHK4UPD"s,
    "CHOLESKY"s,
    "CSV2GDX"s,
    "EIGENVALUE"s,
    "EIGENVECTOR"s,
    "ENDECRYPT"s,
    "FINDTHISGAMS"s,
    "GAMSIDE"s,
    "GDX2ACCESS"s,
    "GDX2SQLITE"s,
    "GDX2VEDA"s,
    "GDX2XLS"s,
    "GDXCOPY"s,
    "GDXDIFF"s,
    "GDXDUMP"s,
    "GDXMERGE"s,
    "GDXMRW"s,
    "GDXRANK"s,
    "GDXRENAME"s,
    "GDXRRW"s,
    "GDXTROLL"s,
    "GDXVIEWER"s,
    "GDXXRW"s,
    "GMSPYTHON"s,
    "GMSZIP"s,
    "GMSZLIB"s,
    "HARUTILITIES"s,
    "INVERT"s,
    "MCFILTER"s,
    "MDB2GMS"s,
    "MSGRWIN"s,
    "MODEL2TEX"s,
    "MPS2GMS"s,
    "MPSGE"s,
    "MSAPPAVAIL"s,
    "SCENRED"s,
    "SCENRED2"s,
    "SHELLEXECUTE"s,
    "SQL2GMS"s,
    "STUDIO"s,
    "Tools"s,
    "XLS2GMS"s,
    "XLSDUMP"s,
    "XLSTALK"s,
    "GAMS"s,
    "Documents"s,
    "GRID"s,
    "LSTLIB"s,
    "SECURE"s,
    "MIRO"s,
    "ModelLibrary"s,
    "TestLibrary"s,
    "DataLibrary"s,
    "FinanceLib"s,
    "EMPLibrary"s,
    "APILibrary"s,
    "NonlinearLib"s,
    "PSOptLibrary"s,
    "gdxAPI"s,
    "dctAPI"s,
    "gmdAPI"s,
    "gucAPI"s,
    "joatAPI"s,
    "optionAPI"s,
    "gamsAPI"s,
    "idxgdxAPI"s,
    "gamsCPP"s,
    "gamsDotNet"s,
    "gamsPython"s,
    "gamsJava"s,
    "matlabapi"s,
    "IPOPTH"s,
    "CONOPT"s,
    "CONOPT3"s,
    "CONVERTD"s,
    "COINCBC"s,
    "COINIPOPT"s,
    "COINSCIP"s,
    "CPLEXD"s,
    "GLOMIQO"s,
    "LOGMIP"s,
    "MILESE"s,
    "MINOS"s,
    "MINOS5"s,
    "PATH"s,
    "OSISOPLEX"s
};

const std::array<std::string, maxPlatforms> platformsTexts2 = {
    "x86 64bit MS Windows"s,
    "x86 64bit Linux"s,
    "x86 64bit macOS"s,
    "Generic platforms"s
};

const std::array<std::string, maxModelTypesX> ModelTypesXKeyS = {
    "NONE"s,
    "LP"s,
    "MIP"s,
    "RMIP"s,
    "NLP"s,
    "MCP"s,
    "MPEC"s,
    "RMPEC"s,
    "CNS"s,
    "DNLP"s,
    "RMINLP"s,
    "MINLP"s,
    "QCP"s,
    "MIQCP"s,
    "RMIQCP"s,
    "EMP"s
};

const std::array<std::array<int, 3>, maxSolverTypePlatformMap> slvTypePlatformMapTriples = {
    std::array<int, 3> {1,16,1},
    {1,16,2},
    {1,16,3},
    {2,12,1},
    {2,12,2},
    {2,12,3},
    {2,14,1},
    {2,14,2},
    {2,14,3},
    {3,5,1},
    {3,5,2},
    {3,5,3},
    {3,9,1},
    {3,9,2},
    {3,9,3},
    {3,10,1},
    {3,10,2},
    {3,10,3},
    {3,11,1},
    {3,11,2},
    {3,11,3},
    {3,12,1},
    {3,12,2},
    {3,12,3},
    {3,13,1},
    {3,13,2},
    {3,13,3},
    {3,14,1},
    {3,14,2},
    {3,14,3},
    {3,15,1},
    {3,15,2},
    {3,15,3},
    {4,2,1},
    {4,2,2},
    {4,2,3},
    {4,3,1},
    {4,3,2},
    {4,3,3},
    {4,4,1},
    {4,4,2},
    {4,4,3},
    {4,5,1},
    {4,5,2},
    {4,5,3},
    {4,9,1},
    {4,9,2},
    {4,9,3},
    {4,10,1},
    {4,10,2},
    {4,10,3},
    {4,11,1},
    {4,11,2},
    {4,11,3},
    {4,12,1},
    {4,12,2},
    {4,12,3},
    {4,13,1},
    {4,13,2},
    {4,13,3},
    {4,14,1},
    {4,14,2},
    {4,14,3},
    {4,15,1},
    {4,15,2},
    {4,15,3},
    {5,2,1},
    {5,2,2},
    {5,2,3},
    {5,3,1},
    {5,3,2},
    {5,3,3},
    {5,4,1},
    {5,4,2},
    {5,4,3},
    {6,2,1},
    {6,2,2},
    {6,2,3},
    {6,4,1},
    {6,4,2},
    {6,4,3},
    {6,5,1},
    {6,5,2},
    {6,5,3},
    {6,9,1},
    {6,9,2},
    {6,9,3},
    {6,10,1},
    {6,10,2},
    {6,10,3},
    {6,11,1},
    {6,11,2},
    {6,11,3},
    {6,13,1},
    {6,13,2},
    {6,13,3},
    {6,15,1},
    {6,15,2},
    {6,15,3},
    {7,2,1},
    {7,2,2},
    {7,2,3},
    {7,4,1},
    {7,4,2},
    {7,4,3},
    {7,5,1},
    {7,5,2},
    {7,5,3},
    {7,9,1},
    {7,9,2},
    {7,9,3},
    {7,10,1},
    {7,10,2},
    {7,10,3},
    {7,11,1},
    {7,11,2},
    {7,11,3},
    {7,13,1},
    {7,13,2},
    {7,13,3},
    {7,15,1},
    {7,15,2},
    {7,15,3},
    {8,2,1},
    {8,2,2},
    {8,2,3},
    {8,3,1},
    {8,3,2},
    {8,3,3},
    {8,4,1},
    {8,4,2},
    {8,4,3},
    {8,5,1},
    {8,5,2},
    {8,5,3},
    {8,6,1},
    {8,6,2},
    {8,6,3},
    {8,7,1},
    {8,7,2},
    {8,7,3},
    {8,8,1},
    {8,8,2},
    {8,8,3},
    {8,9,1},
    {8,9,2},
    {8,9,3},
    {8,10,1},
    {8,10,2},
    {8,10,3},
    {8,11,1},
    {8,11,2},
    {8,11,3},
    {8,12,1},
    {8,12,2},
    {8,12,3},
    {8,13,1},
    {8,13,2},
    {8,13,3},
    {8,14,1},
    {8,14,2},
    {8,14,3},
    {8,15,1},
    {8,15,2},
    {8,15,3},
    {9,2,1},
    {9,2,2},
    {9,2,3},
    {9,3,1},
    {9,3,2},
    {9,3,3},
    {9,4,1},
    {9,4,2},
    {9,4,3},
    {9,5,1},
    {9,5,2},
    {9,5,3},
    {9,6,1},
    {9,6,2},
    {9,6,3},
    {9,7,1},
    {9,7,2},
    {9,7,3},
    {9,8,1},
    {9,8,2},
    {9,8,3},
    {9,9,1},
    {9,9,2},
    {9,9,3},
    {9,10,1},
    {9,10,2},
    {9,10,3},
    {9,11,1},
    {9,11,2},
    {9,11,3},
    {9,12,1},
    {9,12,2},
    {9,12,3},
    {9,13,1},
    {9,13,2},
    {9,13,3},
    {9,14,1},
    {9,14,2},
    {9,14,3},
    {9,15,1},
    {9,15,2},
    {9,15,3},
    {9,16,1},
    {9,16,2},
    {9,16,3},
    {10,2,1},
    {10,2,2},
    {10,2,3},
    {10,3,1},
    {10,3,2},
    {10,3,3},
    {10,4,1},
    {10,4,2},
    {10,4,3},
    {10,13,1},
    {10,13,2},
    {10,13,3},
    {10,15,1},
    {10,15,2},
    {10,15,3},
    {11,2,1},
    {11,2,2},
    {11,2,3},
    {11,3,1},
    {11,3,2},
    {11,3,3},
    {11,4,1},
    {11,4,2},
    {11,4,3},
    {11,13,1},
    {11,13,2},
    {11,13,3},
    {11,14,1},
    {11,14,2},
    {11,14,3},
    {11,15,1},
    {11,15,2},
    {11,15,3},
    {12,2,1},
    {12,2,2},
    {12,2,3},
    {12,3,1},
    {12,3,2},
    {12,3,3},
    {12,4,1},
    {12,4,2},
    {12,4,3},
    {12,13,1},
    {12,13,2},
    {12,13,3},
    {12,14,1},
    {12,14,2},
    {12,14,3},
    {12,15,1},
    {12,15,2},
    {12,15,3},
    {13,3,1},
    {13,3,2},
    {13,3,3},
    {13,12,1},
    {13,12,2},
    {13,12,3},
    {13,14,1},
    {13,14,2},
    {13,14,3},
    {14,16,1},
    {14,16,2},
    {14,16,3},
    {15,2,1},
    {15,2,2},
    {15,2,3},
    {16,2,1},
    {16,2,2},
    {16,2,3},
    {17,12,1},
    {17,12,2},
    {17,12,3},
    {17,14,1},
    {17,14,2},
    {17,14,3},
    {18,2,1},
    {18,2,2},
    {18,2,3},
    {18,3,1},
    {18,3,2},
    {18,3,3},
    {18,4,1},
    {18,4,2},
    {18,4,3},
    {18,5,1},
    {18,5,2},
    {18,5,3},
    {18,6,1},
    {18,6,2},
    {18,6,3},
    {18,7,1},
    {18,7,2},
    {18,7,3},
    {18,8,1},
    {18,8,2},
    {18,8,3},
    {18,10,1},
    {18,10,2},
    {18,10,3},
    {18,11,1},
    {18,11,2},
    {18,11,3},
    {18,12,1},
    {18,12,2},
    {18,12,3},
    {18,13,1},
    {18,13,2},
    {18,13,3},
    {18,14,1},
    {18,14,2},
    {18,14,3},
    {18,15,1},
    {18,15,2},
    {18,15,3},
    {19,2,1},
    {19,2,2},
    {19,2,3},
    {19,3,1},
    {19,3,2},
    {19,3,3},
    {19,4,1},
    {19,4,2},
    {19,4,3},
    {19,5,1},
    {19,5,2},
    {19,5,3},
    {19,6,1},
    {19,6,2},
    {19,6,3},
    {19,10,1},
    {19,10,2},
    {19,10,3},
    {19,11,1},
    {19,11,2},
    {19,11,3},
    {19,12,1},
    {19,12,2},
    {19,12,3},
    {19,13,1},
    {19,13,2},
    {19,13,3},
    {19,14,1},
    {19,14,2},
    {19,14,3},
    {19,15,1},
    {19,15,2},
    {19,15,3},
    {20,2,1},
    {20,2,2},
    {20,2,3},
    {20,3,1},
    {20,3,2},
    {20,3,3},
    {20,4,1},
    {20,4,2},
    {20,4,3},
    {20,5,1},
    {20,5,2},
    {20,5,3},
    {20,6,1},
    {20,6,2},
    {20,6,3},
    {20,10,1},
    {20,10,2},
    {20,10,3},
    {20,11,1},
    {20,11,2},
    {20,11,3},
    {20,12,1},
    {20,12,2},
    {20,12,3},
    {20,13,1},
    {20,13,2},
    {20,13,3},
    {20,14,1},
    {20,14,2},
    {20,14,3},
    {20,15,1},
    {20,15,2},
    {20,15,3},
    {21,2,1},
    {21,2,2},
    {21,2,3},
    {21,3,1},
    {21,3,2},
    {21,3,3},
    {21,4,1},
    {21,4,2},
    {21,4,3},
    {21,5,1},
    {21,5,2},
    {21,5,3},
    {21,10,1},
    {21,10,2},
    {21,10,3},
    {21,11,1},
    {21,11,2},
    {21,11,3},
    {21,12,1},
    {21,12,2},
    {21,12,3},
    {21,13,1},
    {21,13,2},
    {21,13,3},
    {21,14,1},
    {21,14,2},
    {21,14,3},
    {21,15,1},
    {21,15,2},
    {21,15,3},
    {22,2,1},
    {22,2,2},
    {22,2,3},
    {22,3,1},
    {22,3,2},
    {22,3,3},
    {22,4,1},
    {22,4,2},
    {22,4,3},
    {22,5,1},
    {22,5,2},
    {22,5,3},
    {22,6,1},
    {22,6,2},
    {22,6,3},
    {22,9,1},
    {22,9,2},
    {22,9,3},
    {22,10,1},
    {22,10,2},
    {22,10,3},
    {22,11,1},
    {22,11,2},
    {22,11,3},
    {22,12,1},
    {22,12,2},
    {22,12,3},
    {22,13,1},
    {22,13,2},
    {22,13,3},
    {22,14,1},
    {22,14,2},
    {22,14,3},
    {22,15,1},
    {22,15,2},
    {22,15,3},
    {23,2,1},
    {23,2,2},
    {23,2,3},
    {23,3,1},
    {23,3,2},
    {23,3,3},
    {23,4,1},
    {23,4,2},
    {23,4,3},
    {24,2,1},
    {24,2,2},
    {24,2,3},
    {24,4,1},
    {24,4,2},
    {24,4,3},
    {24,5,1},
    {24,5,2},
    {24,5,3},
    {24,9,1},
    {24,9,2},
    {24,9,3},
    {24,10,1},
    {24,10,2},
    {24,10,3},
    {24,11,1},
    {24,11,2},
    {24,11,3},
    {24,13,1},
    {24,13,2},
    {24,13,3},
    {24,15,1},
    {24,15,2},
    {24,15,3},
    {25,16,1},
    {25,16,2},
    {25,16,3},
    {26,2,1},
    {26,2,2},
    {26,2,3},
    {26,3,1},
    {26,3,2},
    {26,3,3},
    {26,4,1},
    {26,4,2},
    {26,4,3},
    {26,5,1},
    {26,5,2},
    {26,5,3},
    {26,6,1},
    {26,6,2},
    {26,6,3},
    {26,7,1},
    {26,7,2},
    {26,7,3},
    {26,8,1},
    {26,8,2},
    {26,8,3},
    {26,9,1},
    {26,9,2},
    {26,9,3},
    {26,10,1},
    {26,10,2},
    {26,10,3},
    {26,11,1},
    {26,11,2},
    {26,11,3},
    {26,12,1},
    {26,12,2},
    {26,12,3},
    {26,13,1},
    {26,13,2},
    {26,13,3},
    {26,14,1},
    {26,14,2},
    {26,14,3},
    {26,15,1},
    {26,15,2},
    {26,15,3},
    {26,16,1},
    {26,16,2},
    {26,16,3},
    {27,2,1},
    {27,2,2},
    {27,2,3},
    {27,4,1},
    {27,4,2},
    {27,4,3},
    {27,5,1},
    {27,5,2},
    {27,5,3},
    {27,6,1},
    {27,6,2},
    {27,6,3},
    {27,7,1},
    {27,7,2},
    {27,7,3},
    {27,8,1},
    {27,8,2},
    {27,8,3},
    {27,9,1},
    {27,9,2},
    {27,9,3},
    {27,10,1},
    {27,10,2},
    {27,10,3},
    {27,11,1},
    {27,11,2},
    {27,11,3},
    {27,12,1},
    {27,12,2},
    {27,12,3},
    {27,13,1},
    {27,13,2},
    {27,13,3},
    {27,14,1},
    {27,14,2},
    {27,14,3},
    {27,15,1},
    {27,15,2},
    {27,15,3},
    {28,2,1},
    {28,2,2},
    {28,2,3},
    {28,3,1},
    {28,3,2},
    {28,3,3},
    {28,4,1},
    {28,4,2},
    {28,4,3},
    {28,5,1},
    {28,5,2},
    {28,5,3},
    {28,10,1},
    {28,10,2},
    {28,10,3},
    {28,11,1},
    {28,11,2},
    {28,11,3},
    {28,12,1},
    {28,12,2},
    {28,12,3},
    {28,13,1},
    {28,13,2},
    {28,13,3},
    {28,14,1},
    {28,14,2},
    {28,14,3},
    {28,15,1},
    {28,15,2},
    {28,15,3},
    {28,16,1},
    {28,16,2},
    {28,16,3},
    {29,2,1},
    {29,2,2},
    {29,2,3},
    {29,3,1},
    {29,3,2},
    {29,3,3},
    {29,4,1},
    {29,4,2},
    {29,4,3},
    {29,5,1},
    {29,5,2},
    {29,5,3},
    {29,10,1},
    {29,10,2},
    {29,10,3},
    {29,11,1},
    {29,11,2},
    {29,11,3},
    {29,12,1},
    {29,12,2},
    {29,12,3},
    {29,13,1},
    {29,13,2},
    {29,13,3},
    {29,14,1},
    {29,14,2},
    {29,14,3},
    {29,15,1},
    {29,15,2},
    {29,15,3},
    {30,6,1},
    {30,6,2},
    {30,6,3},
    {31,2,1},
    {31,2,2},
    {31,2,3},
    {31,4,1},
    {31,4,2},
    {31,4,3},
    {31,5,1},
    {31,5,2},
    {31,5,3},
    {31,9,1},
    {31,9,2},
    {31,9,3},
    {31,10,1},
    {31,10,2},
    {31,10,3},
    {31,11,1},
    {31,11,2},
    {31,11,3},
    {31,13,1},
    {31,13,2},
    {31,13,3},
    {31,15,1},
    {31,15,2},
    {31,15,3},
    {32,2,1},
    {32,2,2},
    {32,2,3},
    {32,3,1},
    {32,3,2},
    {32,3,3},
    {32,4,1},
    {32,4,2},
    {32,4,3},
    {32,5,1},
    {32,5,2},
    {32,5,3},
    {32,10,1},
    {32,10,2},
    {32,10,3},
    {32,11,1},
    {32,11,2},
    {32,11,3},
    {32,12,1},
    {32,12,2},
    {32,12,3},
    {32,13,1},
    {32,13,2},
    {32,13,3},
    {32,14,1},
    {32,14,2},
    {32,14,3},
    {32,15,1},
    {32,15,2},
    {32,15,3},
    {33,6,1},
    {33,6,2},
    {33,6,3},
    {33,7,1},
    {33,7,2},
    {33,7,3},
    {33,8,1},
    {33,8,2},
    {33,8,3},
    {34,5,1},
    {34,5,2},
    {34,10,1},
    {34,10,2},
    {34,11,1},
    {34,11,2},
    {34,12,1},
    {34,12,2},
    {34,13,1},
    {34,13,2},
    {34,14,1},
    {34,14,2},
    {34,15,1},
    {34,15,2},
    {35,3,1},
    {35,3,2},
    {35,14,1},
    {35,14,2},
    {36,2,1},
    {36,2,2},
    {36,2,3},
    {36,3,1},
    {36,3,2},
    {36,3,3},
    {36,4,1},
    {36,4,2},
    {36,4,3},
    {37,2,1},
    {37,2,2},
    {37,2,3},
    {37,3,1},
    {37,3,2},
    {37,3,3},
    {37,4,1},
    {37,4,2},
    {37,4,3},
    {38,2,1},
    {38,2,2},
    {38,2,3},
    {38,3,1},
    {38,3,2},
    {38,3,3},
    {38,4,1},
    {38,4,2},
    {38,4,3},
    {39,2,1},
    {39,2,2},
    {39,2,3},
    {39,3,1},
    {39,3,2},
    {39,3,3},
    {39,4,1},
    {39,4,2},
    {39,4,3},
    {40,2,1},
    {40,2,2},
    {40,2,3},
    {40,4,1},
    {40,4,2},
    {40,4,3},
    {40,5,1},
    {40,5,2},
    {40,5,3},
    {40,10,1},
    {40,10,2},
    {40,10,3},
    {40,11,1},
    {40,11,2},
    {40,11,3},
    {40,13,1},
    {40,13,2},
    {40,13,3},
    {40,15,1},
    {40,15,2},
    {40,15,3},
    {41,6,1},
    {41,6,2},
    {41,6,3},
    {41,9,1},
    {41,9,2},
    {41,9,3},
    {43,2,1},
    {43,2,2},
    {43,2,3},
    {43,4,1},
    {43,4,2},
    {43,4,3},
    {44,12,1},
    {44,12,2},
    {44,12,3},
    {44,14,1},
    {44,14,2},
    {44,14,3},
    {45,3,1},
    {45,3,2},
    {45,3,3},
    {45,5,1},
    {45,5,2},
    {45,5,3},
    {45,9,1},
    {45,9,2},
    {45,9,3},
    {45,10,1},
    {45,10,2},
    {45,10,3},
    {45,11,1},
    {45,11,2},
    {45,11,3},
    {45,12,1},
    {45,12,2},
    {45,12,3},
    {45,13,1},
    {45,13,2},
    {45,13,3},
    {45,14,1},
    {45,14,2},
    {45,14,3},
    {45,15,1},
    {45,15,2},
    {45,15,3},
    {46,16,1},
    {46,16,2},
    {46,16,3},
    {47,12,1},
    {47,12,2},
    {47,12,3},
    {47,14,1},
    {47,14,2},
    {47,14,3},
    {48,2,1},
    {48,2,2},
    {48,2,3},
    {48,4,1},
    {48,4,2},
    {48,4,3},
    {48,5,1},
    {48,5,2},
    {48,5,3},
    {48,9,1},
    {48,9,2},
    {48,9,3},
    {48,10,1},
    {48,10,2},
    {48,10,3},
    {48,11,1},
    {48,11,2},
    {48,11,3},
    {48,13,1},
    {48,13,2},
    {48,13,3},
    {48,15,1},
    {48,15,2},
    {48,15,3},
    {49,2,1},
    {49,2,2},
    {49,2,3},
    {49,4,1},
    {49,4,2},
    {49,4,3},
    {50,2,1},
    {50,2,2},
    {50,2,3},
    {50,3,1},
    {50,3,2},
    {50,3,3},
    {50,4,1},
    {50,4,2},
    {50,4,3},
    {50,5,1},
    {50,5,2},
    {50,5,3},
    {50,9,1},
    {50,9,2},
    {50,9,3},
    {50,10,1},
    {50,10,2},
    {50,10,3},
    {50,11,1},
    {50,11,2},
    {50,11,3},
    {50,12,1},
    {50,12,2},
    {50,12,3},
    {50,13,1},
    {50,13,2},
    {50,13,3},
    {50,14,1},
    {50,14,2},
    {50,14,3},
    {50,15,1},
    {50,15,2},
    {50,15,3},
    {123,2,1},
    {123,2,2},
    {123,2,3},
    {123,4,1},
    {123,4,2},
    {123,4,3},
    {123,5,1},
    {123,5,2},
    {123,5,3},
    {123,9,1},
    {123,9,2},
    {123,9,3},
    {123,10,1},
    {123,10,2},
    {123,10,3},
    {123,11,1},
    {123,11,2},
    {123,11,3},
    {123,13,1},
    {123,13,2},
    {123,13,3},
    {123,15,1},
    {123,15,2},
    {123,15,3},
    {124,2,1},
    {124,2,2},
    {124,2,3},
    {124,4,1},
    {124,4,2},
    {124,4,3},
    {124,5,1},
    {124,5,2},
    {124,5,3},
    {124,9,1},
    {124,9,2},
    {124,9,3},
    {124,10,1},
    {124,10,2},
    {124,10,3},
    {124,11,1},
    {124,11,2},
    {124,11,3},
    {124,13,1},
    {124,13,2},
    {124,13,3},
    {124,15,1},
    {124,15,2},
    {124,15,3},
    {125,2,1},
    {125,2,2},
    {125,2,3},
    {125,4,1},
    {125,4,2},
    {125,4,3},
    {125,5,1},
    {125,5,2},
    {125,5,3},
    {125,9,1},
    {125,9,2},
    {125,9,3},
    {125,10,1},
    {125,10,2},
    {125,10,3},
    {125,11,1},
    {125,11,2},
    {125,11,3},
    {125,13,1},
    {125,13,2},
    {125,13,3},
    {125,15,1},
    {125,15,2},
    {125,15,3},
    {126,2,1},
    {126,2,2},
    {126,2,3},
    {126,3,1},
    {126,3,2},
    {126,3,3},
    {126,4,1},
    {126,4,2},
    {126,4,3},
    {126,5,1},
    {126,5,2},
    {126,5,3},
    {126,6,1},
    {126,6,2},
    {126,6,3},
    {126,7,1},
    {126,7,2},
    {126,7,3},
    {126,8,1},
    {126,8,2},
    {126,8,3},
    {126,9,1},
    {126,9,2},
    {126,9,3},
    {126,10,1},
    {126,10,2},
    {126,10,3},
    {126,11,1},
    {126,11,2},
    {126,11,3},
    {126,12,1},
    {126,12,2},
    {126,12,3},
    {126,13,1},
    {126,13,2},
    {126,13,3},
    {126,14,1},
    {126,14,2},
    {126,14,3},
    {126,15,1},
    {126,15,2},
    {126,15,3},
    {126,16,1},
    {126,16,2},
    {126,16,3},
    {127,2,1},
    {127,2,2},
    {127,2,3},
    {127,3,1},
    {127,3,2},
    {127,3,3},
    {127,4,1},
    {127,4,2},
    {127,4,3},
    {128,2,1},
    {128,2,2},
    {128,2,3},
    {128,4,1},
    {128,4,2},
    {128,4,3},
    {128,5,1},
    {128,5,2},
    {128,5,3},
    {128,9,1},
    {128,9,2},
    {128,9,3},
    {128,10,1},
    {128,10,2},
    {128,10,3},
    {128,11,1},
    {128,11,2},
    {128,11,3},
    {128,13,1},
    {128,13,2},
    {128,13,3},
    {128,15,1},
    {128,15,2},
    {128,15,3},
    {129,3,1},
    {129,3,2},
    {129,3,3},
    {129,5,1},
    {129,5,2},
    {129,5,3},
    {129,9,1},
    {129,9,2},
    {129,9,3},
    {129,10,1},
    {129,10,2},
    {129,10,3},
    {129,11,1},
    {129,11,2},
    {129,11,3},
    {129,12,1},
    {129,12,2},
    {129,12,3},
    {129,13,1},
    {129,13,2},
    {129,13,3},
    {129,14,1},
    {129,14,2},
    {129,14,3},
    {129,15,1},
    {129,15,2},
    {129,15,3},
    {130,2,1},
    {130,2,2},
    {130,2,3},
    {130,3,1},
    {130,3,2},
    {130,3,3},
    {130,4,1},
    {130,4,2},
    {130,4,3},
    {130,13,1},
    {130,13,2},
    {130,13,3},
    {130,14,1},
    {130,14,2},
    {130,14,3},
    {130,15,1},
    {130,15,2},
    {130,15,3},
    {131,5,1},
    {131,5,2},
    {131,5,3},
    {131,9,1},
    {131,9,2},
    {131,9,3},
    {131,10,1},
    {131,10,2},
    {131,10,3},
    {131,11,1},
    {131,11,2},
    {131,11,3},
    {131,12,1},
    {131,12,2},
    {131,12,3},
    {131,13,1},
    {131,13,2},
    {131,13,3},
    {131,14,1},
    {131,14,2},
    {131,14,3},
    {131,15,1},
    {131,15,2},
    {131,15,3},
    {132,16,1},
    {132,16,2},
    {132,16,3},
    {133,6,1},
    {133,6,2},
    {133,6,3},
    {134,2,1},
    {134,2,2},
    {134,2,3},
    {134,4,1},
    {134,4,2},
    {134,4,3},
    {134,5,1},
    {134,5,2},
    {134,5,3},
    {134,9,1},
    {134,9,2},
    {134,9,3},
    {134,10,1},
    {134,10,2},
    {134,10,3},
    {134,11,1},
    {134,11,2},
    {134,11,3},
    {134,13,1},
    {134,13,2},
    {134,13,3},
    {134,15,1},
    {134,15,2},
    {134,15,3},
    {135,2,1},
    {135,2,2},
    {135,2,3},
    {135,4,1},
    {135,4,2},
    {135,4,3},
    {135,5,1},
    {135,5,2},
    {135,5,3},
    {135,9,1},
    {135,9,2},
    {135,9,3},
    {135,10,1},
    {135,10,2},
    {135,10,3},
    {135,11,1},
    {135,11,2},
    {135,11,3},
    {135,13,1},
    {135,13,2},
    {135,13,3},
    {135,15,1},
    {135,15,2},
    {135,15,3},
    {136,6,1},
    {136,6,2},
    {136,6,3},
    {136,9,1},
    {136,9,2},
    {136,9,3},
    {137,2,1},
    {137,2,2},
    {137,2,3},
    {137,4,1},
    {137,4,2},
    {137,4,3}
};

const std::array<std::array<int, 2>, maxSolverPlatformMap> SolverPlatformMapTuple = {
    std::array<int, 2> {1,1},
    {1,2},
    {1,3},
    {2,1},
    {2,2},
    {2,3},
    {3,1},
    {3,2},
    {3,3},
    {4,1},
    {4,2},
    {4,3},
    {5,1},
    {5,2},
    {5,3},
    {6,1},
    {6,2},
    {6,3},
    {7,1},
    {7,2},
    {7,3},
    {8,1},
    {8,2},
    {8,3},
    {9,1},
    {9,2},
    {9,3},
    {10,1},
    {10,2},
    {10,3},
    {11,1},
    {11,2},
    {11,3},
    {12,1},
    {12,2},
    {12,3},
    {13,1},
    {13,2},
    {13,3},
    {14,1},
    {14,2},
    {14,3},
    {15,1},
    {15,2},
    {15,3},
    {16,1},
    {16,2},
    {16,3},
    {17,1},
    {17,2},
    {17,3},
    {18,1},
    {18,2},
    {18,3},
    {19,1},
    {19,2},
    {19,3},
    {20,1},
    {20,2},
    {20,3},
    {21,1},
    {21,2},
    {21,3},
    {22,1},
    {22,2},
    {22,3},
    {23,1},
    {23,2},
    {23,3},
    {24,1},
    {24,2},
    {24,3},
    {25,1},
    {25,2},
    {25,3},
    {26,1},
    {26,2},
    {26,3},
    {27,1},
    {27,2},
    {27,3},
    {28,1},
    {28,2},
    {28,3},
    {29,1},
    {29,2},
    {29,3},
    {30,1},
    {30,2},
    {30,3},
    {31,1},
    {31,2},
    {31,3},
    {32,1},
    {32,2},
    {32,3},
    {33,1},
    {33,2},
    {33,3},
    {34,1},
    {34,2},
    {35,1},
    {35,2},
    {36,1},
    {36,2},
    {36,3},
    {37,1},
    {37,2},
    {37,3},
    {38,1},
    {38,2},
    {38,3},
    {39,1},
    {39,2},
    {39,3},
    {40,1},
    {40,2},
    {40,3},
    {41,1},
    {41,2},
    {41,3},
    {43,1},
    {43,2},
    {43,3},
    {44,1},
    {44,2},
    {44,3},
    {45,1},
    {45,2},
    {45,3},
    {46,1},
    {46,2},
    {46,3},
    {47,1},
    {47,2},
    {47,3},
    {48,1},
    {48,2},
    {48,3},
    {49,1},
    {49,2},
    {49,3},
    {50,1},
    {50,2},
    {50,3},
    {51,1},
    {52,1},
    {52,2},
    {52,3},
    {53,1},
    {53,2},
    {53,3},
    {54,1},
    {54,2},
    {54,3},
    {55,1},
    {55,2},
    {55,3},
    {56,1},
    {56,2},
    {56,3},
    {57,1},
    {57,2},
    {57,3},
    {58,1},
    {59,1},
    {60,1},
    {61,1},
    {61,2},
    {61,3},
    {62,1},
    {62,2},
    {62,3},
    {63,1},
    {64,1},
    {64,2},
    {64,3},
    {65,1},
    {65,2},
    {65,3},
    {66,1},
    {66,2},
    {66,3},
    {67,1},
    {67,2},
    {67,3},
    {68,1},
    {68,2},
    {68,3},
    {69,1},
    {69,2},
    {69,3},
    {70,1},
    {70,2},
    {70,3},
    {71,1},
    {71,2},
    {71,3},
    {72,1},
    {72,2},
    {72,3},
    {73,1},
    {74,1},
    {75,1},
    {75,2},
    {75,3},
    {76,1},
    {76,2},
    {76,3},
    {77,1},
    {77,2},
    {77,3},
    {78,1},
    {79,1},
    {79,2},
    {79,3},
    {80,1},
    {80,2},
    {80,3},
    {81,1},
    {82,1},
    {83,1},
    {83,2},
    {83,3},
    {84,1},
    {84,2},
    {84,3},
    {85,1},
    {85,2},
    {85,3},
    {86,1},
    {87,1},
    {87,2},
    {87,3},
    {88,1},
    {88,2},
    {88,3},
    {89,1},
    {90,1},
    {91,1},
    {91,2},
    {91,3},
    {92,1},
    {92,2},
    {92,3},
    {93,1},
    {94,1},
    {95,1},
    {96,1},
    {96,2},
    {96,3},
    {97,1},
    {97,2},
    {97,3},
    {98,1},
    {98,2},
    {98,3},
    {99,1},
    {99,2},
    {99,3},
    {100,1},
    {100,2},
    {100,3},
    {101,1},
    {101,2},
    {101,3},
    {102,1},
    {102,2},
    {102,3},
    {103,1},
    {103,2},
    {103,3},
    {104,1},
    {104,2},
    {104,3},
    {105,1},
    {105,2},
    {105,3},
    {106,1},
    {106,2},
    {106,3},
    {107,1},
    {107,2},
    {107,3},
    {108,1},
    {108,2},
    {108,3},
    {109,1},
    {109,2},
    {109,3},
    {110,1},
    {110,2},
    {110,3},
    {111,1},
    {111,2},
    {111,3},
    {112,1},
    {112,2},
    {112,3},
    {113,1},
    {113,2},
    {113,3},
    {114,1},
    {114,2},
    {114,3},
    {115,1},
    {115,2},
    {115,3},
    {116,1},
    {116,2},
    {116,3},
    {117,1},
    {117,2},
    {117,3},
    {118,1},
    {118,2},
    {118,3},
    {119,1},
    {119,2},
    {119,3},
    {120,1},
    {120,2},
    {120,3},
    {121,1},
    {121,2},
    {121,3},
    {122,1},
    {122,2},
    {122,3},
    {123,1},
    {123,2},
    {123,3},
    {124,1},
    {124,2},
    {124,3},
    {125,1},
    {125,2},
    {125,3},
    {126,1},
    {126,2},
    {126,3},
    {127,1},
    {127,2},
    {127,3},
    {128,1},
    {128,2},
    {128,3},
    {129,1},
    {129,2},
    {129,3},
    {130,1},
    {130,2},
    {130,3},
    {131,1},
    {131,2},
    {131,3},
    {132,1},
    {132,2},
    {132,3},
    {133,1},
    {133,2},
    {133,3},
    {134,1},
    {134,2},
    {134,3},
    {135,1},
    {135,2},
    {135,3},
    {136,1},
    {136,2},
    {136,3},
    {137,1},
    {137,2},
    {137,3}
};

const std::array<std::string, maxClipCodes> ClipCodesKeyS = {
    "00"s,
    "01"s,
    "02"s,
    "03"s,
    "04"s,
    "05"s,
    "06"s,
    "0M"s,
    "0S"s,
    "AT"s,
    "BA"s,
    "BD"s,
    "CL"s,
    "CO"s,
    "CP"s,
    "CD"s,
    "CM"s,
    "CT"s,
    "CK"s,
    "DE"s,
    "DI"s,
    "EC"s,
    "FB"s,
    "FR"s,
    "GE"s,
    "GS"s,
    "GQ"s,
    "GL"s,
    "GU"s,
    "GD"s,
    "HI"s,
    "IP"s,
    "KN"s,
    "LA"s,
    "LD"s,
    "LG"s,
    "LI"s,
    "LS"s,
    "LL"s,
    "LO"s,
    "M5"s,
    "MB"s,
    "MC"s,
    "ML"s,
    "MK"s,
    "MN"s,
    "MS"s,
    "NA"s,
    "O2"s,
    "OA"s,
    "OC"s,
    "OD"s,
    "OL"s,
    "OQ"s,
    "OS"s,
    "PT"s,
    "SB"s,
    "SC"s,
    "SE"s,
    "SN"s,
    "XA"s,
    "XL"s,
    "XP"s,
    "XS"s,
    "XX"s,
    "WZ"s,
    "ZO"s
};

const std::array<std::string, maxClipCodes> ClipCodesTextS = {
    "GAMS/Demo GAMS Development Corp"s,
    "GAMS      GAMS Development Corp"s,
    "GAMS/Run  GAMS Development Corp"s,
    "GAMS/App  GAMS Development Corp"s,
    "GAMS/Node GAMS Development Corp"s,
    "GAMS/Comm GAMS Development Corp"s,
    "GAMS/NSlv GAMS Development Corp"s,
    "GAMS/MIRO GAMS Development Corp"s,
    "GAMS/Sec  GAMS Development Corp"s,
    "ANTIGONE  Princeton Univeristy"s,
    "BARON     University of Illinois at Urbana-Champaign"s,
    "BDMLP     GAMS Development Corp"s,
    "CPLEX/L   Cplex Optimization (Link only)"s,
    "CONOPT    ARKI Consulting"s,
    "CPLEX     Cplex Optimization (license options in comment line)"s,
    "CPLEXDIST Cplex Optimization Distributed MIP"s,
    "CPOPTIM   Cplex Optimization"s,
    "COPT      Cardinal Operations"s,
    "COPT      Cardinal Operations (Link only)"s,
    "DECIS     G. Infanger, Inc."s,
    "DICOPT    CAPD, Carnegie Mellon University"s,
    "ALPHAECP  Abo University, Finland"s,
    "FILTERBB  The University of Dundee"s,
    "FREE      Maintained Freeware"s,
    "MPS/GE    Thomas Rutherford"s,
    "GAMS/Sys  GAMS Development Corp"s,
    "GloMIQO   Princeton Univeristy"s,
    "GUROBI/L  Gurobi Optimization (Link only)"s,
    "GUROBI    Gurobi Optimization"s,
    "GUROBI/D  Gurobi Optimization"s,
    "HIGHS     Edinburgh Research Group in Optimization"s,
    "IPOPT     COIN-OR Foundation"s,
    "KNITRO    Artelys           "s,
    "LAMPS     Advanced Mathematical Software, Inc."s,
    "LINDO-API Lindo Systems, Inc."s,
    "LGO       Pinter Consulting Services"s,
    "LINDOGlob Lindo Systems, Inc."s,
    "LOCALSol  LocalSolver"s,
    "LOCALS/L  LocalSolver (Link only)"s,
    "LOQO      Princeton University"s,
    "MINOS     Stanford University"s,
    "MOSEK     EKA Consulting ApS"s,
    "MILES     GAMS Development Corp"s,
    "MOSEK/L   EKA Consulting ApS"s,
    "MOSEK/MIP EKA Consulting ApS"s,
    "MSNLP     Optimal Methods"s,
    "MOPS      Freie University Berlin"s,
    "NONO      Not Allowed"s,
    "OSL2      IBM/OSL Version 2"s,
    "OCTERACT  Octeract"s,
    "CPLEXOSI  COIN-OR Foundation"s,
    "ODHCPLEX  Optimization Direct Inc."s,
    "OSL/L     IBM/OSL Link"s,
    "OQNLP/GRG OptTek Systems and Optimal Methods"s,
    "OSL       IBM/OSL"s,
    "PATH      University of Wisconsin - Madison"s,
    "SBB       ARKI Consulting"s,
    "SCIP      ZIB Berlin"s,
    "OSLSE     IBM/OSL Stochastic Extension"s,
    "SNOPT     Stanford University"s,
    "XA        Sunset Software"s,
    "XPRESS/L  FICO"s,
    "XPRESS    FICO"s,
    "XPRESSSLP FICO"s,
    "XPRESSALL FICO"s,
    "WHIZZARD  Ketron Management Science"s,
    "ZOOM      XMP Optimization Software"s
};

const std::array<std::string, maxTLLicense> TLLicenseKeyS = {
    "EVALUATION"s,
    "COURSE"s,
    "LEASE"s,
    "ANNUAL"s,
    "TAKEGAMSWITHYOU"s,
    "APPLICATION"s
};

const std::array<std::string, maxTLLicense> TLLicenseTextS = {
    "Evaluation license: Not for commercial or production use"s,
    "Course license for use within the course and related course work"s,
    "GAMS month to month lease license"s,
    "GAMS annual lease license"s,
    "Take-GAMS-With-You license: Not for commercial or production use"s,
    "GAMS Application lease license"s
};


