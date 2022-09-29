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
#include "yaml.h"

#include <cassert>

using namespace gdxinterface;
using namespace gdlib::gmsstrm;
using namespace global::gmsspecs;
using namespace std::literals::string_literals;
using namespace gxdefs;
using namespace gdlib::gmsglob;

namespace gxfile {

    NullBuffer null_buffer;

    //version = 5 has 32 bit offsets and no compression
    //version = 6 introduced compression
    //version 7 introduces:
    //      acronyms
    //      longer symbol names
    //      aliases
    //later without bumping version
    //      relaxed domains
    const int   VERSION = 7, //--file version
                gdxHeaderNr = 123; //--patterns to recognize
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

    // If both single and double quotes appear in the string, replace
    // each quote by the first quote seen
    // Replace control character with a question mark
    std::string MakeGoodExplText(const std::string &s) {
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

    bool IsGoodIdent(const std::string &S) {
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

    // for double input x, return the bits as i64
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
        // Note: the default is disabled
        return s.empty() || s == "N" || s == "0" ? 0 : 1;
    }

    // Brief:
    //   Open a new gdx file for output; uses the environment variable GDXCOMPRESS to set compression argument for gdxOpenWriteEx
    // Arguments:
    //   FileName:  File name of the gdx file to be created
    //   Producer:  Name of program that creates the gdx file
    //   ErrNr: Returns an error code or zero if there is no error
    // Returns:
    //   Returns non-zero if the file can be opened; zero otherwise
    // Description:
    //   See gdxOpenWriteEx
    // See Also:
    //   gdxOpenRead, gdxOpenWriteEx, Destroy
    int TGXFileObj::gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) {
        return gdxOpenWriteEx(FileName, Producer, GetEnvCompressFlag(), ErrNr);
    }

    // Brief:
    //   Create a gdx file for writing
    // Arguments:
    //   FileName:  File name of the gdx file to be created
    //   Producer:  Name of program that creates the gdx file
    //   Compr: Zero for no compression; non-zero uses compression if available
    //          Important! when writing compressed, set the AutoConvert flag to zero
    //          so the file is not uncompressed after the Close; see gdxAutoConvert
    //   ErrNr: Returns an error code or zero if there is no error
    // Returns:
    //   Returns non-zero if the file can be opened; zero otherwise
    // Description:
    //   Open a new gdx file for output. If a file extension is not
    //   supplied, the extension '.gdx' will be used. The return code is
    //   a system dependent I/O error.
    // See Also:
    //   gdxOpenRead, gdxOpenWrite, gdxAutoConvert, Destroy
    // Example:
    // <CODE>
    //   var
    //      ErrNr: integer;
    //      PGX  : PGXFile;
    //      Msg  : ShortString;
    //   begin
    //   if not gdxGetReady(Msg)
    //   then
    //      begin
    //      WriteLn('Cannot load GDX library, msg: ', Msg);
    //      exit;
    //      end;
    //   gdxOpenWriteEx(PGX,'c:\\mydata\\file1.gdx','Examples', 1, ErrCode);
    //   gdxAutoConvert(PGX, 0);
    //   if ErrCode <> 0
    //   then
    //      [ ... ]
    // </CODE>
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

        if(writeAsText) {
            const std::string FileNameText = utils::replaceSubstrs(FileName, ".gdx", ".txt");
            FFile->ActiveWriteOpTextDumping(FileNameText);
        }

        Compr &= (FFile->GetCanCompress() ? 1 : 0);
        fComprLev = Compr;
        CompressOut = Compr > 0;
        fmode = f_not_open;
        //ReadPtr = nullptr;
        MajContext = "OpenWrite";
        TraceLevel = defaultTraceLevel;
        InitErrors();
        NameList.clear();
        NameListOrdered.clear();
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

        const std::string FileNameYML = utils::replaceSubstrs(FileName, ".gdx", ".yaml");
        YFile = std::make_unique<yaml::TYAMLFile>(FileNameYML, writeAsYAML);
        if(writeAsYAML) {
            YFile->AddKey("gdxfile");
            YFile->IncIndentLevel();
            YFile->AddKeyItem("metadata");
            YFile->IncIndentLevel();
            YFile->AddKeyValue("gdx_header_nr", gdxHeaderNr);
            YFile->AddKeyValue("gdx_header_id", gdxHeaderId);
            YFile->AddKeyValue("version_read", VersionRead);
            YFile->AddKeyValue("compression", Compr);
            YFile->AddKeyValue("filesystem_id", FileSystemID);
            YFile->AddKeyValue("producer", FProducer);
            YFile->DecIndentLevel();
        }

