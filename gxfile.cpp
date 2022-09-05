#include "gxfile.h"
#include "rtl/sysutils_p3.h"
#include "utils.h"
#include "gdlib/gmsstrm.h"
#include "gdlib/gmsglob.h"
#include "gdlib/runner.h"
#include "global/modhead.h"
#include "global/gmsspecs.h"
#include "palxxx/gdlaudit.h"
#include "global/unit.h"

#include <cassert>

using namespace gdxinterface;
using namespace gdlib::gmsstrm;
using namespace global::gmsspecs;
using namespace std::literals::string_literals;
using namespace gxdefs;
using namespace gdlib::gmsglob;

namespace gxfile {

    const int   VERSION = 7,
                gdxHeaderNr = 123;
    const std::string gdxHeaderId = "GAMSGDX";

    const int   MARK_BOI = 19510624;

    const std::string
        MARK_UEL  = "_UEL_"s,
        MARK_SYMB = "_SYMB_"s,
        MARK_DATA = "_DATA_"s,
        MARK_SETT = "_SETT_"s,
        MARK_ACRO = "_ACRO_"s,
        MARK_DOMS = "_DOMS_"s;

    const int INDEX_INITIAL = -256;

    const std::array fmode_str {
        "FileNotOpen"s  ,   //f_not_open
        "ReadCommand"s  ,   //fr_init
        "WriteCommand"s ,   //fw_init
        "Write-Dom-Raw"s,
        "Write-Dom-Map"s,
        "Write-Dom-Str"s,
        "Write-Raw"s    ,   //fw_raw_data
        "Write-Map"s    ,   //fw_Map_data
        "Write-Str"s    ,   //fw_str_data
        "Regis-Raw"s    ,   //f_raw_elem
        "Regis-Map"s    ,   //f_Map_elem
        "Regis-Str"s    ,   //f_str_elem
        "Read-Raw"s     ,   //fr_raw_data
        "Read-Map"s     ,   //fr_Map_data
        "Read_MapR"s    ,   //fr_MapR_data
        "Read-Str"s     ,   //fr_str_data
        "Regis-Filter"s ,   //fr_filter
        "Read-Slice"s     //fr_slice
    };

    std::string DLLLoadPath;
    
    const int
    ERR_NOERROR              =  0,
    ERR_NOFILE               = -100000,
    ERR_FILEERROR            = -100001,
    ERR_BADMODE              = -100002,
    ERR_BADDIMENSION         = -100003,
    ERR_BADELEMENTINDEX      = -100004,
    ERR_BADSYMBOLINDEX       = -100005,
    ERR_ELEMENTSEQUENCE      = -100006,
    ERR_DUPLICATESYMBOL      = -100007,
    ERR_DATANOTSORTED        = -100008,
    ERR_DATADUPLICATE        = -100009,
    ERR_UNKNOWNFILTER        = -100010,
    ERR_BADSTRINGFORMAT      = -100011,
    ERR_BADIDENTFORMAT       = -100012,
    ERR_UELCONFLICT          = -100013,
    ERR_DUPLICATESPECVAL     = -100014,
    ERR_BADERRORRECORD       = -100015,
    ERR_DUPLICATEUEL         = -100016,
    ERR_BADUELSTR            = -100017,
    ERR_UNDEFUEL             = -100018,
    ERR_UELSECONDWRITE       = -100019,
    ERR_UELNOTEMPTY          = -100020,
    ERR_BAD_FILTER_NR        = -100021,
    ERR_BAD_FILTER_INDX      = -100022,
    ERR_FILTER_UNMAPPED      = -100023,
    ERR_OBSOLETE_FUNCTION    = -100024,
    ERR_RAWNOTSORTED         = -100025,
    ERR_BAD_ALIAS_DIM        = -100026,
    ERR_BADDATAMARKER_DATA   = -100029,
    ERR_BADDATAMARKER_DIM    = -100030,
    ERR_OPEN_BOI             = -100031,
    ERR_OPEN_FILEHEADER      = -100032,
    ERR_OPEN_FILEVERSION     = -100033,
    ERR_OPEN_FILEMARKER      = -100034,
    ERR_OPEN_SYMBOLMARKER1   = -100035,
    ERR_OPEN_SYMBOLMARKER2   = -100036,
    ERR_OPEN_UELMARKER1      = -100037,
    ERR_OPEN_UELMARKER2      = -100038,
    ERR_OPEN_TEXTMARKER1     = -100039,
    ERR_OPEN_TEXTMARKER2     = -100040,
    ERR_BADDATAFORMAT        = -100041,
    ERR_NEXT_ERROR           = -100042,
    ERR_OUT_OF_MEMORY        = -100043,
    ERR_ZLIB_NOT_FOUND       = -100044, //check with gmsstrm
    ERR_OPEN_ACROMARKER1     = -100045,
    ERR_OPEN_ACROMARKER2     = -100046,
    ERR_BADACROINDEX         = -100047,
    ERR_BADACRONUMBER        = -100048,
    ERR_BADACRONAME          = -100049,
    ERR_ACRODUPEMAP          = -100050,
    ERR_ACROBADADDITION      = -100051,
    ERR_UNKNOWNDOMAIN        = -100052,
    ERR_BADDOMAIN            = -100053,
    ERR_NODOMAINDATA         = -100054,
    ERR_ALIASSETEXPECTED     = -100055,
    ERR_BADDATATYPE          = -100056,
    ERR_NOSYMBOLFORCOMMENT   = -100057,
    ERR_DOMAINVIOLATION      = -100058,
    ERR_FILEALREADYOPEN      = -100059,
    ERR_FILETOOLDFORAPPEND   = -100060,
    ERR_OPEN_DOMSMARKER1     = -100061,
    ERR_OPEN_DOMSMARKER2     = -100062,
    ERR_OPEN_DOMSMARKER3     = -100063,

    //Errors from gdxcopy
    ERR_GDXCOPY              = -100100;
    const int
    ERR_PARAMETER            = ERR_GDXCOPY -  1,
    ERR_DLL_NOT_FOUND        = ERR_GDXCOPY -  2,
    ERR_CREATE_DIR           = ERR_GDXCOPY -  3,
    ERR_FILE_OPEN            = ERR_GDXCOPY -  4,
    ERR_FILE_WRITE           = ERR_GDXCOPY -  5,
    ERR_UEL_LENGTH           = ERR_GDXCOPY -  6,
    ERR_UEL_REGISTER         = ERR_GDXCOPY -  7,
    ERR_EXPL_TEXT            = ERR_GDXCOPY -  8,
    ERR_DIMENSION            = ERR_GDXCOPY -  9,
    ERR_WRITE_SYMBOL         = ERR_GDXCOPY - 10,
    ERR_CLOSE_FILE           = ERR_GDXCOPY - 11,
    ERR_CANNOT_DELETE        = ERR_GDXCOPY - 12,
    ERR_CANNOT_RENAME        = ERR_GDXCOPY - 13;

    // ...

    int GetEnvCompressFlag();

    static int ConvertGDXFile(const std::string &fn, const std::string &MyComp) {
        std::string Conv {utils::trim(utils::uppercase(rtl::sysutils_p3::QueryEnvironmentVariable(strGDXCONVERT)))};
        if(Conv.empty()) Conv = "V7"s;
        std::string Comp = Conv == "V5" ? ""s : (!GetEnvCompressFlag() ? "U" : "C");
        if(utils::sameText(Conv+Comp, "V7"+MyComp)) return 0;

        gdlib::runner::TRunner R {};
        R.SetExecutable(DLLLoadPath.empty() ? "gdxcopy" : DLLLoadPath + rtl::sysutils_p3::PathDelim + "gdxcopy");
        R.ParamsAdd("-" + Conv + Comp);
        R.ParamsAdd("-Replace");
        R.ParamsAdd(fn);
        int res{ R.StartAndWait() };
        if(!res && R.GetProgRC()) res = ERR_GDXCOPY - R.GetProgRC();
        return res;
    }

    static std::string MakeGoodExplText(const std::string &s) {
        char q {'\0'};
        std::string res;
        res.reserve(s.length());
        for(char Ch : s) {
            if(!utils::in(Ch, '\"', '\'')) {
                if(Ch < ' ') Ch = '?';
            } else {
                if(q == '\0') q = Ch;
                Ch = q;
            }
            res += Ch;
        }
        return res;
    }

    static bool IsGoodIdent(const std::string &S) {
        if(!S.empty() && S.length() <= MaxNameLen && isalpha(S.front())) {
            for(int n{1}; n<S.length(); n++)
                if(!isalnum(S[n]) && S[n] != '_') return false;
            return true;
        }
        return false;
    }

    static TgdxElemSize GetIntegerSize(int N) {
        if (N <= 0) return sz_integer;
        else if (N <= 255) return sz_byte;
        else if (N <= 65535) return sz_word;
        return sz_integer;
    }

    union TI64Rec {
        double x;
        int64_t i64;
    };

    int64_t dblToI64(double x) {
        TI64Rec i64Rec {x};
        return i64Rec.i64;
    }

    void copyIntlMapDblToI64(const TIntlValueMapDbl &dMap, TIntlValueMapI64 &iMap) {
        for(int k=0; k<iMap.size(); k++) {
            iMap[k] = dblToI64(dMap[k]);
        }
    }

    int GetEnvCompressFlag() {
        std::string s{ rtl::sysutils_p3::QueryEnvironmentVariable(strGDXCOMPRESS) };
        s = utils::uppercase(s.substr(0, 1));
        return s.empty() || s == "N" || s == "0" ? 0 : 1;
    }