        return true;
    }

    // Brief:
    //   Start writing a new symbol in string mode
    // Arguments:
    //   SyId: Name of the symbol
    //   ExplTxt: Explanatory text for the symbol
    //   Dimen: Dimension of the symbol
    //   Typ: Type of the symbol
    //   UserInfo: See gdxDataWriteRawStart for more information
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Description:
    // See Also:
    //   gdxDataWriteStr, gdxDataWriteDone
    int TGXFileObj::gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) {
        if(!PrepareSymbolWrite("DataWriteStrStart", SyId, ExplTxt, Dim, Typ, UserInfo)) return false;
        /*for (int D{}; D < FCurrentDim; D++)
            LastStrElem[D] = (char)0xFF;*/
        SortList = std::make_unique<gdlib::datastorage::TLinkedData<gxdefs::TgdxValues>>(FCurrentDim, static_cast<int>(DataSize * sizeof(double)));
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
    int TGXFileObj::gdxDataWriteStr(const char **KeyStr, const double *Values) {
        const TgxModeSet AllowedModes {fw_str_data};
        if(fmode == fw_dom_str) fmode = fw_str_data;
        if(TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) {
            if(!CheckMode("DataWriteStr", AllowedModes)) return false;
            std::cout << "  Index =\n";
            for(int D{}; D<FCurrentDim; D++)
                std::cout << " " << KeyStr[D] << (D+1 < FCurrentDim ? "," : "") << "\n";
        }
        for(int D{}; D<FCurrentDim; D++) {
            std::string SV {utils::trimRight(KeyStr[D])};
            if(SV != LastStrElem[D]) {
                // 0=not found, >=1 found
                int KD {UELTable.IndexOf(SV)+1};
                if(!KD) {
                    if(ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
                    KD = UELTable.AddObject(SV, -1);
                }
                LastElem[D] = KD;
                LastStrElem[D] = SV;
                if(KD < MinElem[D]) MinElem[D] = KD;
                if(KD > MaxElem[D]) MaxElem[D] = KD;
            }
        }
        SortList->AddItem(LastElem, utils::asArray<double, 5>( Values ));
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
            InitDoWrite(static_cast<int>(SortList->size()));
            SortList->Sort();
            for(const auto &[keys, values] : *SortList) {
                DoWrite(keys.data(), values.data());
            }
            SortList->clear();
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
        if(utils::in(fmode, fw_raw_data, fw_map_data, fw_str_data)) // unfinished write
            gdxDataWriteDone();
        if(fmode == fw_init) {
            fnConv = FFile->GetFileName();
            FFile->SetCompression(CompressOut);
            FFile->SetPosition(NextWritePosition);
            int64_t SymbPos = NextWritePosition;
            FFile->WriteString(MARK_SYMB);
            FFile->WriteInteger(static_cast<int>(NameList.size()));
            YFile->AddKeyItem("symbols");
            YFile->IncIndentLevel();
            for(const auto &name : NameListOrdered) {
                const auto &PSy = NameList[name];
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

                if(writeAsYAML) {
                    YFile->AddKey(name);
                    YFile->IncIndentLevel();
                    YFile->AddKeyValue("position", static_cast<int>(PSy->SPosition));
                    YFile->AddKeyValue("dim", PSy->SDim);
                    YFile->AddKeyValue("type", PSy->SDataType);
                    YFile->AddKeyValue("user_info", PSy->SUserInfo);
                    YFile->AddKeyValue("num_records", PSy->SDataCount);
                    YFile->AddKeyValue("error_count", PSy->SErrors);
                    YFile->AddKeyValue("set_text", PSy->SSetText);
                    YFile->AddKeyValue("explanatory_text", PSy->SExplTxt);
                    YFile->AddKeyValue("compressed", PSy->SIsCompressed);
                }

                YFile->AddKeyValue("has_domain_symbols", (PSy->SDomSymbols.empty() ? "no"s : "yes"s));
                FFile->WriteByte(PSy->SDomSymbols.empty() ? 0 : 1);

                if(!PSy->SDomSymbols.empty()) {
                    YFile->AddKeyItem("domain_symbols");
                    YFile->IncIndentLevel();
                    for(int D{}; D<PSy->SDim; D++) {
                        FFile->WriteInteger(PSy->SDomSymbols[D]);
                        YFile->AddItem(std::to_string(PSy->SDomSymbols[D]));
                    }
                    YFile->DecIndentLevel();
                }

                int CommCnt {static_cast<int>(PSy->SCommentsList.size())};
                FFile->WriteInteger(CommCnt);
                if(CommCnt) {
                    YFile->AddKeyItem("comments");
                    YFile->IncIndentLevel();
                    for (int Cnt{}; Cnt < CommCnt; Cnt++) {
                        FFile->WriteString(PSy->SCommentsList[Cnt]);
                        YFile->AddItem(PSy->SCommentsList[Cnt]);
                    }
                    YFile->DecIndentLevel();
                }

                if(writeAsYAML) YFile->DecIndentLevel();
            }
            FFile->WriteString(MARK_SYMB);
            YFile->DecIndentLevel();

            auto SetTextPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_SETT);
            YFile->AddKeyItem("set_texts");
            YFile->IncIndentLevel();
            FFile->WriteInteger(static_cast<int>(SetTextList.size()));
            if(!SetTextList.empty()) {
                for (const auto &SetText: SetTextList) {
                    FFile->WriteString(SetText);
                    YFile->AddItem(SetText);
                }
            }
            FFile->WriteString(MARK_SETT);
            YFile->DecIndentLevel();

            auto UELPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_UEL);
            YFile->AddKeyItem("uels");
            YFile->IncIndentLevel();
            FFile->WriteInteger(UELTable.size());
            if(!UELTable.empty()) {
                for (const auto &uelName: UELTable.getNames()) {
                    FFile->WriteString(uelName);
                    YFile->AddItem(uelName);
                }
            }
            FFile->WriteString(MARK_UEL);
            YFile->DecIndentLevel();

            auto AcronymPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_ACRO);
            YFile->AddKeyItem("acronyms");
            YFile->IncIndentLevel();
            FFile->WriteInteger(static_cast<int>(AcronymList.size()));
            for(const auto &acro : AcronymList) {
                const auto acroName = acro.AcrName.empty() ? "UnknownACRO" + std::to_string(acro.AcrMap) : acro.AcrName;
                FFile->WriteString(acroName);
                FFile->WriteString(acro.AcrText);
                FFile->WriteInteger(acro.AcrMap);
                YFile->AddKeyValue(acroName, "{text:"s +  acro.AcrText + ", map:"s + std::to_string(acro.AcrMap) + "}"s);
            }
            FFile->WriteString(MARK_ACRO);
            YFile->DecIndentLevel();

            auto DomStrPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_DOMS);
            FFile->WriteInteger(static_cast<int>(DomainStrList.size()));
            YFile->AddKeyItem("domain_strings");
            YFile->IncIndentLevel();
            for(const auto &DomStr : DomainStrList) {
                FFile->WriteString(DomStr);
                YFile->AddItem(DomStr);
            }
            FFile->WriteString(MARK_DOMS);
            YFile->DecIndentLevel();

            YFile->AddKeyItem("symbol_domains");
            YFile->IncIndentLevel();
            int ix{1};
            for(const auto &name : NameListOrdered) {
                const auto &PSy = NameList[name];
                if(!PSy->SDomStrings.empty()) {
                    YFile->AddKeyItem(name);
                    YFile->IncIndentLevel();
                    FFile->WriteInteger(ix);
                    for(const auto &i : PSy->SDomStrings) {
                        FFile->WriteInteger(i);
                        YFile->AddItem(std::to_string(i));
                    }
                    YFile->DecIndentLevel();
                }
                ix++;
            }
            FFile->WriteInteger(-1);
            FFile->WriteString(MARK_DOMS);
            YFile->DecIndentLevel();

            // This must be at the very end!!!
            FFile->SetPosition(MajorIndexPosition);
            FFile->SetCompression(false);
            FFile->WriteInteger(MARK_BOI);
            // Note that we have room for 10 indices; if we need more, create an overflow link in the 10th position.
            const std::array<int64_t, 6> offsets = {SymbPos, UELPos, SetTextPos, AcronymPos, NextWritePosition, DomStrPos};
            const std::array offsetNames = {"symbol_table"s, "uel_table"s, "set_texts"s, "acronyms"s, "next_write"s, "domain_strs"s };
            for(int64_t offset : offsets)
                FFile->WriteInt64(offset);

            YFile->AddKeyItem("section_offsets");
            YFile->IncIndentLevel();
            for(int i{}; i<offsets.size(); i++)
                YFile->AddKeyValue(offsetNames[i], (int)offsets[i]);
            YFile->DecIndentLevel();
        }

        int res{FFile ? FFile->GetLastIOResult() : 1};

        // Many free operations. Some not necessary anymore due to RAII pattern (out of scope -> destroy)
        for(const auto &[name, psy] : NameList)
            delete psy;

        FFile = nullptr;
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

    // Brief:
    //   Reset the internal values for special values
    // Returns:
    //   Always non-zero
    // See Also:
    //   gdxSetSpecialValues, gdxGetSpecialValues
    // Description:
    //
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
        if(SortList) SortList->clear();

        if(!MajorCheckMode(Caller, AllowedModes)) return false;

        if(TraceLevel >= TraceLevels::trl_some)
            WriteTrace("Symbol = "s + AName + ", Dim = "s + std::to_string(ADim));

        if(!IsGoodNewSymbol(AName)) return false;

        if( ErrorCondition(ADim >= 0 && ADim <= global::gmsspecs::MaxDim, ERR_BADDIMENSION) ||
            ErrorCondition(AType >= 0 && AType <= dt_equ, ERR_BADDATATYPE)) return false;
        CurSyPtr = new TgdxSymbRecord{};
        auto &obj = CurSyPtr;
        obj->SPosition = CurSyPtr->SDataCount = CurSyPtr->SErrors = 0; // Position
        obj->SDim = ADim;
        obj->SDataType = static_cast<TgdxDataType>(AType);
        obj->SUserInfo = AUserInfo;
        obj->SSetText = false;
        obj->SExplTxt = MakeGoodExplText(AText);
        obj->SIsCompressed = CompressOut && ADim > 0;
        obj->SCommentsList.clear();
        obj->SDomSymbols.clear();
        obj->SDomStrings.clear();
        obj->SSetBitMap = utils::in((TgdxDataType)AType, dt_set, dt_alias) && ADim == 1 && StoreDomainSets ?
                std::make_optional<std::vector<bool>>() : std::nullopt;

        CurSyPtr->SSyNr = static_cast<int>(NameListOrdered.size()+1); // +1 for universe
        NameList[AName] = CurSyPtr;
        NameListOrdered.push_back(AName);
        FCurrentDim = ADim;
        // old case; we never write V6
        // V = 0..Dim which dimension changed
        // V = Dim+1 .. 254 change in last dimension = V - Dim
        // 255 is EOF
        DeltaForWrite = 255 - (VERSION <= 6 ? MaxDimV148 : FCurrentDim) - 1;

        DataSize = gxdefs::DataTypSize[AType];
        if(DataSize > 0)
            LastDataField = static_cast<tvarvaltype>(DataSize - 1);

        for(int D{}; D<FCurrentDim; D++) {
            LastElem[D] = INDEX_INITIAL;
            MinElem[D] = std::numeric_limits<int>::max();
            MaxElem[D] = 0;
            WrBitMaps[D] = std::nullopt;
        }

        FFile->SetCompression(CurSyPtr->SIsCompressed);
        return true;
    }

    bool TGXFileObj::MajorCheckMode(const std::string &Routine, const TgxModeSet &MS) {
        MajContext = Routine;
        LastRepError = ERR_NOERROR;
        return !((TraceLevel >= TraceLevels::trl_some || !utils::in(fmode, MS)) && !CheckMode(Routine, MS));
    }

    void TGXFileObj::WriteTrace(const std::string &s) {
        std::cout << "gdxTrace " << TraceStr << ": " << s << '\n';
    }

    bool TGXFileObj::IsGoodNewSymbol(const std::string &s) {
        return !(ErrorCondition(NameList.find(s) == NameList.end(), ERR_DUPLICATESYMBOL) ||
                 ErrorCondition(utils::indexOf<TAcronym>(AcronymList, [&s](auto acro) { return acro.AcrName == s; }) == -1, ERR_DUPLICATESYMBOL) ||
                 ErrorCondition(IsGoodIdent(s), ERR_BADIDENTFORMAT));
    }

    bool TGXFileObj::ErrorCondition(bool cnd, int N) {
        if(!cnd) ReportError(N);
        return !cnd;
    }

    void TGXFileObj::ReportError(int N) {
        if(TraceLevel >= TraceLevels::trl_errors && N != LastRepError) {
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

    int TGXFileObj::PrepareSymbolRead(const std::string& Caller, int SyNr, const int *ADomainNrs, TgxFileMode newmode) {
        if (utils::in(fmode, fr_str_data, fr_map_data, fr_mapr_data, fr_raw_data))
            gdxDataReadDone();
        int res{-1};
        NrMappedAdded = 0;
        TIntegerMapping ExpndList;
        ErrorList.clear();
        CurSyPtr = nullptr;
        if(SortList) SortList->clear();

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
                    while(DoRead(Avals.data(), AFDim)) {
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
                            AddToErrorListDomErrs(LastElem, Avals.data()); // unmapped
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
                            SortList->AddItem(AElements, Avals);
                        }
                    }
                    SortList->Sort();
                    res = (int)SortList->size();
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
            SortList->clear();
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

        if(writeAsYAML) {
            YFile->AddKeyItem("sym_write_header");
            YFile->IncIndentLevel();
            YFile->AddKeyValue("symbol_index", CurSyPtr->SSyNr);
            YFile->AddKeyValue("dim", FCurrentDim);
            YFile->AddKeyValue("nr_recs", NrRecs);
            for (int D{}; D < FCurrentDim; D++) {
                YFile->AddKeyValue("min_elem_" + std::to_string(D), MinElem[D]);
                YFile->AddKeyValue("max_elem_" + std::to_string(D), MaxElem[D]);
            }
            YFile->DecIndentLevel();
        }
    }

    /* we have to make these mask "constants" vars since we cannot
    * have large constants on input
    */
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

    /* Assume the IEEE double looks like
            * s eeeeeeeeeee mmm....mmm
            * ^                        signbit
    *   ^^^^^^^^^^^            11 bits in the exponent
    *               ^^^^^^^^^^ 52 bits in the mantissa
    * Possible values include:
    *  1. +/-    INF: exp=all ones, mant=0
                                         *  2.        Nan: exp=all ones, mant nonzero
    *  3.  otherwise: finite
    */
    // for double input x, return the bits (in i) and the class of x
    TDblClass dblInfo(double x, int64_t &i) {
        TI64Rec i64Rec{};
        i64Rec.x = x;
        i = i64Rec.i64;
        int64_t exponent {i & expoMask};
        if(exponent == expoMask) {
            int64_t mantissa {i & mantMask};
            return mantissa ? DBL_NAN : ((i & signMask) ? DBL_NINF : DBL_PINF);
        }
        return DBL_FINITE;
    }

    inline bool accessBitMap(const std::vector<bool> &bmap, int index) {
        return !(index < 0 || index >= bmap.size()) && bmap[index];
    }

    bool TGXFileObj::DoWrite(const int* AElements, const double* AVals)
    {
        int FDim{ FCurrentDim + 1 }, delta{};
        for (int D{}; D < FCurrentDim; D++) {
            if (WrBitMaps[D] && !accessBitMap(*WrBitMaps[D], AElements[D])) {
                ReportError(ERR_DOMAINVIOLATION);
                TgdxUELIndex  ErrorUELs;
                for (int DD{}; DD < D - 1; DD++)
                    ErrorUELs[DD] = AElements[DD];
                ErrorUELs[D] = -AElements[D];
                // see if there are more domain violations
                for (int DD{ D + 1 }; DD < FCurrentDim; DD++) {
                    bool neg{ WrBitMaps[DD] && !accessBitMap(*WrBitMaps[DD],AElements[DD]) };
                    ErrorUELs[DD] = (neg ? -1 : 1) * AElements[DD];
                }
                AddToErrorListDomErrs(ErrorUELs, AVals);
                return false;
            }
        }
        for (int D{}; D < FCurrentDim; D++) {
            delta = AElements[D] - LastElem[D];
            if (delta) {
                FDim = D + 1; // plus one to match Delphi semantics
                break;
            }
        }
        if (FDim > FCurrentDim) {
            if (FCurrentDim > 0 && DataCount >= 1) {
                ReportError(ERR_DATADUPLICATE);
                AddToErrorList(AElements, AVals);
                return false;
            }
            FFile->WriteByte(1); // keeps logic working for scalars
        }
        else {
            if (delta < 0) {
                ReportError(ERR_RAWNOTSORTED);
                AddToErrorList(AElements, AVals);
                return false;
            }
            if (FDim == FCurrentDim && delta <= DeltaForWrite) { // small change in last dimension
                FFile->WriteByte(FCurrentDim + delta);
                LastElem[FCurrentDim - 1] = AElements[FCurrentDim - 1];
            }
            else { // general change
                FFile->WriteByte(FDim);
                for (int D{ FDim - 1 }; D < FCurrentDim; D++) {
                    int v{ AElements[D] - MinElem[D] };
                    switch (ElemType[D]) {
                    case sz_integer: FFile->WriteInteger(v); break;
                    case sz_word: FFile->WriteWord(v); break;
                    case sz_byte: FFile->WriteByte(v); break;
                    }
                    LastElem[D] = AElements[D];
                }
            }
        }
        if (DataSize > 0) {
            for (int DV{}; DV <= LastDataField; DV++) {
                double X{ AVals[DV] };
                int64_t i64;
                TDblClass dClass{ dblInfo(X, i64) };
                int xv{ vm_valund };
                for (; xv < vm_normal; xv++)
                    if (i64 == intlValueMapI64[xv]) break;
                if (xv == vm_normal && dClass != DBL_FINITE) {
                    switch (dClass) {
                    case DBL_NINF: xv = vm_valmin; break;
                    case DBL_PINF: xv = vm_valpin; break;
                    case DBL_NAN: xv = vm_valna; break;
                    default: break;
                    }
                }
                FFile->WriteByte(xv);
                if (xv == vm_normal) {
                    FFile->WriteDouble(X);
                    if (X >= Zvalacr) {
                        int v = static_cast<int>(std::round(X / Zvalacr));
                        int ix = utils::indexOf<TAcronym>(AcronymList, [&v](const TAcronym& acro) { return acro.AcrMap == v; });
                        if (ix == -1) AcronymList.push_back(TAcronym{ "", "", v, -1, false });
                    }
                }
            }
        }
        DataCount++;
        if (utils::in(CurSyPtr->SDataType, dt_set, dt_alias)) {
            if (AVals[vallevel] != 0.0) CurSyPtr->SSetText = true;
            if (FCurrentDim == 1 && CurSyPtr->SSetBitMap) {
                auto& ssbm = *CurSyPtr->SSetBitMap;
                if (ssbm.size() <= LastElem.front())
                    ssbm.push_back(true);
                else
                    ssbm[LastElem.front()] = true;
            }
        }
        return true;
    }

    bool TGXFileObj::DoRead(double *AVals, int &AFDim) {
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

    void TGXFileObj::AddToErrorListDomErrs(const std::array<int, MaxDim> &AElements, const double * AVals) {
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
                    ErrorList[AElements] = utils::asArray<double, 5>(AVals);
                    return;
                }
            }
        }
    }

    void TGXFileObj::AddToErrorList(const int *AElements, const double *AVals) {
        if (ErrorList.size() >= 11) // avoid storing too many errors
            return;
        ErrorList[ utils::asArrayN<int, MaxDim>(AElements, FCurrentDim) ] = utils::asArray<double, 5>(AVals);
    }

    bool TGXFileObj::ResultWillBeSorted(const int *ADomainNrs) {
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

    void TGXFileObj::GetDefaultRecord(double *Avals) {
        int ui{};
        switch (CurSyPtr->SDataType) {
        case dt_set:
        case dt_alias:
        case dt_par:
            Avals[vallevel] = 0.0;
            break;
        case dt_var:
            ui = CurSyPtr->SUserInfo;
            memcpy(Avals, ui >= stypunknwn && ui <= stypsemiint ? defrecvar[ui].data() : defrecvar[stypunknwn].data(), sizeof(double)*5);
            break;
        case dt_equ:
            ui = CurSyPtr->SUserInfo;
            memcpy(Avals, ui >= ssyeque && ui <= ssyeque + (styequb + 1) ? defrecequ[ui].data() : defrecequ[ssyeque].data(), sizeof(double)*5);
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

    // Summary:
    //   Returns the text for a given error number
    // Arguments:
    //    N: Error number
    //    S: Contains error text after return
    // Return Value:
    //   Always returns non-zero
    // Description:
    //
    // See Also:
    //   gdxGetLastError
    int TGXFileObj::gdxErrorStr(int ErrNr, std::string &ErrMsg) {
        return gdxErrorStrStatic(ErrNr, ErrMsg);
    }

    int TGXFileObj::gdxErrorStrStatic(int ErrNr, std::string& ErrMsg) {
        const auto it = errorCodeToStr.find(ErrNr);
        if (it == errorCodeToStr.end()) ErrMsg = rtl::sysutils_p3::SysErrorMessage(ErrNr);
        else ErrMsg = it->second;
        return true;
    }

    // Brief:
    //   Open a gdx file for reading
    // Arguments:
    //   FileName: file name of the gdx file to be opened
    //   ErrNr: Returns an error code or zero if there is no error
    // Returns:
    //   Returns non-zero if the file can be opened; zero otherwise
    // See Also:
    //   gdxOpenWrite, Destroy, gdxGetLastError
    // Description:
    //   Open an existing gdx file for input. If a file extension is not
    //   supplied, the extension '.gdx' will be used. The return code is
    //   a system dependent I/O error. If the file was found, but is not
    //   a valid gdx file, the function GetLastError can be used to handle
    //   these type of errors.
    // Example:
    // <CODE>
    //   var
    //      ErrNr: integer;
    //      PGX  : PGXFile;
    //   begin
    //   gdxOpenRead(PGX,'c:\\mydata\\file1.gdx', ErrNr);
    //   if ErrNr <> 0
    //   then
    //      begin
    //      [...]
    // </CODE>
    int TGXFileObj::gdxOpenRead(const std::string &FileName, int &ErrNr) {
        return gdxOpenReadXX(FileName, fmOpenRead, 0, ErrNr);
    }

    // Brief:
    //   Return strings for file version and file producer
    // Arguments:
    //   FileStr: Version string
    //   ProduceStr: Producer string
    // Returns:
    //   Always non-zero
    // See Also:
    //   gdxOpenWrite, gdxOpenWriteEx
    // Description:
    //
    int TGXFileObj::gdxFileVersion(std::string &FileStr, std::string &ProduceStr) {
        FileStr = FileSystemID;
        ProduceStr = FProducer;
        return true;
    }

    // Brief:
    //   Find symbol by name
    // Arguments:
    //   SyId: Name of the symbol
    //   SyNr: Symbol number
    // Returns:
    //   Non-zero if the symbol is found, zero otherwise.
    // Description:
    //   Search for a symbol by name; the search is not case sensitive.
    //   When the symbol is found, SyNr contains the symbol number and the
    //   function returns a non-zero integer. When the symbol is not found, the function
    //   returns zero.
    // See Also:
    //   gdxSymbolInfo, gdxSymbolInfoX
    int TGXFileObj::gdxFindSymbol(const std::string &SyId, int &SyNr) {
        if (SyId == "*") {
            SyNr = 0;
            return true;
        }
        // FIXME: This is slow (linear), use RB-tree instead
        const auto it = std::find_if(NameListOrdered.begin(), NameListOrdered.end(), [&](const std::string &name) {
           return  utils::sameText(SyId, name);
        });
        if(it == NameListOrdered.end()) return false;
        // FIXME: The symbol number cannot be trusted because it is not set correctly for set alias (also in Delphi)
        //SyNr = (*it).second->SSyNr;
        SyNr = static_cast<int>(std::distance(NameListOrdered.begin(), it) + 1);
        return true;
    }

    // Brief:
    //   Read the next record in string mode
    // Arguments:
    //   KeyStr: The index of the record as strings for the unique elements
    //   Values: The data of the record
    //   DimFrst: The first index position in KeyStr that changed
    // Returns:
    //   Non-zero if the operation is possible; return zero if the operation is not
    //   possible or if there is no more data
    // See Also:
    //   gdxDataReadStrStart, gdxDataReadDone
    // Description:
    //   Read the next record using strings for the unique elements. The
    //   reading should be initialized by calling DataReadStrStart
    int TGXFileObj::gdxDataReadStr(char **KeyStr, double *Values, int &DimFrst) {
        const TgxModeSet AllowedModes{ fr_str_data };
        if ((TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) && !CheckMode("DataReadStr", AllowedModes))
            return false;
        if (!DoRead(Values, DimFrst)) {
            gdxDataReadDone();
            return false;
        }
        else {
            for (int D{}; D < FCurrentDim; D++) {
                int LED{ LastElem[D] };
                std::string s {LED >= 1 && LED <= UELTable.size() ? UELTable[LED-1] : BADUEL_PREFIX + std::to_string(LED)};
                memcpy(KeyStr[D], s.c_str(), sizeof(char)*(s.length()+1));
            }
            return true;
        }
    }

    // Brief:
    //   Finish reading of a symbol in any mode(raw, mapped, string)
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataReadRawStart, gdxDataReadMapStart, gdxDataReadStrStart
    // Description:
    int TGXFileObj::gdxDataReadDone() {
        TgxModeSet AllowedMode {fr_init,fr_raw_data,fr_map_data,fr_mapr_data, fr_str_data,fr_slice};
        if(SortList) SortList->clear();
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
            int HighestIndex = UELTable.UsrUel2Ent.GetHighestIndex();
            for(int N{HighestIndex}; N >= HighestIndex - NrMappedAdded + 1; N--) {
                assert(N >= 1 && "Wrong entry number");
                int EN {UELTable.UsrUel2Ent[N]}, // nr in ueltable
                    d {UELTable.GetUserMap(EN)};
                assert(d == -1 || d == N && "Mapped already");
                UELTable.SetUserMap(EN, N); // map to user entry
            }
            NrMappedAdded = 0;
        }
        fmode = fr_init;
        return true;
    }

    // Brief:
    //  Returns information about a symbol
    // Arguments:
    //   SyNr: The symbol number (range 0..NrSymbols); return universe info when SyNr = 0
    //   SyId: Name of the symbol
    //   Dimen: Dimension of the symbol
    //   Typ: Symbol type
    // Returns:
    //   Zero if the symbol number is not in the correct range, non-zero otherwise
    // See Also:
    //   gdxSystemInfo, gdxSymbolInfoX, gdxSymbolDim, gdxFindSymbol
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
            auto &[name, sym] = *maybeNameAndSym;
            SyId = name;
            Dim = sym->SDim;
            Typ = sym->SDataType;
            return true;
        }

        return badLookup();
    }

    // Brief:
    //   Initialize the reading of a symbol in string mode
    // Arguments:
    //   SyNr: The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
    //   NrRecs: The number of records available for reading
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataReadStr, gdxDataReadRawStart, gdxDataReadMapStart, gdxDataReadDone
    // Description:
    //   Reading data using strings is the simplest way to read data.
    //   Every record read using DataReadStr will return the strings
    //   for the unique elements. Internal mapping is not affected by
    //   this function.
    // Example:
    //   if DataReadStrStart(PGX,1,NrRecs)
    //   then
    //      begin
    //      while DataReadStr(PGX,Uels,Vals)
    //      do [...]
    //      DataReadDone(PGX)
    //      end;
    int TGXFileObj::gdxDataReadStrStart(int SyNr, int &NrRecs) {
        NrRecs = PrepareSymbolRead("DataReadStrStart"s, SyNr,
                                   utils::arrayWithValue<int, MaxDim>(DOMC_UNMAPPED).data(), fr_str_data);
        return NrRecs >= 0;
    }

    int TGXFileObj::gdxOpenReadXX(const std::string &Afn, int filemode, int ReadMode, int &ErrNr) {
        if(fmode != f_not_open) {
            ErrNr = ERR_FILEALREADYOPEN;
            return false;
        }
        MajContext = "OpenRead"s;
        TraceLevel = defaultTraceLevel;
        fmode = f_not_open;
        //ReadPtr = nullptr;
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
        NameListOrdered.clear();
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
            CurSyPtr->SSetBitMap = std::nullopt;
            CurSyPtr->SDomStrings.clear();
            NameList[S] = CurSyPtr;
            NameListOrdered.push_back(S);
            CurSyPtr->SSyNr = static_cast<int>(NameListOrdered.size());
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
                if (TextNum != N) { // duplicates stored in GDX file, e.g. empty string
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
            for (auto & i : DomainStrList)
                i = FFile->ReadString();
            if (ErrorCondition(FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER2)) return FileErrorNr();
            while (true) {
                int SyNr = FFile->ReadInteger();
                if (SyNr <= 0) break;
                auto maybeNameAndSym = symbolWithIndex(SyNr);
                if (maybeNameAndSym) {
                    auto sym = (*maybeNameAndSym).second;
                    sym->SDomStrings.resize(sym->SDim + 1);
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
        if(index < 1 || index > NameList.size()) return std::nullopt;
        std::string name = NameListOrdered[index-1];
        return std::make_optional(std::make_pair(name, NameList[name]));
    }

    // Summary:
    //   Add an alias for a set to the symbol table
    // Arguments:
    //    AName1: set identifier
    //    AName2: set identifier
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Description:
    //    One of the two identifiers has to be a known set, an alias or * (universe);
    //    the other identifier is used as the new alias for the given set.
    //    The function gdxSymbolInfoX can be used to retrieve the set or alias
    //       associated with the identifier; it is returned as the UserInfo parameter.
    // See Also:
    //   gdxSymbolSetDomain
    int TGXFileObj::gdxAddAlias(const std::string &Id1, const std::string &Id2) {
        if(!MajorCheckMode("AddAlias", AnyWriteMode)) return false;
        auto symbolNameToIndex = [&](const std::string &name) {
            // FIXME: A lookup through NameList map would be faster *but* SSyNr is not set correctly for alias objects in memory!
            return name == "*" ? std::numeric_limits<int>::max() : utils::indexOf(NameListOrdered, name, -2) + 1;
        };
        int SyNr1 { symbolNameToIndex(Id1) }, SyNr2 { symbolNameToIndex(Id2) };
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
        // TODO: Also the Delphi source does not set SSyNr here correctly, what to do?
        NameList[AName] = SyPtr;
        NameListOrdered.push_back(AName);
        return true;
    }

    // Brief:
    //   Register a string in the string table
    // Arguments:
    //   Txt: The string to be registered
    //   TxtNr: The index number assigned to this string
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //  gdxGetElemText, gdxSetTextNodeNr
    // Description:
    //  Register a string in the string table and return the integer number assigned to this string.
    //  The integer value can be used to set the associated text of a set element.
    //  The string must follow the GAMS syntax rules for explanatory text.
    int TGXFileObj::gdxAddSetText(const std::string &Txt, int &TxtNr) {
        if(SetTextList.empty() || TraceLevel >= TraceLevels::trl_all && !CheckMode("AddSetText", {}))
            return false;
        SetTextList.emplace_back(MakeGoodExplText(Txt));
        TxtNr = static_cast<int>(SetTextList.size()-1);
        return true;
    }

    // Brief:
    //   The number of error records
    // Returns:
    //   The number of error records available.
    // See Also:
    //   gdxDataErrorRecord
    // Description:
    //   After a write operation is finished (gdxDataWriteDone), the data
    //   is sorted and written to the gdx file. If there are duplicate records,
    //   the first record is written to the file and the duplicates are
    //   added to the error list.
    //   <P>
    //   When reading data using a filtered read operation, data records that were
    //   filtered out because an index is not in the user index space or not in a
    //   filter are added the error list.
    int TGXFileObj::gdxDataErrorCount()
    {
        return static_cast<int>(ErrorList.size());
    }

    // Brief:
    //   Retrieve an error record
    // Arguments:
    //   RecNr: The number of the record to be retrieved, range = 1..NrErrorRecords
    //   KeyInt: Index for the record
    //   Values: Values for the record
    // Returns:
    //   Non-zero if the record number is valid, zero otherwise
    // See Also:
    //   gdxDataErrorCount
    int TGXFileObj::gdxDataErrorRecord(int RecNr,  int * KeyInt,  double * Values)
    {
        int res{ gdxDataErrorRecordX(RecNr, KeyInt, Values) };
        if (res) {
            for (int D{}; D < FCurrentDim; D++) {
                if (KeyInt[D] < 0) KeyInt[D] = -KeyInt[D];
            }
        }
        return res;
    }

    // Brief:
    //   Retrieve an error record
    // Arguments:
    //   RecNr: The number of the record to be retrieved, range = 1..NrErrorRecords
    //   KeyInt: Index for the record, negative uel indicates domain violation for filtered/strict read
    //   Values: Values for the record
    // Returns:
    //   Non-zero if the record number is valid, zero otherwise
    // See Also:
    //   gdxDataErrorCount
    int TGXFileObj::gdxDataErrorRecordX(int RecNr, int * KeyInt,  double * Values)
    {
        TgxModeSet AllowedModes{ fr_init,fw_init,fr_map_data, fr_mapr_data, fw_raw_data, fw_map_data,fw_str_data };
        if ((TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) && !CheckMode("DataErrorRecord", AllowedModes))
            return false;

        if (!ErrorList.empty()) {
            if (RecNr < 1 || RecNr > ErrorList.size()) {
                ReportError(ERR_BADERRORRECORD);
            }
            else {
                ErrorList.GetRecord(RecNr - 1, KeyInt, FCurrentDim, Values);
                return true;
            }
        }

        return false;
    }

    // Brief:
    //   Read the next record in raw mode
    // Arguments:
    //   KeyInt: The index of the record
    //   Values: The data of the record
    //   DimFrst: The first index position in KeyInt that changed
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataReadRawStart, gdxDataReadDone
    // Description:
    int TGXFileObj::gdxDataReadRaw(int *KeyInt, double *Values, int &DimFrst) {
        TgxModeSet  AllowedModes{fr_raw_data};
        if((TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) && !CheckMode("DataReadRaw", AllowedModes)) return false;
        if(!DoRead(Values, DimFrst)) gdxDataReadDone();
        else {
            //std::copy(LastElem.begin(), LastElem.begin()+FCurrentDim-1, KeyInt.begin());
            memcpy(KeyInt, LastElem.data(), (FCurrentDim-1)*sizeof(int));
            return true;
        }
        return false;
    }

    // Brief:
    //   Initialize the reading of a symbol in raw mode
    // Arguments:
    //   SyNr: The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
    //   NrRecs: The number of records available for reading
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataReadRaw, gdxDataReadMapStart, gdxDataReadStrStart, gdxDataReadDone
    int TGXFileObj::gdxDataReadRawStart(int SyNr, int &NrRecs) {
        NrRecs = PrepareSymbolRead("DataReadRawStart", SyNr,
                                   utils::arrayWithValue<int, MaxDim>(DOMC_UNMAPPED).data(), fr_raw_data);
        return NrRecs >= 0;
    }

    // Brief:
    //   Write a data element in raw mode
    // Arguments:
    //   KeyInt: The index for this element
    //   Values: The values for this element
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Description:
    //   When writing data in raw mode, the index space used is based on the
    //   internal index space. The indices used are in the range 1..NrUels but this is not enforced.
    //   Before we can write in raw mode, the unique elements (strings) should
    //   be registered first.
    //   <P>
    //   When writing raw, it assumed that the records are written in sorted order and
    //   that there are no duplicate records. Records that are not in sorted order or are
    //   duplicates will be added to the error list (see DataErrorCount and DataErrorRecord)
    // See Also:
    //   gdxDataWriteRawStart, gdxDataWriteDone
    int TGXFileObj::gdxDataWriteRaw(const int* KeyInt, const double* Values)
    {
        TgxModeSet AllowedModes{ fw_raw_data };
        if (fmode == fw_dom_raw) fmode = fw_raw_data;
        if ((TraceLevel >= TraceLevels::trl_some || !utils::in(fmode, AllowedModes)) && !CheckMode("DataWriteRaw", AllowedModes)) return false;
        if (DoWrite(KeyInt, Values)) return true;
        return false;
    }

    // Brief:
    //   Start writing a new symbol in raw mode
    // Arguments:
    //   SyId: Name of the symbol
    //   ExplTxt: Explanatory text for the symbol
    //   Dimen: Dimension of the symbol
    //   Typ: Type of the symbol
    //   UserInfo: GAMS follows the following conventions:
    //<TABLE>
    //Type           Value(s)
    //-------------  -------------------------------------------------------
    // Aliased Set   The symbol number of the aliased set, or zero for
    //                 the universe
    // Set           Zero
    // Parameter     Zero
    // Variable      The variable type: binary=1, integer=2, positive=3,
    //                 negative=4, free=5, sos1=6, sos2=7, semicontinous=8,
    //                 semiinteger=9
    // Equation      The equation type: eque=53, equg=54, equl=55, equn=56,
    //                 equx=57, equc=58, equb=59
    //</TABLE>
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Description:
    //
    // See Also:
    //   gdxDataWriteRaw, gdxDataWriteDone
    int TGXFileObj::gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                         int UserInfo) {
        if(!PrepareSymbolWrite("DataWriteRawStart", SyId, ExplTxt, Dimen, Typ, UserInfo)) return false;
        // we overwrite the initialization
        std::fill_n(MinElem.begin(), FCurrentDim, 0); // no assumptions about the range for a uel
        std::fill_n(MaxElem.begin(), FCurrentDim, std::numeric_limits<int>::max());
        InitDoWrite(-1);
        fmode = fw_dom_raw;
        return true;
    }

    // Brief:
    //  Returns the number of errors
    // Returns:
    //  Total number of errors encountered
    // See Also:
    //   gdxGetLastError
    int TGXFileObj::gdxErrorCount() {
        return ErrCntTotal;
    }

    // Brief:
    //   Retrieve the string and node number for an entry in the string table
    // Arguments:
    //   TxtNr: String table index
    //   Txt: Text found for the entry
    //   Node: Node number found for the entry
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxAddSetText, gdxSetTextNodeNr
    // Description:
    //   Retrieve a string based on the string table index. When writing to a gdx file,
    //   this index is the value returned by calling gdxAddSetText. When reading a gdx file,
    //   the index is returned as the level value when reading a set.
    //   The Node number can be used as an index in a string table in the user space;
    //   the value is set by calling SetTextNodeNr. If the Node number was never assigned,
    //   it will be returned as zero.
    //   Example:
    // <CODE>
    // [assumes we are reading using strings ...]
    // while gdxDataReadStr(PGX, Uels, Vals) <> 0
    // do begin
    //    for D := 1 to Dim
    //    do Write(Uels[D], '  ');
    //    indx := Round(Vals[vallevel]);
    //    if indx > 0
    //    then
    //       begin
    //       gdxGetElemText(indx, S, N);
    //       Write('txt = ', S, ' Node = ', N);
    //       end;
    //    WriteLn;
    //    end
    // </CODE>
    int TGXFileObj::gdxGetElemText(int TxtNr, std::string &Txt, int &Node) {
        Node = 0;
        if(SetTextList.empty()) {
            Txt.clear();
            return false;
        }
        if(TraceLevel >= TraceLevels::trl_all && !CheckMode("GetElemText", {}))
            return false;
        if(TxtNr < 0 || TxtNr >= SetTextList.size()) {
            Txt = BADStr_PREFIX + std::to_string(TxtNr);
        } else {
            Txt = SetTextList[TxtNr];
            Node = 0;
            return true;
        }
        return false;
    }

    // Brief:
    //  Return the last error
    // Returns:
    //  The error number, or zero if there was no error
    // Description:
    //  When an error is encountered, an error code is stored which can
    //  be retrieved with this function. If subsequent errors occur before
    //  this function is called, the first error code will be maintained.
    //  Calling this function will clear the last error stored.
    // See Also:
    //  gdxErrorCount
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

    // Brief:
    //   Retrieve the internal values for special values
    // Arguments:
    //   AVals: array of special values used for Eps, +Inf, -Inf, NA and Undef
    // Returns:
    //   Always non-zero
    // See Also:
    //  gdxResetSpecialValues, gdxSetSpecialValues
    // Description:
    //
    int TGXFileObj::gdxGetSpecialValues(TgdxSVals &AVals) {
        AVals[sv_valund] = intlValueMapDbl[vm_valund] ;
        AVals[sv_valna ] = intlValueMapDbl[vm_valna ] ;
        AVals[sv_valpin] = intlValueMapDbl[vm_valpin] ;
        AVals[sv_valmin] = intlValueMapDbl[vm_valmin] ;
        AVals[sv_valeps] = intlValueMapDbl[vm_valeps] ;
        AVals[sv_acronym] = Zvalacr;
        return true;
    }

    // Brief:
    //   Set the internal values for special values
    // Arguments:
    //   AVals: array of special values to be used for Eps, +Inf, -Inf, NA and Undef
    //          Note that the values have to be unique
    // Returns:
    //   Non-zero if all values specified are unique, zero otherwise
    // Note: Before calling this function, initialize the array of special values
    //   by calling gdxGetSpecialValues first
    // See Also:
    //   gdxSetReadSpecialValues, gdxResetSpecialValues, gdxGetSpecialValues
    // Description:
    //
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

    // Summary:
    //   Retrieve the domain of a symbol
    // Arguments:
    //   SyNr: The index number of the symbol, range 1..NrSymbols
    //   DomainSyNrs: array returning the set identifiers or *;
    //   DomainSyNrs[D] will contain the index number of the one dimensional
    //   set or alias used as the domain for index position D. A value of zero represents the universe ( * )
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxSymbolSetDomain, gdxSymbolGetDomainX
    int TGXFileObj::gdxSymbolGetDomain(int SyNr, int *DomainSyNrs) {
        if (ErrorCondition(SyNr >= 1 && SyNr <= NameList.size(), ERR_BADSYMBOLINDEX)) return false;
        PgdxSymbRecord SyPtr{ (*symbolWithIndex(SyNr)).second };
        for (int D{}; D < SyPtr->SDim; D++)
            DomainSyNrs[D] = SyPtr->SDomSymbols.empty() || SyPtr->SDomSymbols[D] == 0 ? 0 : SyPtr->SDomSymbols[D];
        return true;
    }

    // Summary:
    //   Retrieve the domain of a symbol (using relaxed or domain information)
    // Arguments:
    //   SyNr: The index number of the symbol, range 1..NrSymbols
    //   DomainIDs[D] will contain the strings as they were stored with the call
    //   gdxSymbolSetDomainX. If gdxSymbolSetDomainX was never called, but gdxSymbolSetDomain
    //   was called, that information will be used instead.
    // Returns:
    //   0: If operation was not possible (Bad SyNr)
    //   1: No domain information was available
    //   2: Data used was defined using gdxSymbolSetDomainX
    //   3: Data used was defined using gdxSymbolSetDomain
    // See Also:
    //   gdxSymbolSetDomainX, gdxSymbolSetDomain
    int TGXFileObj::gdxSymbolGetDomainX(int SyNr, char **DomainIDs) {
        if (ErrorCondition(!NameList.empty() && SyNr >= 1 && SyNr <= NameList.size(), ERR_BADSYMBOLINDEX)) return 0;
        PgdxSymbRecord SyPtr{ (*symbolWithIndex(SyNr)).second };

        for(int D{0}; D<SyPtr->SDim; D++) {
            DomainIDs[D][0] = '*';
            DomainIDs[D][1] = '\0';
        }

        if (!SyPtr->SDomStrings.empty()) {
            for (int D{}; D<SyPtr->SDim; D++)
                utils::stocp(DomainStrList[SyPtr->SDomStrings[D] - 1], DomainIDs[D]);
            return 2;
        }
        else if (SyPtr->SDomSymbols.empty())
            return 1;
        else {
            for (int D{}; D < SyPtr->SDim; D++)
                if (SyPtr->SDomSymbols[D])
                    utils::stocp((*symbolWithIndex(SyPtr->SDomSymbols[D])).first, DomainIDs[D]);
            return 3;
        }
        return 0;
    }

    // Brief:
    //  Returns Dimension of a symbol
    // Arguments:
    //   SyNr: The symbol number (range 0..NrSymbols); return universe info when SyNr = 0
    // Returns:
    //   -1 if the symbol number is not in the correct range, the symbol dimension otherwise
    // See Also:
    //   gdxSymbolInfo, gdxSymbolInfoX, gdxFindSymbol
    int TGXFileObj::gdxSymbolDim(int SyNr) {
        if (!SyNr) return 1;
        return NameList.empty() || SyNr < 1 || SyNr > NameList.size() ? -1 : (*symbolWithIndex(SyNr)).second->SDim;
    }

    // Brief:
    //  Returns additional information about a symbol
    // Arguments:
    //   SyNr: The symbol number (range 0..NrSymbols); return universe info when SyNr = 0
    //   RecCnt: Total number of records stored (unmapped); for the universe (SyNr = 0) this is the
    //      number of entries when the gdx file was openened for reading.
    //   UserInfo: User field value; see gdxDataWriteRawStart for more information
    //   ExplTxt: Explanatory text for the symbol
    // Returns:
    //   Zero if the symbol number is not in the correct range, non-zero otherwise
    // See Also:
    //   gdxSystemInfo, gdxSymbolInfo, gdxFindSymbol
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

    // Summary:
    //   Define the domain of a symbol
    // Arguments:
    //   DomainIDs: array of identifers or *
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Description:
    //   This function defines the domain for the symbol for which a write
    //     data operation just started using DataWriteRawStart, DataWriteMapStart or
    //     DataWriteStrStart. At this point the symbol and dimension is known,
    //     but no data has been written yet.
    //   Each identifier will be checked to be a one dimensional set or an alias.
    //   When a domain is specified, write operations will be domain checked; records
    //   violating the domain will be added to the internal error list (see DataErrorCount
    //   and DataErrorRecord.)
    // See Also:
    //   gdxSymbolGetDomain
    int TGXFileObj::gdxSymbolSetDomain(const char **DomainIDs) {
        int res{ false }, SyNr;
        TgxModeSet AllowedModes{ fw_dom_raw, fw_dom_map, fw_dom_str };
        if (!MajorCheckMode("SymbolSetDomain", AllowedModes) || !CurSyPtr) return res;
        res = true;
        assert(CurSyPtr->SDomSymbols.empty() && "SymbolSetDomain");
        CurSyPtr->SDomSymbols.resize(CurSyPtr->SDim + 1);
        for (int D{}; D < CurSyPtr->SDim; D++) {
            bool domap = true;
            int DomSy;
            if (!std::strcmp(DomainIDs[D], "*")) DomSy = 0;
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
                    const auto *obj = (*symbolWithIndex(SyNr)).second;
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
                if (CurSyPtr->SDim != 1 || CurSyPtr != (*symbolWithIndex(DomSy)).second) {
                    WrBitMaps[D] = (*((*symbolWithIndex(SyNr)).second)->SSetBitMap);
                }
            }
        }
        switch (fmode) {
        case fw_dom_raw: fmode = fw_raw_data; break;
        case fw_dom_map: fmode = fw_map_data; break;
        case fw_dom_str: fmode = fw_str_data; break;
        default: break;
        }
        return res;
    }

    // Summary:
    //   Define the domain of a symbol (relaxed version)
    // Arguments:
    //   DomainIDs: array of identifers or *
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Description:
    //   This function defines the relaxed domain information for the symbol SyNr.
    //   The identifiers will NOT be checked to be known one-dimensional sets, and
    //   no domain checking will be performed. This function can be called during or after
    //   the write operation.
    //   If domain checking is needed, use gdxSymbolSetDomain
    // See Also:
    //   gdxSymbolSetDomain, gdxSymbolGetDomainX
    int TGXFileObj::gdxSymbolSetDomainX(int SyNr, const char **DomainIDs) {
        // check for write or append only
        if (ErrorCondition(SyNr >= 1 && SyNr <= NameList.size(), ERR_BADSYMBOLINDEX)) return false;
        PgdxSymbRecord SyPtr = (*symbolWithIndex(SyNr)).second;
        if (SyPtr->SDim > 0) {
            if (SyPtr->SDomStrings.empty())
                SyPtr->SDomStrings.resize(SyPtr->SDim);
            for (int D{}; D < SyPtr->SDim; D++) {
                const std::string &S { DomainIDs[D] };
                if (S.empty() || S == "*" || !IsGoodIdent(S)) SyPtr->SDomStrings[D] = 0;
                else {
                    SyPtr->SDomStrings[D] = utils::indexOf(DomainStrList, S);
                    if (SyPtr->SDomStrings[D] <= -1) {
                        DomainStrList.push_back(S);
                        SyPtr->SDomStrings[D] = (int) DomainStrList.size();
                    }
                }
            }
        }
        return true;
    }

    // Brief:
    //   Returns the number of symbols and unique elements
    // Arguments:
    //   SyCnt: Number of symbols available in the gdx file
    //   UelCnt: Number of unique elements stored in the gdx file
    // Returns:
    //   Returns a non-zero value
    int TGXFileObj::gdxSystemInfo(int &SyCnt, int &UelCnt) {
        UelCnt = (int)UELTable.size();
        SyCnt = (int)NameList.size();
        return true;
    }

    // Brief:
    //   Finish registration of unique elements
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUELRegisterRawStart, gdxUELRegisterMapStart, gdxUELRegisterStrStart
    int TGXFileObj::gdxUELRegisterDone() {
        TgxModeSet AllowedModes{ f_raw_elem,f_map_elem,f_str_elem };
        if (!MajorCheckMode("UELRegisterDone", AllowedModes)) return false;
        fmode = fmode_AftReg;
        return true;
    }

    // Brief:
    //   Register an unique elements in raw mode
    // Arguments:
    //   Uel: String for unique element
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUELRegisterMap, gdxUELRegisterDone
    // Description:
    //  The unique element is registered in raw mode, i.e. the internally
    //  assigned integer index is determined by the system
    //  Can only be used while writing to a gdx file
    int TGXFileObj::gdxUELRegisterRaw(const std::string &Uel) {
        TgxModeSet AllowedModes{ f_raw_elem };
        if ((TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) && !CheckMode("UELRegisterRaw", AllowedModes))
            return false;
        std::string SV = utils::trimRight(Uel);
        if (ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
        UELTable.AddObject(SV, -1); // should about existing mapping?
        return true;
    }

    // Brief:
    //   Start registering unique elements in raw mode
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUELRegisterRaw, gdxUELRegisterDone
    int TGXFileObj::gdxUELRegisterRawStart() {
        TgxModeSet AllowedModes { fw_init };
        if (!MajorCheckMode("UELRegisterRawStart", AllowedModes)) return false;
        fmode_AftReg = fw_init;
        fmode = f_raw_elem;
        return true;
    }

    // Brief:
    //   Register a unique element in string mode
    // Arguments:
    //   Uel: String for unique element
    //   UelNr: Index number assigned to this unique element in user space
    // Returns:
    //   Non-zero if the element was registered, zero otherwise.
    // Description:
    //  The unique element is registered in user mapped space. The returned
    //  index is the next higher value. Registering an element a second time
    //  is not considered an error and the same index position will be returned.
    //   A unique element must follow the GAMS rules when it contains quote characters.
    // See Also:
    //   gdxUELRegisterStrStart, gdxUELRegisterDone
    int TGXFileObj::gdxUELRegisterStr(const std::string &Uel, int &UelNr) {
        const TgxModeSet AllowedModes{ f_str_elem };
        if ((TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) && !CheckMode("UELRegisterStr", AllowedModes))
            return false;
        std::string SV{ utils::trimRight(Uel) };
        if (ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
        UelNr = UELTable.AddUsrNew(SV);
        return true;
    }

    // Brief:
    //   Start registering unique elements in string mode
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUELRegisterStr, gdxUELRegisterDone
    // Description:
    int TGXFileObj::gdxUELRegisterStrStart() {
        TgxModeSet AllowedModes{ fr_init, fw_init };
        if (!MajorCheckMode("UELRegisterStrStart", AllowedModes)) return false;
        fmode_AftReg = fmode == fw_init ? fw_init : fr_init;
        fmode = f_str_elem;
        return true;
    }

    // Brief:
    //   Get a unique element using an unmapped index
    // Arguments:
    //   UelNr: Element number (unmapped) in the range 1..NrElem
    //   Uel: String for unique element
    //   UelMap: User mapping for this element or -1 if element was never mapped
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUMUelInfo, gdxGetUEL
    // Description:
    //
    int TGXFileObj::gdxUMUelGet(int UelNr, std::string &Uel, int &UelMap) {
        if (!UELTable.empty() && UelNr >= 1 && UelNr <= UELTable.size()) {
            Uel = UELTable[UelNr-1];
            UelMap = UELTable.UsrUel2Ent.empty() ? -1 : UELTable.UsrUel2Ent.GetReverseMapping(UelNr);
            return true;
        }
        else {
            Uel = BADUEL_PREFIX + std::to_string(UelNr);
            UelMap = -1;
            return false;
        }
    }

    // Brief:
    //   Return information about the unique elements
    // Arguments:
    //   UelCnt: Total number of unique elements (uels in gdx file + new registered uels)
    //   HighMap: Highest user mapping index used
    // Returns:
    //   Always returns non-zero
    // See Also:
    //  gdxUMUelGet
    // Description:
    int TGXFileObj::gdxUMUelInfo(int &UelCnt, int &HighMap) {
        if (!FFile) { // AS: Use FFile != nullptr as proxy for checking open has been called before
            UelCnt = HighMap = 0;
            return false;
        }
        else {
            UelCnt = UELTable.size();
            HighMap = UELTable.UsrUel2Ent.GetHighestIndex(); // highest index
            return true;
        }
    }

    // Summary:
    //   Returns the dimension of the current active symbol
    // Arguments:
    // Return Value:
    //   Dimension of current active symbol
    // Description:
    //    When reading or writing data, the dimension of the current active symbol
    //    is sometimes needed to convert arguments from strings to pchars etc.
    // See Also:
    //
    int TGXFileObj::gdxCurrentDim() {
        return FCurrentDim; //need to do more checks here
    }

    // Brief:
    //   Rename UEL OldName to NewName
    // Arguments:
    //   OldName: Name of an existing UEL
    //   NewName: New name for the UEL
    // Returns:
    //   Zero if the renaming was possible; non-zero is an error indicator
    int TGXFileObj::gdxRenameUEL(const std::string &OldName, const std::string &NewName) {
        if(UELTable.empty())
            return -1;
        std::string S{utils::trimRight(NewName)};
        if(!GoodUELString(S))
            return ERR_BADUELSTR;
        int N{UELTable.IndexOf(utils::trimRight(OldName))};
        if(N < 0)
            return 2;
        else if(UELTable.IndexOf(S) >= 0)
            return 3;
        UELTable.RenameEntry(N, S);
        return 0;
    }

    // Brief:
    //   Open a gdx file for reading
    // Arguments:
    //   FileName: file name of the gdx file to be opened
    //   ReadMode: bitmap skip reading sections: 0-bit: string (1 skip reading string)
    //   ErrNr: Returns an error code or zero if there is no error
    // Returns:
    //   Returns non-zero if the file can be opened; zero otherwise
    // See Also:
    //   gdxOpenWrite, Destroy, gdxGetLastError
    // Description:
    //   Open an existing gdx file for input. If a file extension is not
    //   supplied, the extension '.gdx' will be used. The return code is
    //   a system dependent I/O error. If the file was found, but is not
    //   a valid gdx file, the function GetLastError can be used to handle
    //   these type of errors.
    // Example:
    // <CODE>
    //   var
    //      ErrNr: integer;
    //      PGX  : PGXFile;
    //   begin
    //   gdxOpenRead(PGX,'c:\\mydata\\file1.gdx', ErrNr);
    //   if ErrNr <> 0
    //   then
    //      begin
    //      [...]
    // </CODE>
    int TGXFileObj::gdxOpenReadEx(const std::string &FileName, int ReadMode, int &ErrNr) {
        return gdxOpenReadXX(FileName, fmOpenRead, ReadMode, ErrNr);
    }

    // Brief:
    //  Get the string for a unique element using a mapped index
    // Arguments:
    //   UelNr: Index number in user space (1..NrUserElem)
    //   Uel: String for the unique element
    // Returns:
    //  Return non-zero if the index is in a valid range, zero otherwise
    // Description:
    //  Retrieve the string for a unique element based on a mapped index number.
    // See Also:
    //   gdxUMUelGet
    int TGXFileObj::gdxGetUEL(int uelNr, std::string &Uel) {
        if(UELTable.empty()) {
            Uel.clear();
            return false;
        }
        int EN = UELTable.UsrUel2Ent[uelNr];
        Uel = EN >= 1 ? UELTable[EN-1] : BADUEL_PREFIX + std::to_string(uelNr);
        return EN >= 1;
    }

    // Brief:
    //   Start writing a new symbol in mapped mode
    // Arguments:
    //   SyId: Name of the symbol
    //   ExplTxt: Explanatory text for the symbol
    //   Dimen: Dimension of the symbol
    //   Type: Type of the symbol
    //   UserInfo: See gdxDataWriteRawStart for more information
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Description:
    // See Also:
    //   gdxDataWriteMap, gdxDataWriteDone
    int TGXFileObj::gdxDataWriteMapStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                         int UserInfo) {
        if(!PrepareSymbolWrite("DataWriteMapStart", SyId, ExplTxt, Dimen, Typ, UserInfo)) return false;
        SortList = std::make_unique<gdlib::datastorage::TLinkedData<gxdefs::TgdxValues>>(FCurrentDim, static_cast<int>(DataSize * sizeof(double)));
        fmode = fw_dom_map;
        return true;
    }

    // Brief:
    //   Write a data element in mapped mode
    // Arguments:
    //   KeyInt: The index for this element using mapped values
    //   Values: The values for this element
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataWriteMapStart, gdxDataWriteDone
    // Description:
    int TGXFileObj::gdxDataWriteMap(const int *KeyInt, const double *Values) {
        const TgxModeSet AllowedModes {fw_map_data};
        TIndex Keys;
        if(fmode == fw_dom_map)
            fmode = fw_map_data;
        if(TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) {
            if(!CheckMode("DataWriteMap", AllowedModes)) return false;
            std::cout << "   Index =";
            for(int D{}; D<FCurrentDim; D++) {
                std::cout << " " << std::to_string(KeyInt[D]);
                if(D+1 < FCurrentDim) std::cout << ",";
            }
        }
        for(int D{}; D<FCurrentDim; D++) {
            int KD = UELTable.UsrUel2Ent[KeyInt[D]];
            if(KD < 0) {
                ReportError(ERR_BADELEMENTINDEX);
                return false;
            }
            Keys[D] = KD;
            if(KD < MinElem[D]) MinElem[D] = KD;
            if(KD > MaxElem[D]) MaxElem[D] = KD;
        }
        SortList->AddItem(Keys, utils::asArray<double, 5>(Values));
        return true;
    }

    // Brief:
    //   Start registering unique elements in mapped mode
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUELRegisterMap, gdxUELRegisterDone
    // Description:
    int TGXFileObj::gdxUELRegisterMapStart() {
        TgxModeSet AllowedModes {fr_init, fw_init};
        if(!MajorCheckMode("UELRegisterMapStart", AllowedModes)) return false;
        fmode_AftReg = fmode == fw_init ? fw_init : fr_init;
        fmode = f_map_elem;
        return true;
    }

    // Brief:
    //   Register an unique elements in mapped mode
    // Arguments:
    //   UMap: User index number to be assigned to the unique element
    //   Uel: String for unique element
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUELRegisterMapStart, gdxUELRegisterDone
    // Description:
    //  Register a unique element in mapped space; UMap is the user assigned
    //  index for the element. Registering an element a second time is not considered
    //  an error as long as the same UMap is used. Assigning different elements
    //  with the same UMap value is an error.
    //   A unique element
    //   must follow the GAMS rules when it contains quote characters.
    int TGXFileObj::gdxUELRegisterMap(int UMap, const std::string &Uel) {
        TgxModeSet AllowedModes {f_map_elem};
        std::string SV {utils::trimRight(Uel)};
        if(TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) {
            if(!CheckMode("UELRegisterMap", AllowedModes)) return false;
            std::cout << "   Enter UEL: " << SV << " with number " << UMap << "\n";
        }
        if(ErrorCondition(GoodUELString(SV), ERR_BADUELSTR) ||
            ErrorCondition(UELTable.AddUsrIndxNew(SV, UMap) >= 0, ERR_UELCONFLICT)) return false;
        return true;
    }

    // Brief:
    //   Initialize the reading of a symbol in mapped mode
    // Arguments:
    //   SyNr: The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
    //   NrRecs: The number of records available for reading
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataReadMap, gdxDataReadRawStart, gdxDataReadStrStart, gdxDataReadDone
    int TGXFileObj::gdxDataReadMapStart(int SyNr, int &NrRecs) {
        auto XDomains = utils::arrayWithValue<int, MaxDim>(DOMC_STRICT);
        NrRecs = PrepareSymbolRead("DataReadMapStart", SyNr, XDomains.data(), fr_map_data);
        return NrRecs >= 0;
    }

    // Brief:
    //   Read the next record in mapped mode
    // Arguments:
    //   RecNr: Ignored (left in for backward compatibility)
    //   KeyInt: The index of the record
    //   Values: The data of the record
    //   DimFrst: The first index position in KeyInt that changed
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataReadMapStart, gdxDataReadFilteredStart, gdxDataReadDone
    // Description:
    int TGXFileObj::gdxDataReadMap(int RecNr, int *KeyInt, double *Values, int &DimFrst) {
        TgxModeSet AllowedModes{fr_map_data, fr_mapr_data};
        if((TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) && !CheckMode("DataReadMap", AllowedModes)) return false;
        if(CurSyPtr && CurSyPtr->SScalarFrst) {
            CurSyPtr->SScalarFrst = false;
            GetDefaultRecord(Values);
            DimFrst = 0;
            return true;
        }
        if(fmode == fr_map_data) {
            DimFrst = 0;
            //if(!ReadPtr) return false;
            auto first = (*SortList->begin());
            memcpy(KeyInt, first.first.data(), sizeof(int)*FCurrentDim);
            memcpy(Values, first.second.data(), sizeof(double)*5);
            // checking mapped values
            for(int D{}; D<FCurrentDim; D++) {
                if(KeyInt[D] != PrevElem[D]) {
                    PrevElem[D] = KeyInt[D];
                    if(!DimFrst) DimFrst = D+1;
                }
            }
            return true;
        }

        assert(fmode == fr_mapr_data && "fr_mapr_data expected");

        bool AddNew{}, AddError{}, BadError{};
        int FIDim {FCurrentDim};

    again:
        if(!DoRead(Values, DimFrst)) return false;
        if(FIDim < DimFrst) DimFrst = FIDim;
        FIDim = FCurrentDim;
        if(DimFrst > 0) {
            bool loopDone{};
            for(int D{DimFrst-1}; D<FCurrentDim && !loopDone; D++) {
                const auto &obj = DomainList[D];
                if(LastElem[D] < 0) {
                    ReportError(ERR_BADELEMENTINDEX);
                    BadError = true;
                    break;
                }
                int V;
                switch(obj.DAction) {
                    case dm_unmapped:
                        KeyInt[D] = LastElem[D];
                        break;
                    case dm_filter:
                        V = UELTable.GetUserMap(LastElem[D]);
                        if(obj.DFilter->InFilter(V)) KeyInt[D] = V;
                        else {
                            AddError = true;
                            FIDim = D;
                            loopDone = true;
                        }
                        break;
                    case dm_strict:
                        V = UELTable.GetUserMap(LastElem[D]);
                        if(V >= 0) KeyInt[D] = V;
                        else {
                            AddError = true;
                            FIDim = D+1;
                            loopDone = true;
                        }
                        break;
                    case dm_expand: // no filter, allow growth of domain
                        {
                            int EN = LastElem[D];
                            V = UELTable.GetUserMap(EN);
                            if (V >= 0) KeyInt[D] = V;
                            else {
                                KeyInt[D] = -EN;
                                AddNew = true;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        if(BadError) return false;
        else if(AddError) {
            for(int D{}; D<FCurrentDim; D++) {
                if(LastElem[D] < 0) {
                    ReportError(ERR_BADELEMENTINDEX);
                    return false;
                }
                int V;
                switch(DomainList[D].DAction) {
                    case dm_filter:
                        V = UELTable.GetUserMap(LastElem[D]);
                        if(!DomainList[D].DFilter->InFilter(V))
                            LastElem[D] = -LastElem[D];
                        break;
                    case dm_strict:
                        V = UELTable.GetUserMap(LastElem[D]);
                        if(V < 0)
                            LastElem[D] = -LastElem[D];
                        break;
                    default:
                        break;
                }
            }
            AddToErrorListDomErrs(LastElem, Values); // unmapped
            for(int D{}; D<FCurrentDim; D++)
                if(LastElem[D] < 0) LastElem[D] = -LastElem[D];
            AddError = false;
            goto again;
        }

        if(AddNew) {
            for(int D{}; D<FCurrentDim; D++) {
                int EN = KeyInt[D];
                if(EN < 0) {
                    int V = UELTable.NewUsrUel(-EN);
                    KeyInt[D] = V;
                    NrMappedAdded++;
                    // look for same mapping to be issued
                    for(int D2{D+1}; D2<FCurrentDim; D2++)
                        if(KeyInt[D2] == EN) KeyInt[D2] = V;
                }
            }
        }

        // with all filtered we lost track of AFDIM
        DimFrst = 0;
        for(int D{}; D<FCurrentDim; D++) {
            if(PrevElem[D] != KeyInt[D]) {
                PrevElem[D] = KeyInt[D];
                if(!DimFrst) DimFrst = D+1;
            }
        }

        return true;
    }

    void TGXFileObj::SetTraceLevel(TGXFileObj::TraceLevels tl) {
        TraceLevel = tl;
    }

    void TGXFileObj::SetWriteModes(bool asYAML, bool asText) {
        this->writeAsYAML = asYAML;
        this->writeAsText = asText;

    }

    // Summary:
    //   Number of entries in the acronym table
    // Arguments:
    // Returns:
    //   The number of entries in the acronym table
    // See Also:
    //    gdxAcronymSetInfo, gdxAcronymSetInfo
    int TGXFileObj::gdxAcronymCount() const {
        return AcronymList.size();
    }

    // Summary:
    //   Retrieve acronym information from the acronym table
    // Arguments:
    //   N: Index into acronym table; range from 1 to AcronymCount
    //   AName: Name of the acronym
    //   Txt: Explanatory text of the acronym
    //   AIndx:  Index value of the acronym
    // Returns:
    //   Non-zero if the index into the acronym table is valid; false otherwise
    // See Also:
    //    gdxAcronymSetInfo, gdxAcronymCount
    int TGXFileObj::gdxAcronymGetInfo(int N, std::string &AName, std::string &Txt, int &AIndx) const {
        if(N < 1 || N > AcronymList.size()) {
            AName.clear();
            Txt.clear();
            AIndx = 0;
            return false;
        }
        auto acr = AcronymList[N-1];
        AName = acr.AcrName;
        Txt = acr.AcrText;
        AIndx = acr.AcrMap;
        return true;
    }

    // Summary:
    //   Modify acronym information in the acronym table
    // Arguments:
    //   N: Index into acronym table; range from 1 to AcronymCount
    //   AName: Name of the acronym
    //   Txt: Explanatory text of the acronym
    //   AIndx:  Index value of the acronym
    // Return Value:
    //   Non-zero if the index into the acronym table is valid; false otherwise
    // See Also:
    //   gdxAcronymGetInfo, gdxAcronymCount
    // Description:
    //   When writing a gdx file, this function is used to provide the name of an acronym;
    //     in this case the Indx parameter must match.
    //   When reading a gdx file, this function
    //     is used to provide the acronym index, and the AName parameter must match.
    int TGXFileObj::gdxAcronymSetInfo(int N, const std::string &AName, const std::string &Txt, int AIndx) {
        auto MapIsUnique = [this](int Indx) {
            return std::none_of(AcronymList.begin(), AcronymList.end(), [&](const TAcronym &acr) {
                return acr.AcrReadMap == Indx;
            });
        };

        if(TraceLevel >= TraceLevels::trl_some)
            WriteTrace("AcronymSetInfo: "s + AName + " index = " + std::to_string(AIndx));

        if(ErrorCondition(N >= 1 || N <= AcronymList.size(), ERR_BADACRONUMBER)) return false;
        auto &obj = AcronymList[N-1];
        if(utils::in(fmode, AnyWriteMode) || obj.AcrAutoGen) {
            if(ErrorCondition(IsGoodNewSymbol(AName), ERR_BADACRONAME)) return false;
            if(obj.AcrAutoGen) {
                assert(obj.AcrReadMap == AIndx && "gdxAcronymSetInfo");
                obj.AcrAutoGen = false;
            }
            else if(ErrorCondition(AIndx == obj.AcrMap, ERR_BADACROINDEX)) return false;

        } else if(obj.AcrReadMap != AIndx) {
            if(ErrorCondition(utils::sameText(AName, obj.AcrName), ERR_BADACRONAME) ||
                ErrorCondition(MapIsUnique(AIndx), ERR_ACRODUPEMAP)) return false;
            obj.AcrReadMap = AIndx;
        }
        return true;
    }

    // Summary:
    //   Returns the value of the NextAutoAcronym variable and sets the variable to nv
    // Arguments:
    //    nv: New value for NextAutoAcronym; a value of less than zero is ignored
    // Return Value:
    //    Previous value of NextAutoAcronym
    // Description:
    //    When we read from a gdx file and encounter an acronym that was not defined, we need to assign
    //    a new index for that acronym. The variable NextAutoAcronym is used for this purpose and is
    //    incremented for each new undefined acronym.
    //    When NextAutoAcronym has a value of zero, the default, the value is ignored and the original
    //    index as stored in the gdx file is used for the index.
    int TGXFileObj::gdxAcronymNextNr(int nv) {
        int res {NextAutoAcronym};
        if(nv >= 0) NextAutoAcronym = nv;
        return res;
    }

    // Summary:
    //   Get information how acronym values are remapped
    // Arguments:
    //   N: Index into acronym table; range from 1 to AcronymCount
    //   orgIndx: The Index used in the gdx file
    //   newIndx: The Index returned when reading gdx data
    //   autoIndex: non-zero if the newIndx was generated using the value of NextAutoAcronym
    // Return Value:
    //   Non-zero if the index into the acronym table is valid; false otherwise
    // See Also:
    //   gdxAcronymGetInfo, gdxAcronymCount, gdxAcronymNextNr
    // Description:
    //   When reading gdx data, we need to map indices for acronyms used in the gdx file to
    //   indices used by the reading program. There is a problen when not all acronyms have been
    //   registered before reading the gdx data. We need to map an udefined index we read to a new value.
    //   The value of NextAutoAcronym is used for that.
    //
    int TGXFileObj::gdxAcronymGetMapping(int N, int &orgIndx, int &newIndx, int &autoIndex) {
        if(TraceLevel >= TraceLevels::trl_some)
            WriteTrace("AcronymGetMapping: N = "s + std::to_string(N));
        if(ErrorCondition(N >= 1 || N <= AcronymList.size(), ERR_BADACRONUMBER)) return false;
        auto &obj = AcronymList[N-1];
        orgIndx = obj.AcrMap;
        newIndx = obj.AcrReadMap;
        autoIndex = obj.AcrAutoGen;
        return true;
    }

    // Brief:
    //   Check if there is a filter defined based on its number
    // Arguments:
    //   FilterNr: Filter number as used in FilterRegisterStart
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //  gdxFilterRegisterStart
    // Description:
    //
    int TGXFileObj::gdxFilterExists(int FilterNr) {
        if(!MajorCheckMode("FilterExists"s, AnyReadMode)) return false;
        return FilterList.FindFilter(FilterNr) != nullptr;
    }

    // Brief:
    //   Define a unique element filter
    // Arguments:
    //   FilterNr: Filter number to be assigned
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //  gdxFilterRegister, gdxFilterRegisterDone, gdxDataReadFilteredStart
    // Description:
    //   Start the registration of a filter. A filter is used to map a number
    //   of elements to a single integer; the filter number. A filter number
    //   can later be used to specify a filter for an index postion when reading data.
    int TGXFileObj::gdxFilterRegisterStart(int FilterNr) {
        static const TgxModeSet AllowedModes {fr_init};
        if(!MajorCheckMode("FilterRegisterStart"s, AllowedModes) ||
            ErrorCondition(FilterNr >= 1, ERR_BAD_FILTER_NR)) return false;

        FilterList.AddFilter(TDFilter{ FilterNr, UELTable.UsrUel2Ent.GetHighestIndex() });
        CurFilter = &FilterList.back();
        fmode = fr_filter;
        return true;
    }

    // Brief:
    //   Add a unique element to the current filter definition
    // Arguments:
    //   UelMap: Unique element number in the user index space
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //  gdxFilterRegisterStart, gdxFilterRegisterDone
    // Description:
    //  Register a unique element as part of the current filter. The
    //  function returns false if the index number is out of range of
    //  valid user indices or the index was never mapped into the
    //  user index space.
    int TGXFileObj::gdxFilterRegister(int UelMap) {
        static const TgxModeSet AllowedModes {fr_filter};
        if(TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes) &&
            !CheckMode("FilterRegister"s, AllowedModes)) return false;
        auto &obj = *CurFilter;
        if(ErrorCondition(UelMap >= 1 && UelMap <= obj.FiltMaxUel, ERR_BAD_FILTER_INDX)) return false;
        int EN{UELTable.UsrUel2Ent[UelMap]};
        if (EN >= 1) obj.SetFilter(UelMap, true);
        else {
            ReportError(ERR_FILTER_UNMAPPED);
            return false;
        }
        return true;
    }

    // Brief:
    //   Finish registration of unique elements for a filter
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //  gdxFilterRegisterStart, gdxFilterRegister
    // Description:
    //
    int TGXFileObj::gdxFilterRegisterDone() {
        static const TgxModeSet AllowedModes {fr_filter};
        if(!MajorCheckMode("FilterRegisterDone"s, AllowedModes)) return false;
        fmode = fr_init;
        CurFilter->FiltSorted = true;
        if(UELTable.GetMapToUserStatus() == map_unsorted) {
            int LV {-1};
            for(int N{1}; N<=UELTable.size(); N++) {
                int V{UELTable.GetUserMap(N)};
                if(!CurFilter->InFilter(V)) continue;
                if(V <= LV) {
                    CurFilter->FiltSorted = false;
                    break;
                }
                LV = V;
            }
        }
        CurFilter = nullptr;
        return true;
    }

    // Brief:
    //   Initialize the reading of a symbol in filtered mode
    // Arguments:
    //   SyNr: The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
    //   FilterAction: Array of filter actions for each index position
    //   NrRecs: The maximum number of records available for reading. The actual number of records may be
    //      less when a filter is applied to the records read.
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxFilterRegisterStart, gdxDataReadMap, gdxDataReadRawStart, gdxDataReadStrStart, gdxDataReadDone
    // Description:
    //   Start reading data for a symbol in filtered mode. Each filter action
    //   (1..Dimension) describes how each index should be treated when reading
    //   a data record. When new unique elements are returned, they are added
    //   to the user index space automatically. The actual reading of records
    //   is done with DataReadMap.
    //   <P>The action codes are as follows:
    //   <TABLE>
    //   Action code         Result
    //   -----------         ------
    //   DOMC_UNMAPPED       The index is not mapped into user space
    //   DOMC_EXPAND         New unique elements encountered will be
    //                         be mapped into the user space
    //   DOMC_STRICT         If the unique element in this position does not map into
    //                         user space, the record will not be available and
    //                         is added to the error list instead
    //   FilterNumber        If the unique element in this position does not map
    //                         into user space or is not enabled in this filter,
    //                         the record will not be available and is added to
    //                         the error list instead
    //   </TABLE>
    int TGXFileObj::gdxDataReadFilteredStart(int SyNr, const int* FilterAction, int& NrRecs)
    {
        NrRecs = PrepareSymbolRead("DataReadStartFiltered"s, SyNr, FilterAction, fr_map_data);
        return NrRecs >= 0;
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
        int ix = utils::indexOf(uelNames, id);
        if (ix == -1) {
            uelNames.push_back(id);
            nameToNum[id] = mapping;
            return static_cast<int>(uelNames.size());
        }
        return ix+1;
    }

    bool TUELTable::empty() const {
        return !size();
    }

    std::string TUELTable::operator[](int index) {
        return uelNames[index];
    }

    int TUELTable::GetUserMap(int i) const {
        //return UsrUel2Ent.GetReverseMapping(i);
        return nameToNum.at(uelNames[i-1]);
    }

    void TUELTable::SetUserMap(int EN, int N) {
        nameToNum[uelNames[EN-1]] = N;
    }

    void TUELTable::ResetMapToUserStatus() {
        FMapToUserStatus = map_unknown;
    }

    int TUELTable::NewUsrUel(int EN) {
        auto maxKey = UsrUel2Ent.GetHighestIndex();
        UsrUel2Ent[maxKey+1] = EN;
        ResetMapToUserStatus();
        return maxKey+1;
    }

    int TUELTable::AddUsrNew(const std::string &s) {
        int EN{ AddObject(s, -1) };
        int res{ nameToNum[uelNames[EN - 1]] };
        if (res < 0) {
            res = UsrUel2Ent.GetHighestIndex() + 1;
            nameToNum[uelNames[EN-1]] = res;
            UsrUel2Ent[res] = EN;
        }
        ResetMapToUserStatus();
        return res;
    }

    TUELUserMapStatus TUELTable::GetMapToUserStatus() {
        if(FMapToUserStatus == map_unknown) {
            int LV {-1};
            bool C {true};
            FMapToUserStatus = map_sortgrow;
            for(int N{1}; N<=size(); N++) {
                int V = GetUserMap(N);
                if(V < 0) C = false;
                else if(V > LV) {
                    LV = V;
                    if(!C) FMapToUserStatus = map_sorted;
                } else {
                    FMapToUserStatus = map_unsorted;
                    break;
                }
            }
            if(FMapToUserStatus == map_sortgrow && C)
                FMapToUserStatus = map_sortfull;
        }
        return FMapToUserStatus;
    }

    // FIXME: How does this affect the ordering / sort list?
    // Should renaming change the index?
    void TUELTable::RenameEntry(int N, const std::string &s) {
        std::string oldName = uelNames[N];
        uelNames[N] = s;
        nameToNum.erase(oldName);
        nameToNum[s] = N+1;
    }

    int TUELTable::AddUsrIndxNew(const std::string &s, int UelNr) {
        int EN {AddObject(s, -1)};
        int res {nameToNum[uelNames[EN-1]]};
        if (res < 0) {
            res = UelNr;
            nameToNum[s] = res;
            UsrUel2Ent[res] = EN;
        }
        else if (res != UelNr)
            res = -1;
        ResetMapToUserStatus();
        return res;
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

    void TFilterList::AddFilter(const TDFilter& F)
    {
        for (int N{}; N < size(); N++) {
            if ((*this)[N].FiltNumber == F.FiltNumber) {
                this->erase(this->begin() + N);
                break;
            }
        }
        this->push_back(F);
    }

    UNIT_INIT_FINI();

    int TIntegerMapping::GetHighestIndex() const {
        return std::max<int>(0, static_cast<int>(Map.size()) - 1);
    }

    void TIntegerMapping::SetMapping(int F, int T) {
        if(F >= Map.size()) {
            Map.resize(F + 1);
            assert(F+1 < FMAXCAPACITY && "Already at maximum capacity: cannot grow TIntegerMapping");
        }
        Map[F] = T;
    }

    int TIntegerMapping::GetMapping(int F) const {
        return F >= 0 && F < Map.size() ? Map[F] : -1;
    }

    void TIntegerMapping::clear() {
        Map.clear();
    }

    int &TIntegerMapping::operator[](int index) {
        if(index >= Map.size()) {
            Map.resize(index+1);
            assert(index+1 < FMAXCAPACITY && "Already at maximum capacity: cannot grow TIntegerMapping");
        }
        return Map[index];
    }

    int TIntegerMapping::MemoryUsed() {
        return static_cast<int>(Map.size() * sizeof(int));
    }

    bool TIntegerMapping::empty() const {
        return Map.empty();
    }

    int TIntegerMapping::GetReverseMapping(int T) const {
        for (int i{}; i < Map.size(); i++) {
            if (Map[i] == T)
                return i;
        }
        return -1;
    }
}