    int TGXFileObj::gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) {
        return gdxOpenWriteEx(FileName, Producer, GetEnvCompressFlag(), ErrNr);
    }

    int TGXFileObj::gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) {
        if(fmode != f_not_open) {
            ErrNr = ERR_FILEALREADYOPEN;
            return false;
        }
        if(FileName.empty()) {
            ErrNr = ERR_NOFILE;
            LastError = ErrNr;
            return false;
        }
        FFile = std::make_unique<TMiBufferedStreamDelphi>(FileName, fmCreate, DLLLoadPath);
        ErrNr = FFile->GetLastIOResult();
        if(ErrNr) {
            FFile = nullptr;
            if(ErrNr == strmErrorZLib) ErrNr = ERR_ZLIB_NOT_FOUND;
            LastError = ErrNr;
            return false;
        }

        Compr &= (FFile->GetCanCompress() ? 1 : 0);
        fComprLev = Compr;
        CompressOut = Compr > 0;
        fmode = f_not_open;
        ReadPtr = nullptr;
        MajContext = "OpenWrite";
        TraceLevel = trl_none;
        InitErrors();
        NameList.clear();
        UELTable.clear();
        AcronymList.clear();
        FilterList.clear();
        FFile->WriteByte(gdxHeaderNr);
        FFile->WriteString(gdxHeaderId);
        VersionRead = VERSION;
        FFile->WriteInteger(VersionRead);
        FFile->WriteInteger(Compr);
        FileSystemID = palxxx::gdlaudit::gdlGetAuditLine();
        //FileSystemID = "GDX Library      41.0.0 30fa42b3 Aug  9, 2022  (ALPHA) WEI x86 64bit/MS Window"s;
        FFile->WriteString(FileSystemID);
        FProducer = Producer;
        FProducer2.clear();
        FFile->WriteString(FProducer);
        // Reserve some space for positions
        MajorIndexPosition = FFile->GetPosition();
        for(int N{1}; N<=10; N++) FFile->WriteInt64(0);
        SetTextList.clear();
        SetTextList.emplace_back(""s);
        gdxResetSpecialValues();
        NextWritePosition = FFile->GetPosition();
        fmode = fw_init;
        DomainStrList.clear();
        return true;
    }

    int TGXFileObj::gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) {
        if(!PrepareSymbolWrite("DataWriteStrStart", SyId, ExplTxt, Dim, Typ, UserInfo)) return false;
        /*for (int D{}; D < FCurrentDim; D++)
            LastStrElem[D] = (char)0xFF;*/
        fmode = fw_dom_str;
        return true;
    }

    // Brief:
    //   Write a data element in string mode
    // Arguments:
    //   KeyStr: The index for this element using strings for the unique elements
    //   Values: The values for this element
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataWriteMapStart, gdxDataWriteDone
    // Description:
    //   When writing data using string elements, each string element is added
    //   to the internal unique element table and assigned an index.
    //   Writing using strings does not add the unique elements to the
    //   user mapped space.
    //   Each element string must follow the GAMS rules for unique elements.
    int TGXFileObj::gdxDataWriteStr(const TgdxStrIndex &KeyStr, const TgdxValues &Values) {
        const TgxModeSet AllowedModes {fw_str_data};
        if(fmode == fw_dom_str) fmode = fw_str_data;
        if(TraceLevel >= trl_all || !utils::in(fmode, AllowedModes)) {
            if(!CheckMode("DataWriteStr", AllowedModes)) return false;
            std::cout << "  Index =\n";
            for(int D{}; D<FCurrentDim; D++)
                std::cout << " " << KeyStr[D] << (D+1 < FCurrentDim ? "," : "") << "\n";
        }
        for(int D{}; D<FCurrentDim; D++) {
            std::string SV {utils::trimRight(KeyStr[D])};
            if(SV != LastStrElem[D]) {
                int KD {UELTable.IndexOf(SV)};
                if(KD == -1) {
                    if(ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
                    KD = UELTable.AddObject(SV, -1);
                }
                KD++; // increment here to match Delphi behavior
                LastElem[D] = KD;
                LastStrElem[D] = SV;
                if(KD < MinElem[D]) MinElem[D] = KD;
                if(KD > MaxElem[D]) MaxElem[D] = KD;
            }
        }
        SortList[LastElem] = Values;
        return true;
    }

    // Brief:
    //   Finish a write operation
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //  gdxDataErrorCount, gdxDataWriteRawStart, gdxDataWriteMapStart, gdxDataWriteStrStart,
    // Description:
    int TGXFileObj::gdxDataWriteDone() {
        const TgxModeSet AllowedModes {fw_raw_data,fw_map_data,fw_str_data, fw_dom_raw, fw_dom_map, fw_dom_str};
        if(!MajorCheckMode("DataWriteDone"s, AllowedModes)) return false;
        if(!utils::in(fmode, fw_raw_data, fw_dom_raw)) {
            InitDoWrite(static_cast<int>(SortList.size()));
            for(const auto &[keys, values] : SortList) {
                /*TIndex AElements;
                gxdefs::TgdxValues AVals;*/
                DoWrite(keys, values);
            }
            SortList.clear();
        }
        FFile->WriteByte(255); // end of data
        NextWritePosition = FFile->GetPosition();
        CurSyPtr->SDataCount = DataCount; // reflects dupes
        CurSyPtr->SErrors = ErrCnt;
        ErrCnt = 0;
        fmode = fw_init;
        FFile->SetCompression(false);
        CurSyPtr = nullptr;
        return true;
    }

    // Brief:
    //   Close a gdx file
    // Description
    //   Close a gdx file that was previously opened for reading or writing.
    //   Before the file is closed, any pending write operations will be
    //   finished. To free the gdx object, call gdxFree.
    // Returns:
    //   Returns the value of gdxGetLastError
    // See Also:
    //   gdxOpenRead, gdxOpenWrite
    int TGXFileObj::gdxClose() {
        std::string fnConv;
        if(utils::in(fmode, fw_raw_data, fw_map_data, fw_str_data))
            gdxDataWriteDone();
        if(fmode == fw_init) {
            fnConv = FFile->GetFileName();
            FFile->SetCompression(CompressOut);
            FFile->SetPosition(NextWritePosition);
            int64_t SymbPos = NextWritePosition;
            FFile->WriteString(MARK_SYMB);
            FFile->WriteInteger(static_cast<int>(NameList.size()));
            for(const auto &[name, PSy] : NameList) {
                FFile->WriteString(name);
                FFile->WriteInt64(PSy->SPosition);
                FFile->WriteInteger(PSy->SDim);
                FFile->WriteByte(PSy->SDataType);
                FFile->WriteInteger(PSy->SUserInfo);
                FFile->WriteInteger(PSy->SDataCount);
                FFile->WriteInteger(PSy->SErrors);
                FFile->WriteByte(PSy->SSetText);
                FFile->WriteString(PSy->SExplTxt);
                FFile->WriteByte(PSy->SIsCompressed);
                FFile->WriteByte(PSy->SDomSymbols.empty() ? 0 : 1);
                if(!PSy->SDomSymbols.empty()) {
                    for(int D{}; D<PSy->SDim; D++)
                        FFile->WriteInteger(PSy->SDomSymbols[D]);
                }
                int CommCnt {static_cast<int>(PSy->SCommentsList.size())};
                FFile->WriteInteger(CommCnt);
                for(int Cnt{}; Cnt<CommCnt; Cnt++)
                    FFile->WriteString(PSy->SCommentsList[Cnt]);
            }
            FFile->WriteString(MARK_SYMB);

            auto SetTextPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_SETT);
            FFile->WriteInteger(static_cast<int>(SetTextList.size()));
            for(const auto &SetText : SetTextList)
                FFile->WriteString(SetText);
            FFile->WriteString(MARK_SETT);

            auto UELPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_UEL);
            FFile->WriteInteger(UELTable.size());
            for(const auto &uelName : UELTable.getNames())
                FFile->WriteString(uelName);
            FFile->WriteString(MARK_UEL);

            auto AcronymPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_ACRO);
            FFile->WriteInteger(static_cast<int>(AcronymList.size()));
            for(const auto &acro : AcronymList) {
                FFile->WriteString(acro.AcrName.empty() ? "UnknownACRO" + std::to_string(acro.AcrMap) : acro.AcrName);
                FFile->WriteString(acro.AcrText);
                FFile->WriteInteger(acro.AcrMap);
            }
            FFile->WriteString(MARK_ACRO);

            auto DomStrPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_DOMS);
            FFile->WriteInteger(static_cast<int>(DomainStrList.size()));
            for(const auto &DomStr : DomainStrList)
                FFile->WriteString(DomStr);
            FFile->WriteString(MARK_DOMS);
            int ix{};
            for(const auto &[name, PSy] : NameList) {
                if(!PSy->SDomStrings.empty()) {
                    FFile->WriteInteger(ix);
                    for(const auto &i : PSy->SDomStrings)
                        FFile->WriteInteger(i);
                }
                ix++;
            }
            FFile->WriteInteger(-1);
            FFile->WriteString(MARK_DOMS);

            // This must be at the very end!!!
            FFile->SetPosition(MajorIndexPosition);
            FFile->SetCompression(false);
            FFile->WriteInteger(MARK_BOI);
            // Note that we have room for 10 indices; if we need more, create an overflow link in the 10th position.
            const std::array<int64_t, 6> offsets = {SymbPos, UELPos, SetTextPos, AcronymPos, NextWritePosition, DomStrPos};
            for(int64_t offset : offsets)
                FFile->WriteInt64(offset);
        }

        int res{FFile->GetLastIOResult()};

        // Many free operations. Some not necessary anymore due to RAII pattern (out of scope -> destroy)

        // ...
        STUBWARN();

        for(const auto &[name, psy] : NameList)
            delete psy;

        fmode = f_not_open;
        fstatus = stat_notopen;

        if(AutoConvert && !fnConv.empty()) {
            res = ConvertGDXFile(fnConv, CompressOut ? "C" : "U");
            if(res > 0) res += 100;
        }
        return res;
    }

    void TGXFileObj::InitErrors() {
        ErrCnt = ErrCntTotal = 0;
        LastError = LastRepError = ERR_NOERROR;
    }

    TGXFileObj::TGXFileObj(std::string &ErrMsg) : fstatus{stat_notopen} {
        ErrMsg.clear();
        gdxResetSpecialValues();
    }

    TGXFileObj::~TGXFileObj() {
        if(fmode != f_not_open) {
            fmode = fr_init;
            gdxClose();
        }
    }

    int TGXFileObj::gdxResetSpecialValues() {
        intlValueMapDbl[vm_valund] = valund;
        intlValueMapDbl[vm_valna ] = valna;
        intlValueMapDbl[vm_valpin] = valpin;
        intlValueMapDbl[vm_valmin] = valmin;
        intlValueMapDbl[vm_valeps] = valeps;
        intlValueMapDbl[vm_zero] = 0.0;
        intlValueMapDbl[vm_one] = 1.0;
        intlValueMapDbl[vm_mone] = -1.0;
        intlValueMapDbl[vm_half] = 0.5;
        intlValueMapDbl[vm_two] = 2.0;
        readIntlValueMapDbl = intlValueMapDbl;
        copyIntlMapDblToI64(intlValueMapDbl, intlValueMapI64);
        Zvalacr = valacr;
        return true;
    }

    bool TGXFileObj::PrepareSymbolWrite(const std::string &Caller,
                                        const std::string &AName,
                                        const std::string &AText,
                                        int ADim,
                                        int AType,
                                        int AUserInfo) {
        const TgxModeSet AllowedModes{fw_init};

        CurSyPtr = nullptr;
        ErrorList.clear();
        SortList.clear();

        if(!MajorCheckMode(Caller, AllowedModes)) return false;

        if(TraceLevel >= trl_some)
            WriteTrace("Symbol = "s + AName + ", Dim = "s + std::to_string(ADim));

        if(!IsGoodNewSymbol(AName)) return false;

        if( ErrorCondition(ADim >= 0 && ADim <= global::gmsspecs::MaxDim, ERR_BADDIMENSION) ||
            ErrorCondition(AType >= 0 && AType <= dt_equ, ERR_BADDATATYPE)) return false;
        CurSyPtr = new TgdxSymbRecord{};
        auto &obj = CurSyPtr;
        obj->SPosition = CurSyPtr->SDataCount = CurSyPtr->SErrors = 0;
        obj->SDim = ADim;
        obj->SDataType = static_cast<TgdxDataType>(AType);
        obj->SUserInfo = AUserInfo;
        obj->SSetText = false;
        obj->SExplTxt = MakeGoodExplText(AText);
        obj->SIsCompressed = CompressOut && ADim > 0;
        obj->SCommentsList.clear();
        obj->SDomSymbols.clear();
        obj->SDomStrings.clear();

        CurSyPtr->SSyNr = static_cast<int>(NameList.size());
        NameList[AName] = CurSyPtr;
        FCurrentDim = ADim;
        DeltaForWrite = 255 - (VERSION <= 6 ? MaxDimV148 : FCurrentDim) - 1;

        DataSize = gxdefs::DataTypSize[AType];
        if(DataSize > 0)
            LastDataField = static_cast<tvarvaltype>(DataSize - 1);

        for(int D{}; D<FCurrentDim; D++) {
            LastElem[D] = INDEX_INITIAL;
            MinElem[D] = std::numeric_limits<int>::max();
            MaxElem[D] = 0;
            WrBitMaps[D].clear();
        }

        FFile->SetCompression(CurSyPtr->SIsCompressed);
        return true;
    }

    bool TGXFileObj::MajorCheckMode(const std::string &Routine, const TgxModeSet &MS) {
        MajContext = Routine;
        LastRepError = ERR_NOERROR;
        if((TraceLevel >= trl_some || !utils::in(fmode, MS)) && !CheckMode(Routine, MS)) {
            return false;
        }
        return true;
    }

    void TGXFileObj::WriteTrace(const std::string &s) {
        std::cout << "gdxTrace " << TraceStr << ": " << s << '\n';
    }

    bool TGXFileObj::IsGoodNewSymbol(const std::string &s) {
        if( ErrorCondition(NameList.find(s) == NameList.end(), ERR_DUPLICATESYMBOL) ||
            ErrorCondition(utils::indexOf<TAcronym>(AcronymList, [&s](auto acro) { return acro.AcrName == s; }) == -1, ERR_DUPLICATESYMBOL) ||
            ErrorCondition(IsGoodIdent(s), ERR_BADIDENTFORMAT)) return false;
        return true;
    }

    bool TGXFileObj::ErrorCondition(bool cnd, int N) {
        if(!cnd) ReportError(N);
        return !cnd;
    }

    void TGXFileObj::ReportError(int N) {
        if(TraceLevel >= trl_errors && N != LastRepError) {
            if(!MajContext.empty())
                std::cout << "Error after call to " << MajContext << '\n';
            std::string s;
            this->gdxErrorStr(N, s);
            std::cout << "Error = " << N << " : " << s << "\n";
        }
        SetError(N);
        LastRepError = N;
    }

    bool TGXFileObj::CheckMode(const std::string &Routine, const TgxModeSet &MS) {
        if(MS.empty() || utils::in(fmode, MS)) {
            WriteTrace(Routine);
            return true;
        }
        SetError(ERR_BADMODE);
        std::cout << "**** Error: " << Routine << " called out of context\n";
        if(!MajContext.empty() && !utils::sameText(MajContext, Routine))
            std::cout << "     Previous major function called was " << MajContext << '\n';
        std::cout << "     Current context = " << fmode_str[fmode] << '\n';
        std::cout << "     Allowed = {";
        bool f{true};
        for(int M{}; M<tgxfilemode_count; M++) {
            if(utils::in(static_cast<TgxFileMode>(M), MS)) {
                if(f) f = false;
                else std::cout << ',';
                std::cout << fmode_str[M];
            }
        }
        std::cout << "}" << std::endl;
        return false;
    }

    int TGXFileObj::PrepareSymbolRead(const std::string& Caller, int SyNr, const gxdefs::TgdxUELIndex& ADomainNrs, TgxFileMode newmode) {
        if (utils::in(fmode, fr_str_data, fr_map_data, fr_mapr_data, fr_raw_data))
            gdxDataReadDone();
        int res{-1};
        NrMappedAdded = 0;
        TIntegerMapping ExpndList;
        ErrorList.clear();
        CurSyPtr = nullptr;
        SortList.clear();

        const TgxModeSet AllowedModes{ fr_init };

        if (!MajorCheckMode(Caller, AllowedModes)) {
            fmode = fr_init;
            return -1;
        }
        ReadUniverse = !SyNr;
        if (!ReadUniverse) {
            if (ErrorCondition(SyNr >= 1 && SyNr <= NameList.size(), ERR_BADSYMBOLINDEX)) return -1;
            CurSyPtr = (*symbolWithIndex(SyNr)).second;
            if (CurSyPtr->SDataType == dt_alias) {
                do {
                    SyNr = CurSyPtr->SUserInfo;
                    if (!SyNr) {
                        ReadUniverse = true;
                        break;
                    }
                    CurSyPtr = (*symbolWithIndex(SyNr)).second;
                } while (CurSyPtr->SDataType == dt_alias);
                if (!ReadUniverse) {
                    assert(CurSyPtr->SDataType == dt_set && "Bad aliased set-1");
                }
            }
        }
        int NrRecs;
        if (ReadUniverse) {
            FCurrentDim = 1;
            DataSize = DataTypSize[dt_set];
            LastDataField = static_cast<tvarvaltype>( DataSize - 1 );
            NrRecs = UelCntOrig;
            UniverseNr = 0;
            CurSyPtr = nullptr;
        }
        else {
            FCurrentDim = CurSyPtr->SDim;
            FFile->SetCompression(CurSyPtr->SIsCompressed);
            FFile->SetPosition(CurSyPtr->SPosition);
            DataSize = DataTypSize[CurSyPtr->SDataType];
            if (DataSize > 0) LastDataField = static_cast<tvarvaltype>(DataSize - 1);
            NrRecs = CurSyPtr->SDataCount;
        }
        DeltaForRead = VersionRead <= 6 ? MaxDimV148 : FCurrentDim;
        for(int D{}; D<FCurrentDim; D++) {
            auto &obj = DomainList[D];
            obj.DFilter = nullptr;
            switch(ADomainNrs[D]) {
                case DOMC_UNMAPPED: obj.DAction = dm_unmapped; break;
                case DOMC_EXPAND: obj.DAction = dm_expand; break;
                case DOMC_STRICT: obj.DAction = dm_strict; break;
                default: // filter number
                    obj.DFilter = FilterList.FindFilter(ADomainNrs[D]);
                    if(obj.DFilter) obj.DAction = dm_filter;
                    else {
                        ReportError(ERR_UNKNOWNFILTER);
                        return res;
                    }
                    break;
            }
        }

        if(!ReadUniverse) {
            if( ErrorCondition(FFile->ReadString() == MARK_DATA,ERR_BADDATAMARKER_DATA) ||
                ErrorCondition(FFile->ReadByte()  == FCurrentDim,ERR_BADDATAMARKER_DIM)) return -1;
            FFile->ReadInteger();  //skip record counter
        }

        if(!FCurrentDim && !NrRecs) {
            CurSyPtr->SScalarFrst = true;
            fmode = newmode;
            return 1;
        }

        if(!ReadUniverse) {
            CurSyPtr->SScalarFrst = false;
            std::fill_n(LastElem.begin(), FCurrentDim, INDEX_INITIAL);
            std::fill_n(PrevElem.begin(), FCurrentDim, -1);
            for(int D{}; D<FCurrentDim; D++) {
                MinElem[D] = FFile->ReadInteger();
                MaxElem[D] = FFile->ReadInteger();
                ElemType[D] = GetIntegerSize(MaxElem[D] - MinElem[D] + 1);
            }
        }
        bool AllocOk{true};

        if(utils::in(newmode, fr_raw_data, fr_str_data, fr_slice))
            res = NrRecs;
        else {
            assert(newmode == fr_map_data && "Expect to read mapped data");
            if(ResultWillBeSorted(ADomainNrs)) {
                res = NrRecs;
                newmode = fr_mapr_data;
            } else {
                try {
                    int FIDim = FCurrentDim; // First invalid dimension
                    TgdxValues Avals;
                    TIndex AElements{};
                    int AFDim, V;
                    bool AddNew{}, AddError{}, BadError{};
                    while(DoRead(Avals, AFDim)) {
                        if(FIDim < AFDim) AFDim = FIDim;
                        FIDim = FCurrentDim;
                        int D;
                        for(D=AFDim; D<=FCurrentDim; D++) {
                            const auto &obj = DomainList[D];
                            if(LastElem[D] < 0) {
                                ReportError(ERR_BADELEMENTINDEX);
                                BadError = true;
                                break;
                            }
                            switch(obj.DAction) {
                                case dm_unmapped:
                                    AElements[D] = LastElem[D];
                                    break;
                                case dm_filter:
                                    V = UELTable.GetUserMap(LastElem[D]);
                                    if(obj.DFilter->InFilter(V))
                                        AElements[D] = V;
                                    else {
                                        AddError = true;
                                        FIDim = D;
                                        break;
                                    }
                                    break;
                                case dm_strict:
                                    V = UELTable.GetUserMap(LastElem[D]);
                                    if(V >= 0) AElements[D] = V;
                                    else {
                                        AddError = true;
                                        FIDim = D;
                                    }
                                    break;
                                case dm_expand: {
                                        int EN = LastElem[D];
                                        V = ExpndList[EN];
                                        if(V >= 0) AElements[D] = V;
                                        else {
                                            V = UELTable.GetUserMap(EN);
                                            if(V >= 0)
                                                AElements[D] = ExpndList[EN] = V;
                                            else {
                                                AElements[D] = -EN; // so we recognize and assign the same nr
                                                AddNew = true;
                                            }
                                        }
                                    }
                                    break;
                            }
                        }
                        if(BadError) {
                            AllocOk = false; // That is used as a signal below for something bad happend, i.e., an exception was thrown
                            break;
                        }
                        else if(AddError) {
                            // Ensure that dimensions to the right have no bad UELs
                            for(int D2{D+1}; D2 <= FCurrentDim; D2++) {
                                if(LastElem[D2] < 0) {
                                    ReportError(ERR_BADELEMENTINDEX);
                                    AllocOk = false; // That is used as a signal below for something bad happend, i.e., an exception was thrown
                                    break;
                                }
                            }
                            LastElem[FIDim] = -LastElem[FIDim];
                            AddToErrorListDomErrs(LastElem, Avals); // unmapped
                            LastElem[FIDim] = -LastElem[FIDim];
                            AddError = false;
                        } else {
                            if(AddNew) {
                                for(D=0; D<FCurrentDim; D++) {
                                    int EN = AElements[D];
                                    if(EN < 0) {
                                        V = UELTable.NewUsrUel(-EN);
                                        AElements[D] = ExpndList[-EN] = V;
                                        NrMappedAdded++;
                                        // look for same mapping to be issued
                                        for(int D2{D+1}; D2 <= FCurrentDim; D2++) {
                                            if(AElements[D2] == EN) AElements[D2] = V;
                                        }
                                    }
                                }
                                AddNew = false;
                            }
                            SortList[AElements] = Avals;
                        }
                    }
                    // FIXME: SortList.StartRead();
                    res = (int)SortList.size();
                } catch(std::exception &e) {
                    std::cout << "Exception: " << e.what() << std::endl;
                    AllocOk = false;
                }
            }
        }

        if(AllocOk) {
            std::fill_n(LastElem.begin(), FCurrentDim, -1);
            fmode = newmode;
            return res;
        } else {
            SetError(ERR_OUT_OF_MEMORY);
            SortList.clear();
            fmode = fr_init;
            return -1;
        }
    }

    void TGXFileObj::SetError(int N) {
        if(N == 0) return;
        if(LastError == ERR_NOERROR) LastError = N;
        ErrCnt++;
        ErrCntTotal++;
    }

    void TGXFileObj::InitDoWrite(int NrRecs) {
        DataCount = 0;
        FFile->SetPosition(NextWritePosition);
        CurSyPtr->SPosition = NextWritePosition;
        FFile->WriteString(MARK_DATA);
        FFile->WriteByte(FCurrentDim);
        FFile->WriteInteger(NrRecs); // ignores dupes in count
        for (int D{}; D < FCurrentDim; D++) {
            LastElem[D] = INDEX_INITIAL;
            ElemType[D] = GetIntegerSize(MaxElem[D] - MinElem[D] + 1);
            FFile->WriteInteger(MinElem[D]);
            FFile->WriteInteger(MaxElem[D]);
        }
    }

    bool HAVE_MEM;
    int64_t signMask, expoMask, mantMask;

    enum TDblClass {
        DBL_NAN,       /* any sort of NaN */
        DBL_NINF,      /* negative infinity */
        DBL_PINF,      /* positive infinity */
        DBL_FINITE     /* positive infinity */
    };

    const std::array DblClassText {
        "NaN"s, "negative infinity"s, "positive infinity"s, "finite"s
    };

    TDblClass dblInfo(double x, int64_t &i) {
        TI64Rec i64Rec{};
        i64Rec.x = x;
        i = i64Rec.i64;
        int64_t exponent {i & expoMask};
        if(exponent == expoMask) {
            int64_t mantiassa {i & mantMask};
            return mantiassa ? DBL_NAN : ((i & signMask) ? DBL_NINF : DBL_PINF);
        }
        return DBL_FINITE;
    }

    bool TGXFileObj::DoWrite(const gxdefs::TgdxUELIndex &AElements, const TgdxValues &AVals) {
        int FDim {FCurrentDim+1}, delta{};
        for(int D{}; D<FCurrentDim; D++) {
            if(!WrBitMaps[D].empty() && !WrBitMaps[D][AElements[D]]) {
                ReportError(ERR_DOMAINVIOLATION);
                TgdxUELIndex  ErrorUELs;
                for(int DD{}; DD<D-1; DD++)
                    ErrorUELs[DD] = AElements[DD];
                ErrorUELs[D] = -AElements[D];
                // see if there are more domain violations
                for(int DD{D+1}; DD < FCurrentDim; DD++) {
                    bool neg {!WrBitMaps[DD].empty() && !WrBitMaps[DD][AElements[DD]]};
                    ErrorUELs[DD] = (neg ? -1 : 1) * AElements[DD];
                }
                AddToErrorListDomErrs(ErrorUELs, AVals);
                return false;
            }
        }
        for(int D{}; D<FCurrentDim; D++) {
            delta = AElements[D] - LastElem[D];
            if(delta) {
                FDim = D + 1; // plus one to match Delphi semantics
                break;
            }
        }
        if(FDim > FCurrentDim) {
            if(FCurrentDim > 0 && DataCount >= 1) {
                ReportError(ERR_DATADUPLICATE);
                AddToErrorList(AElements, AVals);
                return false;
            }
            FFile->WriteByte(1); // keeps logic working for scalars
        } else {
            if(delta < 0) {
                ReportError(ERR_RAWNOTSORTED);
                AddToErrorList(AElements, AVals);
                return false;
            }
            if(FDim == FCurrentDim && delta <= DeltaForWrite) { // small change in last dimension
                FFile->WriteByte(FCurrentDim + delta);
                LastElem[FCurrentDim-1] = AElements[FCurrentDim-1];
            } else { // general change
                FFile->WriteByte(FDim);
                for(int D{FDim - 1}; D<FCurrentDim; D++) {
                    int v{AElements[D]-MinElem[D]};
                    switch(ElemType[D]) {
                        case sz_integer: FFile->WriteInteger(v); break;
                        case sz_word: FFile->WriteWord(v); break;
                        case sz_byte: FFile->WriteByte(v); break;
                    }
                    LastElem[D] = AElements[D];
                }
            }
        }
        if(DataSize > 0) {
            for(int DV{}; DV <= LastDataField; DV++) {
                double X {AVals[DV]};
                int64_t i64;
                TDblClass dClass {dblInfo(X, i64)};
                int xv{vm_valund};
                for(; xv < vm_normal; xv++)
                    if(i64 == intlValueMapI64[xv]) break;
                if(xv == vm_normal && dClass != DBL_FINITE) {
                    switch(dClass) {
                        case DBL_NINF: xv = vm_valmin; break;
                        case DBL_PINF: xv = vm_valpin; break;
                        case DBL_NAN: xv = vm_valna; break;
                        default: break;
                    }
                }
                FFile->WriteByte(xv);
                if(xv == vm_normal) {
                    FFile->WriteDouble(X);
                    if(X >= Zvalacr) {
                        int v = static_cast<int>(std::round(X / Zvalacr));
                        int ix = utils::indexOf<TAcronym>(AcronymList, [&v](const TAcronym &acro) { return acro.AcrMap == v; });
                        if(ix == -1) AcronymList.push_back(TAcronym{"", "", v, -1, false});
                    }
                }
            }
        }
        DataCount++;
        if(utils::in(CurSyPtr->SDataType, dt_set, dt_alias)) {
            if(AVals[vallevel] != 0.0) CurSyPtr->SSetText = true;
            if(FCurrentDim == 1 && !CurSyPtr->SSetBitMap.empty())
                CurSyPtr->SSetBitMap[LastElem.front()] = true;
        }
        return true;
    }

    bool TGXFileObj::DoRead(TgdxValues &AVals, int &AFDim) {
        const auto maybeRemap = [&](int DV, double v) -> double {
            return v >= Zvalacr ? AcronymRemap(v) : v;
        };

        if (ReadUniverse) {
            UniverseNr++;
            bool res{ UniverseNr <= UelCntOrig };
            if (res) {
                LastElem[0] = UniverseNr;
                AVals[vallevel] = 0.0;
                AFDim = 1;
            }
            return res;
        }
        if (CurSyPtr->SScalarFrst) {
            CurSyPtr->SScalarFrst = false;
            GetDefaultRecord(AVals);
            AFDim = 0;
            return true;
        }

        uint8_t B;
        FFile->Read(&B, 1);
        if(B > DeltaForRead) { // relative change in last dimension
            if (B == 255) return false;
            AFDim = FCurrentDim;
            if(FCurrentDim > 0) LastElem[FCurrentDim-1] += B - DeltaForRead;
        } else {
            AFDim = B;
            for(int D{AFDim-1}; D <FCurrentDim; D++) {
                switch(ElemType[D]) {
                    case sz_integer: LastElem[D] = FFile->ReadInteger() + MinElem[D]; break;
                    case sz_word: LastElem[D] = FFile->ReadWord() + MinElem[D]; break;
                    case sz_byte: LastElem[D] = FFile->ReadByte() + MinElem[D]; break;
                }
            }
        }
        if(DataSize > 0) {
            for(int DV{vallevel}; DV<=LastDataField; DV++) {
                uint8_t BSV;
                FFile->Read(&BSV, 1);
                TgdxIntlValTyp SV {static_cast<TgdxIntlValTyp>(BSV)};
                AVals[DV] = SV != vm_normal ? readIntlValueMapDbl[SV] : maybeRemap(DV, FFile->ReadDouble());
            }
            if(!MapSetText.empty() && AVals[vallevel] != 0.0 && CurSyPtr->SDataType == gms_dt_set) { // remap settext number
                double X {AVals[vallevel]};
                int D {static_cast<int>(std::round(X))};
                if(std::abs(X-D) < 1e-12 && D >= 0 && D < SetTextList.size())
                    AVals[vallevel] = MapSetText[D];
            }
        }
        return true;
    }

    double TGXFileObj::AcronymRemap(double V) {
        auto GetAsAcronym = [&](double V)-> double {
            int orgIndx {static_cast<int>(std::round(V / Zvalacr))};
            int N {AcronymList.FindEntry(orgIndx)};
            int newIndx{};
            if(N < 0) { // not found
                if(NextAutoAcronym <= 0) newIndx = orgIndx;
                else {
                    newIndx = NextAutoAcronym;
                    NextAutoAcronym++;
                    N = AcronymList.AddEntry("", "", orgIndx);
                    AcronymList[N].AcrReadMap = newIndx;
                    AcronymList[N].AcrAutoGen = true;
                }
            } else { // found
                newIndx = AcronymList[N].AcrReadMap;
                if(newIndx <= 0) {
                    if(NextAutoAcronym <= 0) newIndx = orgIndx;
                    else {
                        newIndx = NextAutoAcronym;
                        NextAutoAcronym++;
                        AcronymList[N].AcrReadMap = newIndx;
                        AcronymList[N].AcrAutoGen = true;
                    }
                }
            }
            return Zvalacr * newIndx;
        };

        if (V < Zvalacr)
            return V;
        else {
            if(V == 0.0) return 0.0;
            if(std::isnan(V)) return vm_valna;
            if(std::isinf(V)) return V < 0.0 ? vm_valmin : vm_valpin;
            if(std::isnormal(V)) return V < 0.0 ? V : GetAsAcronym(V);
            return intlValueMapDbl[vm_valna];
        }
    }

    void TGXFileObj::AddToErrorListDomErrs(const gxdefs::TgdxUELIndex &AElements, const TgdxValues &AVals) {
        if (ErrorList.size() >= 11) return;

        for (int D{}; D < FCurrentDim; D++) {
            int EN{ AElements[D] };
            if (EN < 0) {
                bool Found{};
                for (const auto& [keys, vals] : ErrorList) {
                    if (keys[D] == EN) {
                        Found = true;
                        break;
                    }
                }
                if (!Found) {
                    ErrorList[AElements] = AVals;
                    return;
                }
            }
        }
    }

    void TGXFileObj::AddToErrorList(const gxdefs::TgdxUELIndex &AElements, const TgdxValues &AVals) {
        if (ErrorList.size() >= 11) // avoid storing too many errors
            return;
        ErrorList[AElements] = AVals;
    }

    bool TGXFileObj::ResultWillBeSorted(const gxdefs::TgdxUELIndex &ADomainNrs) {
        for(int D{}; D<FCurrentDim; D++) {
            switch(ADomainNrs[D]) {
                case DOMC_UNMAPPED: continue;
                case DOMC_EXPAND:
                    if(UELTable.GetMapToUserStatus() == map_unsorted) return false;
                    if(!D) {
                        if(UELTable.GetMapToUserStatus() >= map_sortgrow) continue;
                        else return false;
                    } else if(UELTable.GetMapToUserStatus() == map_sortfull) continue;
                    else return false;
                    break;
                case DOMC_STRICT:
                    if(UELTable.GetMapToUserStatus() == map_unsorted) return false;
                    break;
                default:
                    if(UELTable.GetMapToUserStatus() >= map_sorted) continue;
                    TDFilter *PFilter = FilterList.FindFilter(ADomainNrs[D]);
                    if(!PFilter->FiltSorted) return false;
                    break;
            }
        }
        return true;
    }

    void TGXFileObj::GetDefaultRecord(TgdxValues &Avals) {
        int ui{};
        switch (CurSyPtr->SDataType) {
        case dt_set:
        case dt_alias:
        case dt_par:
            Avals[vallevel] = 0.0;
            break;
        case dt_var:
            ui = CurSyPtr->SUserInfo;
            Avals = ui >= stypunknwn && ui <= stypsemiint ? defrecvar[ui] : defrecvar[stypunknwn];
            break;
        case dt_equ:
            ui = CurSyPtr->SUserInfo;
            Avals = ui >= ssyeque && ui <= ssyeque + (styequb + 1) ? defrecequ[ui] : defrecequ[ssyeque];
            break;
        default:
            assert(false && "GetDefaultRecord-2");
            break;
        }
    }

    const std::map<int, std::string> errorCodeToStr {
        {ERR_NOFILE               , "File name is empty"s},
        {ERR_FILEERROR            , "File I/O error"s},
        {ERR_NOERROR              , "No error"s},
        {ERR_BADMODE              , "Bad mode"s},
        {ERR_BADDIMENSION         , "Bad dimension"s},
        {ERR_BAD_ALIAS_DIM        , "Bad dimension for aliased set"s},
        {ERR_BADELEMENTINDEX      , "Bad UEL Nr"s},
        {ERR_BADSYMBOLINDEX       , "Bad symbol number"s},
        {ERR_ELEMENTSEQUENCE      , "Element out of sequence"s},
        {ERR_DUPLICATESYMBOL      , "Duplicate symbol"s},
        {ERR_DATANOTSORTED        , "Data is not sorted"s},
        {ERR_DATADUPLICATE        , "Duplicate keys"s},
        {ERR_UNKNOWNFILTER        , "Unknown filter"s},
        {ERR_BADSTRINGFORMAT      , "Bad quotes"s},
        {ERR_BADIDENTFORMAT       , "Illegal identifier"s},
        {ERR_UELCONFLICT          , "UEL string with different index"s},
        {ERR_DUPLICATESPECVAL     , "Duplicate special value"s},
        {ERR_BADERRORRECORD       , "Bad Error record number"s},
        {ERR_DUPLICATEUEL         , "Duplicate UEL"s},
        {ERR_BADUELSTR            , "Bad UEL string"s},
        {ERR_UNDEFUEL             , "Unknown UEL"s},
        {ERR_UELSECONDWRITE       , "gdx file has UEL table already"s},
        {ERR_UELNOTEMPTY          , "UEL table is not empty"s},
        {ERR_BAD_FILTER_NR        , "Bad filter number"s},
        {ERR_BAD_FILTER_INDX      , "Bad index in filter"s},
        {ERR_FILTER_UNMAPPED      , "Unmapped index in filter"s},
        {ERR_OBSOLETE_FUNCTION    , "Use of obsolete function"s},
        {ERR_RAWNOTSORTED         , "Data not sorted when writing raw"s},
        {ERR_BADACROINDEX         , "Bad index for acronym"s},
        {ERR_BADACRONUMBER        , "Bad acronym record number"s},
        {ERR_BADACRONAME          , "Bad acronym name for update"s},
        {ERR_ACRODUPEMAP          , "Bad acronym index for update"s},
        {ERR_ACROBADADDITION      , "Bad addition to acronym table"s},
        {ERR_UNKNOWNDOMAIN        , "Unknown domain"s},
        {ERR_BADDOMAIN            , "Domain not set with dim=1"s},
        {ERR_NODOMAINDATA         , "Set has no data"s},
        {ERR_ALIASSETEXPECTED     , "Set expected for domain"s},
        {ERR_BADDATATYPE          , "Bad data type"s},
        {ERR_NOSYMBOLFORCOMMENT   , "No symbol to add comment to"s},
        {ERR_DOMAINVIOLATION      , "Domain violation"s},
        {ERR_FILEALREADYOPEN      , "File is already open"s},
        {ERR_FILETOOLDFORAPPEND   , "File version to old for append"s},
        {ERR_OPEN_DOMSMARKER1     , "Expected data marker (DOMS_1) not found in GDX file"s},
        {ERR_OPEN_DOMSMARKER2     , "Expected data marker (DOMS_2) not found in GDX file"s},
        {ERR_OPEN_DOMSMARKER3     , "Expected data marker (DOMS_3) not found in GDX file"s},
        {ERR_BADDATAMARKER_DATA   , "Expected data marker (DATA) not found in GDX file"s},
        {ERR_BADDATAMARKER_DIM    , "Expected data marker (DIM) not found in GDX file"s},
        {ERR_OPEN_BOI             , "Expected data marker (BOI) not found in GDX file"s},
        {ERR_OPEN_FILEHEADER      , "Expected data marker (FILEHEADER) not found in GDX file"s},
        {ERR_OPEN_FILEMARKER      , "Expected data marker (FILEMARKER) not found in GDX file"s},
        {ERR_OPEN_SYMBOLMARKER1   , "Expected data marker (SYMBOL_1) not found in GDX file"s},
        {ERR_OPEN_SYMBOLMARKER2   , "Expected data marker (SYMBOL_2) not found in GDX file"s},
        {ERR_OPEN_UELMARKER1      , "Expected data marker (UEL_1) not found in GDX file"s},
        {ERR_OPEN_UELMARKER2      , "Expected data marker (UEL_2) not found in GDX file"s},
        {ERR_OPEN_TEXTMARKER1     , "Expected data marker (TEXT_1) not found in GDX file"s},
        {ERR_OPEN_TEXTMARKER2     , "Expected data marker (TEXT_2) not found in GDX file"s},
        {ERR_OPEN_ACROMARKER1     , "Expected data marker (ACRO_1) not found in GDX file"s},
        {ERR_OPEN_ACROMARKER2     , "Expected data marker (ACRO_2) not found in GDX file"s},
        {ERR_OPEN_FILEVERSION     , "GDX file version not supported"s},
        {ERR_BADDATAFORMAT        , "File not recognized as a GDX file"s},
        {ERR_OUT_OF_MEMORY        , "Out of memory"s},
        {ERR_ZLIB_NOT_FOUND       , "Compression library not found"s},
        {ERR_GDXCOPY              , "GDXCOPY: Unknown error"s},
        {ERR_PARAMETER            , "GDXCOPY: Parameter error"s},
        {ERR_DLL_NOT_FOUND        , "GDXCOPY: DLL not found"s},
        {ERR_CREATE_DIR           , "GDXCOPY: Cannot create directory"s},
        {ERR_FILE_OPEN            , "GDXCOPY: File open failed"s},
        {ERR_FILE_WRITE           , "GDXCOPY: Cannot open file for write"s},
        {ERR_UEL_LENGTH           , "GDXCOPY: UEL length exceeds maximum"s},
        {ERR_UEL_REGISTER         , "GDXCOPY: Cannot register UELs"s},
        {ERR_EXPL_TEXT            , "GDXCOPY: Cannot save explanatory text"s},
        {ERR_DIMENSION            , "GDXCOPY: Dimension exceeds maximum"s},
        {ERR_WRITE_SYMBOL         , "GDXCOPY: Error writing symbol"s},
        {ERR_CLOSE_FILE           , "GDXCOPY: Error closing file"s},
        {ERR_CANNOT_DELETE        , "GDXCOPY: Cannot delete file"s},
        {ERR_CANNOT_RENAME        , "GDXCOPY: Cannot rename file"s}
    };

    int TGXFileObj::gdxErrorStr(int ErrNr, std::string &ErrMsg) {
        const auto it = errorCodeToStr.find(ErrNr);
        if(it == errorCodeToStr.end()) ErrMsg = rtl::sysutils_p3::SysErrorMessage(ErrNr);
        else ErrMsg = it->second;
        return true;
    }

    int TGXFileObj::gdxOpenRead(const std::string &FileName, int &ErrNr) {
        return gdxOpenReadXX(FileName, fmOpenRead, 0, ErrNr);
    }

    int TGXFileObj::gdxFileVersion(std::string &FileStr, std::string &ProduceStr) {
        FileStr = FileSystemID;
        ProduceStr = FProducer;
        return true;
    }

    int TGXFileObj::gdxFindSymbol(const std::string &SyId, int &SyNr) {
        if (SyId == "*") {
            SyNr = 0;
            return true;
        }
        const auto it = NameList.find(SyId);
        if (it == NameList.end()) return false;
        SyNr = (*it).second->SSyNr;
        return true;
    }

    int TGXFileObj::gdxDataReadStr(TgdxStrIndex &KeyStr, TgdxValues &Values, int &DimFrst) {
        const TgxModeSet AllowedModes{ fr_str_data };
        if ((TraceLevel >= trl_all || !utils::in(fmode, AllowedModes)) && CheckMode("DataReadStr", AllowedModes))
            return false;
        if (!DoRead(Values, DimFrst)) {
            gdxDataReadDone();
            return false;
        }
        else {
            for (int D{}; D < FCurrentDim; D++) {
                int LED{ LastElem[D] };
                KeyStr[D] = LED >= 1 && LED <= UELTable.size() ? UELTable[LED-1] : BADUEL_PREFIX + std::to_string(LED);
            }
            return true;
        }
    }

    int TGXFileObj::gdxDataReadDone() {
        TgxModeSet AllowedMode {fr_init,fr_raw_data,fr_map_data,fr_mapr_data, fr_str_data,fr_slice};
        SortList.clear();
        CurSyPtr = nullptr;
        if(!MajorCheckMode("DataReadDone", AllowedMode)) {
            fmode = fr_init;
            return false;
        }
        if(fmode == fr_slice) {
            for(int D{}; D<MaxDim; D++) {
                SliceIndxs[D].clear();
                SliceRevMap[D].clear();
            }
        }
        if(NrMappedAdded) {
            int HighestIndex = UELTable.UsrUel2Ent.rbegin()->first;
            for(int N{HighestIndex}; N >= HighestIndex - NrMappedAdded + 1; N--) {
                assert(N >= 1 && "Wrong entry number");
                int EN {UELTable.UsrUel2Ent[N]};
                /*auto d {UELTable[EN]};
                assert(d == -1 || d == N && "Mapped already");
                UELTable[EN] = N;*/
                // FIXME: Get mapped reading done fixed!
            }
            NrMappedAdded = 0;
        }
        fmode = fr_init;
        return true;
    }

    int TGXFileObj::gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) {
        static auto badLookup = [&]() {
            SyId.clear();
            Dim = -1;
            Typ = dt_set;
            return false;
        };

        if (!SyNr) {
            SyId = "*";
            Dim = 1;
            Typ = dt_set;
            return true;
        }

        if (SyNr <= 0 || SyNr > NameList.size()) return badLookup();
        
        auto maybeNameAndSym = symbolWithIndex(SyNr);
        if (maybeNameAndSym) {
            // TODO: Use destructuring of the pair here
            auto nameAndSym = *maybeNameAndSym;
            SyId = nameAndSym.first;
            Dim = nameAndSym.second->SDim;
            Typ = nameAndSym.second->SDataType;
            return true;
        }

        return badLookup();
    }

    int TGXFileObj::gdxDataReadStrStart(int SyNr, int &NrRecs) {
        TgdxUELIndex XDomains;
        XDomains.fill(DOMC_UNMAPPED);
        NrRecs = PrepareSymbolRead("DataReadStrStart"s, SyNr, XDomains, fr_str_data);
        return NrRecs >= 0;
    }

    int TGXFileObj::gdxOpenReadXX(const std::string &Afn, int filemode, int ReadMode, int &ErrNr) {
        if(fmode != f_not_open) {
            ErrNr = ERR_FILEALREADYOPEN;
            return false;
        }
        MajContext = "OpenRead"s;
        TraceLevel = trl_none;
        fmode = f_not_open;
        ReadPtr = nullptr;
        InitErrors();

        auto FileErrorNr = [&]() {
            ErrNr = LastError;
            FFile = nullptr;
            return false;
        };

        auto FileNoGood = [&]() {
            LastError = ErrNr;
            return FileErrorNr();
        };

        if(Afn.empty()) {
            ErrNr = ERR_NOFILE;
            return FileNoGood();
        }
        FFile = std::make_unique<TMiBufferedStreamDelphi>(Afn, filemode, DLLLoadPath);
        ErrNr = FFile->GetLastIOResult();
        if(ErrNr) return FileNoGood();
        if(FFile->GoodByteOrder()) {
            ErrNr = ERR_BADDATAFORMAT;
            return FileNoGood();
        }
        if( ErrorCondition(FFile->ReadByte() == gdxHeaderNr, ERR_OPEN_FILEHEADER) ||
            ErrorCondition(utils::sameText(FFile->ReadString(), gdxHeaderId), ERR_OPEN_FILEMARKER)) return FileErrorNr();
        VersionRead = FFile->ReadInteger();
        if(ErrorCondition(VersionRead >= VERSION, ERR_OPEN_FILEVERSION)) return FileErrorNr();

        int Compr {VersionRead <= 5 ? 0 : FFile->ReadInteger()};
        DoUncompress = Compr > 0;
        if(DoUncompress && !FFile->GetCanCompress()) {
            ErrNr = ERR_ZLIB_NOT_FOUND;
            return FileNoGood();
        }

        fComprLev = Compr;
        FileSystemID = FFile->ReadString();
        FProducer = FFile->ReadString();
        FProducer2.clear();

        MajorIndexPosition = FFile->GetPosition();
        if(ErrorCondition(FFile->ReadInteger() == MARK_BOI, ERR_OPEN_BOI)) return FileErrorNr();
        int64_t AcronymPos{}, DomStrPos{}, SymbPos{}, UELPos{}, SetTextPos{};

        if(VersionRead <= 5) {
            SymbPos = FFile->ReadInteger();
            UELPos = FFile->ReadInteger();
            SetTextPos = FFile->ReadInteger();
            NextWritePosition = FFile->ReadInteger();
        } else {
            SymbPos = FFile->ReadInt64();
            UELPos = FFile->ReadInt64();
            SetTextPos = FFile->ReadInt64();
            if(VersionRead >= 7) {
                AcronymPos = FFile->ReadInt64();
                NextWritePosition = FFile->ReadInt64();
                DomStrPos = FFile->ReadInt64();
            }
        }
        // reading symbol table
        FFile->SetCompression(DoUncompress);
        FFile->SetPosition(SymbPos);
        if(ErrorCondition(FFile->ReadString() == MARK_SYMB, ERR_OPEN_SYMBOLMARKER1)) return FileErrorNr();
        int NrElem {FFile->ReadInteger()};
        NameList.clear();
        AcronymList.clear();
        FilterList.clear();
        const int NrElemsOfSym = NrElem;
        for(int N{1}; N<=NrElemsOfSym; N++) {
            std::string S {FFile->ReadString()};
            CurSyPtr = new TgdxSymbRecord {};
            CurSyPtr->SPosition = VersionRead <= 5 ? FFile->ReadInteger() : FFile->ReadInt64();
            CurSyPtr->SDim = FFile->ReadInteger();
            uint8_t B {FFile->ReadByte()};
            CurSyPtr->SDataType = static_cast<TgdxDataType>(B);
            CurSyPtr->SUserInfo = FFile->ReadInteger();
            CurSyPtr->SDataCount = FFile->ReadInteger();
            CurSyPtr->SErrors = FFile->ReadInteger();
            B = FFile->ReadByte();
            CurSyPtr->SSetText = B;
            CurSyPtr->SExplTxt = FFile->ReadString();
            CurSyPtr->SIsCompressed = VersionRead > 5 && FFile->ReadByte();
            if(VersionRead >= 7) {
                if(FFile->ReadByte()) {
                    for(int D{}; D<CurSyPtr->SDim; D++)
                        CurSyPtr->SDomSymbols.emplace_back(FFile->ReadInteger());
                }
                NrElem = FFile->ReadInteger();
                if(NrElem) {
                    CurSyPtr->SCommentsList.emplace_back(FFile->ReadString());
                    NrElem--;
                }
            }
            CurSyPtr->SSetBitMap.clear();
            CurSyPtr->SDomStrings.clear();
            NameList[S] = CurSyPtr;
            CurSyPtr->SSyNr = static_cast<int>(NameList.size());
        }

        // reading UEL table
        FFile->SetCompression(DoUncompress);
        FFile->SetPosition(UELPos);
        UELTable.clear();

        if (ErrorCondition(FFile->ReadString() == MARK_UEL, ERR_OPEN_UELMARKER1))
            return FileErrorNr();

        NrElem = FFile->ReadInteger();
        if (FileSystemID.substr(15, 4) == "2001") NrElem--;

        while (UELTable.size() < NrElem) {
            UELTable.AddObject(FFile->ReadString(), -1);
        }
        UelCntOrig = UELTable.size(); // needed when reading universe

        if (ErrorCondition(FFile->ReadString() == MARK_UEL, ERR_OPEN_UELMARKER2)) return FileErrorNr();
        if (ReadMode % 2 == 0) { // reading text table
            FFile->SetCompression(DoUncompress);
            FFile->SetPosition(SetTextPos);
            SetTextList.clear();
            if(ErrorCondition(FFile->ReadString() == MARK_SETT, ERR_OPEN_TEXTMARKER1)) return FileErrorNr();
            NrElem = FFile->ReadInteger();
            SetTextList.reserve(NrElem);
            for (int N{}; N < NrElem; N++) {
                SetTextList.push_back(FFile->ReadString());
                int TextNum{ static_cast<int>(SetTextList.size()) };
                if (TextNum != N) {
                    MapSetText.resize(NrElem);
                    // TODO: Could this be replaced by std::iota?
                    for (int D{}; D < N; D++)
                        MapSetText[D] = D;
                    MapSetText[N] = TextNum;
                }
            }
        }
        if (VersionRead >= 7) {
            FFile->SetCompression(DoUncompress);
            FFile->SetPosition(AcronymPos);
            if (ErrorCondition(FFile->ReadString() == MARK_ACRO, ERR_OPEN_ACROMARKER1)) return FileErrorNr();
            AcronymList.resize(FFile->ReadInteger());
            for (int i = 0; i < AcronymList.size(); i++) {
                auto& obj = AcronymList[i];
                obj.AcrName = FFile->ReadString();
                obj.AcrText = FFile->ReadString();
                obj.AcrMap = FFile->ReadInteger();
            }
        }

        if (VersionRead >= 7 && DomStrPos) {
            FFile->SetCompression(DoUncompress);
            FFile->SetPosition(DomStrPos);
            if(ErrorCondition(FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER1)) return FileErrorNr();
            DomainStrList.resize(FFile->ReadInteger());
            for (int i = 0; i < DomainStrList.size(); i++)
                DomainStrList[i] = FFile->ReadString();
            if (ErrorCondition(FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER2)) return FileErrorNr();
            while (true) {
                int SyNr = FFile->ReadInteger();
                if (SyNr <= 0) break;
                auto maybeNameAndSym = symbolWithIndex(SyNr);
                if (maybeNameAndSym) {
                    auto sym = (*maybeNameAndSym).second;
                    for (int D{}; D < sym->SDim; D++) {
                        sym->SDomStrings[D] = FFile->ReadInteger();
                    }
                }
            }
            if (ErrorCondition(FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER3)) return FileErrorNr();
        }

        LastError = ERR_NOERROR;
        gdxResetSpecialValues();
        fmode = fr_init;
        fstatus = stat_read;
        FFile->SetCompression(false);
        return true;
    }

    std::optional<std::pair<const std::string, PgdxSymbRecord>> TGXFileObj::symbolWithIndex(int index) {
        // FIXME: This is super slow. How to get both by name and by index lookups with good time complexity?
        auto it = std::find_if(NameList.begin(), NameList.end(), [&index](const auto &pair) {
            return pair.second->SSyNr == index;
        });
        return it == NameList.end() ? std::nullopt : std::make_optional(*it);
    }

    int TGXFileObj::gdxAddAlias(const std::string &Id1, const std::string &Id2) {
        if(!MajorCheckMode("AddAlias", AnyWriteMode)) return false;
        int SyNr1 = Id1 == "*" ? std::numeric_limits<int>::max() : NameList[Id1]->SSyNr;
        int SyNr2 = Id2 == "*" ? std::numeric_limits<int>::max() : NameList[Id2]->SSyNr;
        if(ErrorCondition((SyNr1 >= 0) != (SyNr2 >= 0), ERR_ALIASSETEXPECTED)) return false;
        int SyNr;
        std::string AName;
        if(SyNr1 > 0) {
            SyNr = SyNr1;
            AName = Id2;
        } else {
            SyNr = SyNr2;
            AName = Id1;
        }
        if(SyNr == std::numeric_limits<int>::max()) SyNr = 0;
        else if(ErrorCondition(utils::in(symbolWithIndex(SyNr)->second->SDataType, dt_set, dt_alias), ERR_ALIASSETEXPECTED)) return false;
        if(!IsGoodNewSymbol(AName)) return false;
        auto SyPtr = new TgdxSymbRecord{};
        SyPtr->SDataType = dt_alias;
        SyPtr->SUserInfo = SyNr;
        if(!SyNr) {
            SyPtr->SDim = 1;
            SyPtr->SExplTxt = "Aliased with *"s;
        } else {
            SyPtr->SDim = symbolWithIndex(SyNr)->second->SDim;
            SyPtr->SExplTxt = "Aliased with "s + symbolWithIndex(SyNr)->first;
        }
        NameList[AName] = SyPtr;
        return true;
    }

    int TGXFileObj::gdxAddSetText(const std::string &Txt, int &TxtNr) {
        if(SetTextList.empty() || TraceLevel >= trl_all && !CheckMode("AddSetText", {}))
            return false;
        SetTextList.emplace_back(MakeGoodExplText(Txt));
        TxtNr = static_cast<int>(SetTextList.size());
        return true;
    }

    int TGXFileObj::gdxDataErrorCount()
    {
        return static_cast<int>(ErrorList.size());
    }

    int TGXFileObj::gdxDataErrorRecord(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values)
    {
        int res{ gdxDataErrorRecordX(RecNr, KeyInt, Values) };
        if (res) {
            for (int D{}; D < KeyInt.size(); D++) {
                if (KeyInt[D] < 0) KeyInt[D] = -KeyInt[D];
            }
        }
        return res;
    }

    int TGXFileObj::gdxDataErrorRecordX(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values)
    {
        TgxModeSet AllowedModes{ fr_init,fw_init,fr_map_data, fr_mapr_data, fw_raw_data, fw_map_data,fw_str_data };
        if ((TraceLevel >= trl_all || !utils::in(fmode, AllowedModes)) && CheckMode("DataErrorRecord", AllowedModes))
            return false;

        if (!ErrorList.empty()) {
            if (RecNr < 1 || RecNr >= ErrorList.size()) {
                ReportError(ERR_BADERRORRECORD);
            }
            else {
                ErrorList.GetRecord(RecNr - 1, KeyInt, Values);
                return true;
            }
        }

        return false;
    }

    int TGXFileObj::gdxDataReadRaw(TgdxUELIndex &KeyInt, TgdxValues &Values, int &DimFrst) {
        TgxModeSet  AllowedModes{fr_raw_data};
        if((TraceLevel >= trl_all || !utils::in(fmode, AllowedModes)) && !CheckMode("DataReadRaw", AllowedModes)) return false;
        if(!DoRead(Values, DimFrst)) gdxDataReadDone();
        else {
            std::copy(LastElem.begin(), LastElem.begin()+FCurrentDim-1, KeyInt.begin());
            return true;
        }
        return false;
    }

    int TGXFileObj::gdxDataReadRawStart(int SyNr, int &NrRecs) {
        TgdxUELIndex XDomains;
        XDomains.fill(DOMC_UNMAPPED);
        NrRecs = PrepareSymbolRead("DataReadRawStart", SyNr, XDomains, fr_raw_data);
        return NrRecs >= 0;
    }

    int TGXFileObj::gdxDataWriteRaw(const TgdxUELIndex &KeyInt, const TgdxValues &Values) {
        TgxModeSet AllowedModes {fw_raw_data};
        if(fmode == fw_dom_raw) fmode = fw_raw_data;
        if((TraceLevel >= trl_some || !utils::in(fmode, AllowedModes)) && CheckMode("DataWriteRaw", AllowedModes)) return false;
        if(DoWrite(KeyInt, Values)) return true;
        return false;
    }

    int TGXFileObj::gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                         int UserInfo) {
        if(!PrepareSymbolWrite("DataWriteRawStart", SyId, ExplTxt, Dimen, Typ, UserInfo)) return false;
        std::fill_n(MinElem.begin(), FCurrentDim-1, 0);
        std::fill_n(MaxElem.begin(), FCurrentDim-1, std::numeric_limits<int>::max());
        InitDoWrite(-1);
        fmode = fw_dom_raw;
        return true;
    }

    int TGXFileObj::gdxErrorCount() {
        return ErrCntTotal;
    }

    int TGXFileObj::gdxGetElemText(int TxtNr, std::string &Txt, int &Node) {
        Node = 0;
        if(SetTextList.empty()) {
            Txt.clear();
            return false;
        }
        if(TraceLevel >= trl_all && !CheckMode("GetElemText", {}))
            return false;
        if(TxtNr < 0 || TxtNr >= SetTextList.size()) {
            Txt = BADStr_PREFIX + std::to_string(TxtNr);
        } else {
            Txt = SetTextList[TxtNr];
            //FIXME: Node = ???
            return true;
        }
        return false;
    }

    int TGXFileObj::gdxGetLastError() {
        if(!FFile) {
            int le{LastError};
            LastError = ERR_NOERROR;
            return le;
        } else {
            int res{FFile->GetLastIOResult()};
            if(res == ERR_NOERROR) {
                res = LastError;
                LastError = ERR_NOERROR;
            }
            return res;
        }
    }

    int TGXFileObj::gdxGetSpecialValues(TgdxSVals &AVals) {
        AVals[sv_valund] = intlValueMapDbl[vm_valund] ;
        AVals[sv_valna ] = intlValueMapDbl[vm_valna ] ;
        AVals[sv_valpin] = intlValueMapDbl[vm_valpin] ;
        AVals[sv_valmin] = intlValueMapDbl[vm_valmin] ;
        AVals[sv_valeps] = intlValueMapDbl[vm_valeps] ;
        AVals[sv_acronym] = Zvalacr;
        return true;
    }

    int TGXFileObj::gdxSetSpecialValues(const TgdxSVals &AVals) {
        TIntlValueMapDbl tmpDbl{ intlValueMapDbl };

        tmpDbl[vm_valund] = AVals[sv_valund];
        tmpDbl[vm_valna] = AVals[sv_valna];
        tmpDbl[vm_valpin] = AVals[sv_valpin];
        tmpDbl[vm_valmin] = AVals[sv_valmin];
        tmpDbl[vm_valeps] = AVals[sv_valeps];

        TIntlValueMapI64 tmpI64;
        copyIntlMapDblToI64(tmpDbl, tmpI64);

        // check for duplicates using the int64 version of the map
        const TgdxIntlValTyp stopper = vm_valeps;
        for (int iv1{ vm_valund }; iv1 <= stopper; iv1++) {
            for (int iv2{ iv1+1 }; iv2 <= stopper; iv2++) {
                if (tmpI64[iv1] == tmpI64[iv2]) {
                    ReportError(ERR_DUPLICATESPECVAL);
                    return false;
                }
            }
        }
        
        intlValueMapDbl = tmpDbl;
        readIntlValueMapDbl = intlValueMapDbl;
        intlValueMapI64 = tmpI64;
        return true;
    }

    int TGXFileObj::gdxSymbolGetDomain(int SyNr, TgdxUELIndex &DomainSyNrs) {
        if (ErrorCondition(SyNr >= 1 && SyNr <= NameList.size(), ERR_BADSYMBOLINDEX)) return false;
        PgdxSymbRecord SyPtr{ (*symbolWithIndex(SyNr)).second };
        for (int D{}; D < SyPtr->SDim; D++)
            DomainSyNrs[D] = SyPtr->SDomSymbols.empty() || SyPtr->SDomSymbols[D] == 0 ? 0 : SyPtr->SDomSymbols[D];
        return true;
    }

    int TGXFileObj::gdxSymbolGetDomainX(int SyNr, TgdxStrIndex &DomainIDs) {
        if (ErrorCondition(!NameList.empty() && SyNr >= 1 && SyNr <= NameList.size(), ERR_BADSYMBOLINDEX)) return 0;
        PgdxSymbRecord SyPtr{ (*symbolWithIndex(SyNr)).second };
        
        for (int D{}; D < SyPtr->SDim; D++)
            DomainIDs[D] = "*";

        if (!SyPtr->SDomStrings.empty()) {
            for (int D{}; D<SyPtr->SDim; D++)
                DomainIDs[D] = DomainStrList[SyPtr->SDomStrings[D]];
            return 2;
        }
        else if (!SyPtr->SDomSymbols.empty())
            return 1;
        else {
            for (int D{}; D < SyPtr->SDim; D++)
                if (SyPtr->SDomSymbols[D])
                    DomainIDs[D] = (*symbolWithIndex(SyPtr->SDomSymbols[D])).first;
            return 3;
        }
        return 0;
    }

    int TGXFileObj::gdxSymbolDim(int SyNr) {
        if (!SyNr) return 1;
        return NameList.empty() || SyNr < 1 || SyNr > NameList.size() ? -1 : (*symbolWithIndex(SyNr)).second->SDim;
    }

    int TGXFileObj::gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, std::string &ExplTxt) {
        if (!SyNr) {
            RecCnt = UelCntOrig;
            UserInfo = 0;
            ExplTxt = "Universe";
            return true;
        }
        else if (NameList.empty() || SyNr < 1 || SyNr > NameList.size()) {
            RecCnt = UserInfo = 0;
            ExplTxt.clear();
            return false;
        }
        else {
            const auto obj = (*symbolWithIndex(SyNr)).second;
            RecCnt = !obj->SDim ? 1 : obj->SDataCount; // scalar trick
            UserInfo = obj->SUserInfo;
            ExplTxt = obj->SExplTxt;
            return true;
        }
    }

    int TGXFileObj::gdxSymbolSetDomain(const TgdxStrIndex &DomainIDs) {
        int res{ false }, SyNr;
        TgxModeSet AllowedModes{ fw_dom_raw, fw_dom_map, fw_dom_str };
        if (!MajorCheckMode("SymbolSetDomain", AllowedModes) || !CurSyPtr) return res;
        assert(!CurSyPtr->SDomSymbols.empty() && "SymbolSetDomain");
        CurSyPtr->SDomSymbols.resize(CurSyPtr->SDim + 1);
        for (int D{}; D < CurSyPtr->SDim; D++) {
            bool domap = true;
            int DomSy;
            if (DomainIDs[D] == "*") DomSy = 0;
            else {
                DomSy = NameList[DomainIDs[D]]->SSyNr;
                if (DomSy <= -1) {
                    ReportError(ERR_UNKNOWNDOMAIN);
                    DomSy = -1;
                    res = false;
                }
            }
            if (DomSy > 0) {
                SyNr = DomSy;
                do {
                    const auto &obj = (*symbolWithIndex(SyNr)).second;
                    if (obj->SDataType == dt_set) break;
                    if (obj->SDataType == dt_alias) {
                        SyNr = obj->SUserInfo;
                        if (SyNr > 0) continue;
                        domap = false;
                        break;
                    }
                    ReportError(ERR_ALIASSETEXPECTED);
                    res = false;
                    DomSy = 0;
                    break;
                } while (true);
            }
            CurSyPtr->SDomSymbols[D] = DomSy;
            if (domap && DomSy > 0) {
                // this is the case for set i(i)
                if (CurSyPtr->SDim != 1 || CurSyPtr != (*symbolWithIndex(DomSy)).second)
                    WrBitMaps[D] = ((*symbolWithIndex(SyNr)).second)->SSetBitMap;
            }
        }
        switch (fmode) {
        case fw_dom_raw: fmode = fw_raw_data; break;
        case fw_dom_map: fmode = fw_map_data; break;
        case fw_dom_str: fmode = fw_str_data; break;
        }
        return res;
    }

    int TGXFileObj::gdxSymbolSetDomainX(int SyNr, const TgdxStrIndex& DomainIDs) {
        // check for write or append only
        if (ErrorCondition(SyNr >= 1 && SyNr <= NameList.size(), ERR_BADSYMBOLINDEX)) return false;
        PgdxSymbRecord SyPtr = (*symbolWithIndex(SyNr)).second;
        if (SyPtr->SDim > 0) {
            if (SyPtr->SDomStrings.empty()) {
                SyPtr->SDomStrings.resize(SyPtr->SDim + 1);
            }
            for (int D{}; D < SyPtr->SDim; D++) {
                std::string S{ DomainIDs[D] };
                if (S.empty() || S == "*") SyPtr->SDomStrings[D] = 0;
                else if (!IsGoodIdent(S)) {
                    SyPtr->SDomStrings[D] = 0;
                }
                else {
                    SyPtr->SDomStrings[D] = utils::indexOf(DomainStrList, S);
                    if (SyPtr->SDomStrings[D] <= -1)
                        DomainStrList.push_back(S);
                    SyPtr->SDomStrings[D] = (int)DomainStrList.size();
                }
            }
        }
        return true;
    }

    int TGXFileObj::gdxSystemInfo(int &SyCnt, int &UelCnt) {
        UelCnt = (int)UELTable.size();
        SyCnt = (int)NameList.size();
        return true;
    }

    int TGXFileObj::gdxUELRegisterDone() {
        TgxModeSet AllowedModes{ f_raw_elem,f_map_elem,f_str_elem };
        if (!MajorCheckMode("UELRegisterDone", AllowedModes)) return false;
        fmode = fmode_AftReg;
        return true;
    }

    int TGXFileObj::gdxUELRegisterRaw(const std::string &Uel) {
        TgxModeSet AllowedModes{ f_raw_elem };
        if (TraceLevel >= trl_all || !utils::in(fmode, AllowedModes) && CheckMode("UELRegisterRaw", AllowedModes))
            return false;
        std::string SV = utils::trimRight(Uel);
        if (ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
        UELTable.AddObject(SV, -1); // should about existing mapping?
        return true;
    }

    int TGXFileObj::gdxUELRegisterRawStart() {
        TgxModeSet AllowedModes { fw_init };
        if (!MajorCheckMode("UELRegisterRawStart", AllowedModes)) return false;
        fmode_AftReg = fw_init;
        fmode = f_raw_elem;
        return true;
    }

    int TGXFileObj::gdxUELRegisterStr(const std::string &Uel, int &UelNr) {
        const TgxModeSet AllowedModes{ f_str_elem };
        if ((TraceLevel >= trl_all || !utils::in(fmode, AllowedModes)) && CheckMode("UELRegisterStr", AllowedModes))
            return false;
        std::string SV{ utils::trimRight(Uel) };
        if (ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
        UelNr = UELTable.AddUsrNew(SV);
        return true;
    }

    int TGXFileObj::gdxUELRegisterStrStart() {
        TgxModeSet AllowedModes{ fr_init, fw_init };
        if (!MajorCheckMode("UELRegisterStrStart", AllowedModes)) return false;
        fmode_AftReg = fmode == fw_init ? fw_init : fr_init;
        fmode = f_str_elem;
        return true;
    }

    int TGXFileObj::gdxUMUelGet(int UelNr, std::string &Uel, int &UelMap) {
        if (UELTable.size() && UelNr >= 1 && UelNr <= UELTable.size()) {
            Uel = UELTable[UelNr-1];
            //UelMap = UELTable.GetUserMap
            // FIXME: Step through interactive debugger in order to figure out how UELTable is actually used in Delphi
            UelMap = UELTable.UsrUel2Ent.empty() ? -1 : 0;
            return true;
        }
        else {
            Uel = BADUEL_PREFIX + std::to_string(UelNr);
            UelMap = -1;
            return false;
        }
    }

    int TGXFileObj::gdxUMUelInfo(int &UelCnt, int &HighMap) {
        if (!UELTable.size()) {
            UelCnt = HighMap = 0;
            return false;
        }
        else {
            UelCnt = UELTable.size();
            HighMap = UELTable.UsrUel2Ent.empty() ? 0 : UELTable.UsrUel2Ent.rbegin()->first; // highest index
            return true;
        }
    }

    int TGXFileObj::gdxCurrentDim() {
        return FCurrentDim;
    }

    int TGXFileObj::gdxRenameUEL(const std::string &OldName, const std::string &NewName) {
        // ...
        STUBWARN();
        return 0;
    }

    void TUELTable::clear() {
        UsrUel2Ent.clear();
        uelNames.clear();
    }

    int TUELTable::size() const {
        return static_cast<int>(uelNames.size());
    }

    const std::vector<std::string> &TUELTable::getNames() {
        return uelNames;
    }

    int TUELTable::IndexOf(const std::string &s) const {
        return utils::indexOf(uelNames, s);
    }

    int TUELTable::AddObject(const std::string &id, int mapping) {
        uelNames.push_back(id);
        return static_cast<int>(uelNames.size()) - 1;
    }

    void initialization() {
        palxxx::gdlaudit::gdlSetSystemName("GDX Library");
        DLLLoadPath.clear();
        /*do this until P3 accepts large constants like $80000000000000000*/
        signMask = 0x80000000;
        signMask = signMask << 32;
        expoMask = 0x7ff00000;
        expoMask = expoMask << 32;
        mantMask = ~(signMask | expoMask);
    }

    void finalization() {
    }

    int TAcronymList::FindEntry(int Map) const {
        const auto it = std::find_if(begin(), end(), [&](const auto &item) {
            return item.AcrMap == Map;
        });
        return it == end() ? -1 : static_cast<int>(std::distance(begin(), it));
    }

    int TAcronymList::AddEntry(const std::string &Name, const std::string &Text, int Map) {
        TAcronym acr {};
        acr.AcrName = Name;
        acr.AcrText = Text;
        acr.AcrMap = Map;
        emplace_back(acr);
        return static_cast<int>(size())-1;
    }

    int TAcronymList::FindEntry(const std::string &Name) const {
        const auto it = std::find_if(begin(), end(), [&](const auto &item) {
            return item.AcrName == Name;
        });
        return it == end() ? -1 : static_cast<int>(std::distance(begin(), it));
    }

    TDFilter *TFilterList::FindFilter(int Nr) {
        const auto it = std::find_if(begin(), end(),
                                     [&Nr](const auto &f) { return f.FiltNumber == Nr; });
        return it == end() ? nullptr : &(*it);
    }

    UNIT_INIT_FINI();
}