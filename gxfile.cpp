#include "rtl/sysutils_p3.h"
#include "gdlib/gmsstrm.h"
#include "expertapi/palmcc.h"

#include "gxfile.h"
#include "utils.h"

#include <cassert>
#include <numeric>

#ifdef YAML
#include "yaml.h"
#define WRYAML(instr) instr;
#else
#define WRYAML(instr) ;
#endif

using namespace gdxinterface;
using namespace gdlib::gmsstrm;
using namespace std::literals::string_literals;

namespace gxfile {

    bool CanBeQuoted(const std::string &s);
    bool GoodUELString(const std::string &s);
    int64_t dblToI64(double x);

    bool CanBeQuoted(const std::string &s) {
        bool saw_single{}, saw_double{};
        for(auto Ch : s) {
            if(Ch == '\'') {
                if(saw_double) return false;
                saw_single = true;
            } else if(Ch == '\"') {
                if(saw_single) return false;
                saw_double = true;
            } else if(static_cast<unsigned char>(Ch) < ' ') return false;
        }
        return true;
    }

    bool GoodUELString(const std::string &s) {
        return s.length() <= GLOBAL_UEL_IDENT_SIZE-1 && CanBeQuoted(s); // also checks Ch < '
    }

    const int MaxDimV148 = 10;
    using TIndex = std::array<int, GLOBAL_MAX_INDEX_DIM>;

#ifdef GAMSBUILD
    static std::string gdlSetSystemName() {
        palHandle_t pal;
        char msg[256];
        if (!palCreate(&pal, msg, sizeof(msg)))
            printf("error");
        palSetSystemName(pal, "GDX Library");
        palGetAuditLine(pal, msg);
        palFree(&pal);
        return msg;
    }
    static std::string auditLine { gdlSetSystemName() };
#else
    static std::string auditLine {"GDX Library      00.0.0 ffffffff May  4, 1970  (AUDIT) XYZ arch xybit/myOS"};
#endif

#ifdef CPP_HASHMAP
    using UELTableImplChoice = TUELTable;
#else
    using UELTableImplChoice = TUELTableLegacy;
#endif

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

    std::string DLLLoadPath {};

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

    int ConvertGDXFile(const std::string &fn, const std::string &MyComp) {
        std::string Conv {utils::trim(utils::uppercase(rtl::sysutils_p3::QueryEnvironmentVariable(strGDXCONVERT)))};
        if(Conv.empty()) Conv = "V7"s;
        std::string Comp = Conv == "V5" ? ""s : (!GetEnvCompressFlag() ? "U" : "C");
        if(utils::sameText(Conv+Comp, "V7"+MyComp)) return 0;
        return system(("gdxcopy -"s + Conv+Comp + " -Replace "s + utils::quoteWhitespace(fn, '\"')).c_str());
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
        const int MaxNameLen = 63; // starting version 149
        if(!S.empty() && (int)S.length() <= MaxNameLen && isalpha(S.front())) {
            for(int n{1}; n<(int)S.length(); n++)
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

    void copyIntlMapDblToI64(const TIntlValueMapDbl &dMap, TIntlValueMapI64 &iMap);

    void copyIntlMapDblToI64(const TIntlValueMapDbl &dMap, TIntlValueMapI64 &iMap) {
        for(int k=0; k<(int)iMap.size(); k++) {
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
        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::cout << "gdxOpenWrite(" << FileName << ")" << std::endl;
        }

        if(fmode != f_not_open) {
            ErrNr = ERR_FILEALREADYOPEN;
            return false;
        }
        if(FileName.empty()) {
            ErrNr = ERR_NOFILE;
            LastError = ErrNr;
            return false;
        }
        FFile = std::make_unique<TMiBufferedStreamDelphi>(FileName, fmCreate);
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
        ReadPtr = std::nullopt;
        MajContext = "OpenWrite";
        TraceLevel = defaultTraceLevel;
        InitErrors();
        NameList = std::make_unique<TNameList>();
        NameList->OneBased = true;
        UELTable = std::make_unique<UELTableImplChoice>();
        AcronymList.clear();
        FilterList.clear();
        FFile->WriteByte(gdxHeaderNr);
        FFile->WriteString(gdxHeaderId);
        VersionRead = VERSION;
        FFile->WriteInteger(VersionRead);
        FFile->WriteInteger(Compr);
        FileSystemID = auditLine;
        FFile->WriteString(FileSystemID);
        FProducer = Producer;
        FProducer2.clear();
        FFile->WriteString(FProducer);
        // Reserve some space for positions
        MajorIndexPosition = FFile->GetPosition();
        for(int N{1}; N<=10; N++) FFile->WriteInt64(0);
        SetTextList = std::make_unique<TSetTextList>();
        SetTextList->OneBased = false;
        SetTextList->AddObject(""s, 0);
        gdxResetSpecialValues();
        NextWritePosition = FFile->GetPosition();
        fmode = fw_init;
        DomainStrList = std::make_unique<std::vector<std::string>>();

#ifdef YAML
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
#endif

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
        LastStrElem.fill(std::nullopt);
        SortList = std::make_unique<LinkedDataType>(FCurrentDim, DataSize * static_cast<int>(sizeof(double)));
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
        if(fmode == fw_dom_str) fmode = fw_str_data;
        if(TraceLevel >= TraceLevels::trl_all || fmode != fw_str_data) {
            if(!CheckMode("DataWriteStr", fw_str_data)) return false;
            std::cout << "  Index =\n";
            for(int D{}; D<FCurrentDim; D++)
                std::cout << " " << KeyStr[D] << (D+1 < FCurrentDim ? "," : "") << "\n";
        }
        for(int D{}; D<FCurrentDim; D++) {
            std::string SV {utils::trimRight(KeyStr[D])};
            if(!LastStrElem[D] || SV != (*LastStrElem[D])) {
                // -1=not found, >=1 found
                int KD {UELTable->IndexOf(SV)};
                if(KD == -1) {
                    if(ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
                    KD = UELTable->AddObject(SV, -1);
                }
                LastElem[D] = KD;
                LastStrElem[D] = std::make_optional<std::string>(SV);
                if(KD < MinElem[D]) MinElem[D] = KD;
                if(KD > MaxElem[D]) MaxElem[D] = KD;
            }
        }
        SortList->AddItem(LastElem.data(), Values);
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
        static const TgxModeSet AllowedModes {fw_raw_data,fw_map_data,fw_str_data, fw_dom_raw, fw_dom_map, fw_dom_str};
        if(!MajorCheckMode("DataWriteDone"s, AllowedModes)) return false;
        if(!utils::in(fmode, fw_raw_data, fw_dom_raw)) {
            InitDoWrite(static_cast<int>(SortList->Count()));
            ReadPtr = SortList->StartRead(nullptr);
            TIndex AElements;
            TgdxValues AVals;
#ifdef TLD_LEGACY
            while (ReadPtr && SortList->GetNextRecord(&*ReadPtr, AElements.data(), AVals.data()))
#else
            while(ReadPtr && SortList->GetNextRecord(/*&*/*ReadPtr, AElements.data(), AVals.data()))
#endif
                DoWrite(AElements.data(), AVals.data());
            SortList = nullptr;
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
        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::cout << "gdxClose(" << FFile->GetFileName() << ")" << std::endl;
        }
        std::string fnConv;
        if(utils::in(fmode, fw_raw_data, fw_map_data, fw_str_data)) // unfinished write
            gdxDataWriteDone();
        if(fmode == fw_init) {
            fnConv = FFile->GetFileName();
            FFile->SetCompression(CompressOut);
            FFile->SetPosition(NextWritePosition);
            int64_t SymbPos = NextWritePosition;
            FFile->WriteString(MARK_SYMB);
            FFile->WriteInteger(static_cast<int>(NameList->size()));
#ifdef YAML
            YFile->AddKeyItem("symbols");
            YFile->IncIndentLevel();
#endif
            for(int N{1}; N<=NameList->Count(); N++) {
                FFile->WriteString(NameList->GetString(N));
                const auto PSy = *NameList->GetObject(N);
                FFile->WriteInt64(PSy->SPosition);
                FFile->WriteInteger(PSy->SDim);
                FFile->WriteByte(PSy->SDataType);
                FFile->WriteInteger(PSy->SUserInfo);
                FFile->WriteInteger(PSy->SDataCount);
                FFile->WriteInteger(PSy->SErrors);
                FFile->WriteByte(PSy->SSetText);
                FFile->WriteString(PSy->SExplTxt);
                FFile->WriteByte(PSy->SIsCompressed);

#ifdef YAML
                if(writeAsYAML) {
                    YFile->AddKey(NameList->GetString(N));
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

                YFile->AddKeyValue("has_domain_symbols", (PSy->SDomSymbols ? "yes"s : "no"s));
#endif
                FFile->WriteByte(PSy->SDomSymbols ? 1 : 0);

                if(PSy->SDomSymbols) {
#ifdef YAML
                    YFile->AddKeyItem("domain_symbols");
                    YFile->IncIndentLevel();
#endif
                    for(const auto &SDomSym : *PSy->SDomSymbols) {
                        FFile->WriteInteger(SDomSym);
                        WRYAML(YFile->AddItem(std::to_string(SDomSym)));
                    }
                    WRYAML(YFile->DecIndentLevel());
                }

                int CommCnt {static_cast<int>(PSy->SCommentsList.size())};
                FFile->WriteInteger(CommCnt);
                if(CommCnt) {
#ifdef YAML
                    YFile->AddKeyItem("comments");
                    YFile->IncIndentLevel();
#endif
                    for (int Cnt{}; Cnt < CommCnt; Cnt++) {
                        FFile->WriteString(PSy->SCommentsList[Cnt]);
                        WRYAML(YFile->AddItem(PSy->SCommentsList[Cnt]));
                    }
                    WRYAML(YFile->DecIndentLevel());
                }

#ifdef YAML
                if(writeAsYAML) YFile->DecIndentLevel();
#endif
            }
            FFile->WriteString(MARK_SYMB);
            WRYAML(YFile->DecIndentLevel());

            auto SetTextPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_SETT);
            WRYAML(YFile->AddKeyItem("set_texts"));
            WRYAML(YFile->IncIndentLevel());
            FFile->WriteInteger(static_cast<int>(SetTextList ? SetTextList->size() : 0));
            if(SetTextList) {
                for (int N{}; N < static_cast<int>(SetTextList->Count()); N++) {
                    const std::string& SetText = SetTextList->GetString(N);
                    FFile->WriteString(SetText);
                    WRYAML(YFile->AddItem(SetText));
                }
            }
            FFile->WriteString(MARK_SETT);
            WRYAML(YFile->DecIndentLevel());

            auto UELPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_UEL);
            WRYAML(YFile->AddKeyItem("uels"));
            WRYAML(YFile->IncIndentLevel());
            FFile->WriteInteger(UELTable ? UELTable->size() : 0);
            if(UELTable) {
                for(int i=0; i<UELTable->size(); i++) {
                    const std::string &uelName = (*UELTable)[i];
                    FFile->WriteString(uelName);
                    WRYAML(YFile->AddItem(uelName));
                }
            }
            FFile->WriteString(MARK_UEL);
            WRYAML(YFile->DecIndentLevel());

            auto AcronymPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_ACRO);
            WRYAML(YFile->AddKeyItem("acronyms"));
            WRYAML(YFile->IncIndentLevel());
            FFile->WriteInteger(static_cast<int>(AcronymList.size()));
            for(const auto &acro : AcronymList) {
                const auto acroName = acro.AcrName.empty() ? "UnknownACRO" + std::to_string(acro.AcrMap) : acro.AcrName;
                FFile->WriteString(acroName);
                FFile->WriteString(acro.AcrText);
                FFile->WriteInteger(acro.AcrMap);
                WRYAML(YFile->AddKeyValue(acroName, "{text:"s +  acro.AcrText + ", map:"s + std::to_string(acro.AcrMap) + "}"s));
            }
            FFile->WriteString(MARK_ACRO);
            WRYAML(YFile->DecIndentLevel());

            auto DomStrPos {static_cast<int64_t>(FFile->GetPosition())};
            FFile->SetCompression(CompressOut);
            FFile->WriteString(MARK_DOMS);
            FFile->WriteInteger(DomainStrList ? static_cast<int>(DomainStrList->size()) : 0);
            WRYAML(YFile->AddKeyItem("domain_strings"));
            WRYAML(YFile->IncIndentLevel());
            if(DomainStrList) {
                for (const auto &DomStr: *DomainStrList) {
                    FFile->WriteString(DomStr);
                    WRYAML(YFile->AddItem(DomStr));
                }
            }
            FFile->WriteString(MARK_DOMS);
            WRYAML(YFile->DecIndentLevel());

            WRYAML(YFile->AddKeyItem("symbol_domains"));
            WRYAML(YFile->IncIndentLevel());
            int ix{1};
            for(int N{1}; N<=NameList->Count(); N++) {
                const auto PSy = *(*NameList)[N];
                if(PSy->SDomStrings) {
                    WRYAML(YFile->AddKeyItem(NameList->GetString(N)));
                    WRYAML(YFile->IncIndentLevel());
                    FFile->WriteInteger(ix);
                    for(const auto &i : (*PSy->SDomStrings)) {
                        FFile->WriteInteger(i);
                        WRYAML(YFile->AddItem(std::to_string(i)));
                    }
                    WRYAML(YFile->DecIndentLevel());
                }
                ix++;
            }
            FFile->WriteInteger(-1);
            FFile->WriteString(MARK_DOMS);
            WRYAML(YFile->DecIndentLevel());

            // This must be at the very end!!!
            FFile->SetPosition(MajorIndexPosition);
            FFile->SetCompression(false);
            FFile->WriteInteger(MARK_BOI);
            // Note that we have room for 10 indices; if we need more, create an overflow link in the 10th position.
            const std::array<int64_t, 6> offsets = {SymbPos, UELPos, SetTextPos, AcronymPos, NextWritePosition, DomStrPos};
            const std::array offsetNames = {"symbol_table"s, "uel_table"s, "set_texts"s, "acronyms"s, "next_write"s, "domain_strs"s };
            for(int64_t offset : offsets)
                FFile->WriteInt64(offset);

#ifdef YAML
            YFile->AddKeyItem("section_offsets");
            YFile->IncIndentLevel();
            for(int i{}; i<(int)offsets.size(); i++)
                YFile->AddKeyValue(offsetNames[i], (int)offsets[i]);
            YFile->DecIndentLevel();
#endif
        }

        int res{FFile ? FFile->GetLastIOResult() : 1};

        // Many free operations. Some not necessary anymore due to RAII pattern (out of scope -> destroy)
        if(NameList) {
            for (int N{1}; N <= NameList->Count(); N++) {
                const auto PSy = *NameList->GetObject(N);
                //PSy->SDomSymbols = nullptr;
                //PSy->SCommentsList.clear();
                //PSy->SSetBitMap = nullptr;
                //PSy->SDomStrings = nullptr;
                delete PSy;
            }
            NameList = nullptr;
        }

        FFile = nullptr;
        SetTextList = nullptr;
        UELTable = nullptr;
        SortList = nullptr;
        DomainStrList = nullptr;

        ErrorList.clear();
        FilterList.clear();
        AcronymList.clear();
        MapSetText.clear();

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

    TGXFileObj::TGXFileObj(std::string &ErrMsg) : fstatus{stat_notopen}, TraceLevel{TraceLevels::trl_none} {
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
        intlValueMapDbl[vm_valund] = GMS_SV_UNDEF;
        intlValueMapDbl[vm_valna ] = GMS_SV_NA;
        intlValueMapDbl[vm_valpin] = GMS_SV_PINF;
        intlValueMapDbl[vm_valmin] = GMS_SV_MINF;
        intlValueMapDbl[vm_valeps] = GMS_SV_EPS;
        intlValueMapDbl[vm_zero] = 0.0;
        intlValueMapDbl[vm_one] = 1.0;
        intlValueMapDbl[vm_mone] = -1.0;
        intlValueMapDbl[vm_half] = 0.5;
        intlValueMapDbl[vm_two] = 2.0;
        readIntlValueMapDbl = intlValueMapDbl;

        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::cout << "reset special vals, dump of readIntlValueMapDbl" << std::endl;
            std::array svNames {"undef"s, "na"s, "posinf"s, "min"s, "eps"s};
            std::array svIndices {sv_valund, sv_valna, sv_valpin, sv_valmin, sv_valeps};
            for(int i=0; i<(int)svNames.size(); i++)
                std::cout << svNames[i] << "=" << readIntlValueMapDbl[svIndices[i]] << std::endl;
        }

        copyIntlMapDblToI64(intlValueMapDbl, intlValueMapI64);
        Zvalacr = GMS_SV_ACR;
        return true;
    }

    bool TGXFileObj::PrepareSymbolWrite(const std::string &Caller,
                                        const std::string &AName,
                                        const std::string &AText,
                                        int ADim,
                                        int AType,
                                        int AUserInfo) {
        CurSyPtr = nullptr;
        ErrorList.clear();
        SortList = nullptr;

        if(!MajorCheckMode(Caller, fw_init)) return false;

        if(TraceLevel >= TraceLevels::trl_some)
            WriteTrace("Symbol = "s + AName + ", Dim = "s + std::to_string(ADim));

        if(!IsGoodNewSymbol(AName)) return false;

        if( ErrorCondition(ADim >= 0 && ADim <= GLOBAL_MAX_INDEX_DIM, ERR_BADDIMENSION) ||
            ErrorCondition(AType >= 0 && AType <= dt_equ, ERR_BADDATATYPE)) return false;
        CurSyPtr = new TgdxSymbRecord{};
        auto &obj = CurSyPtr;
        obj->SPosition = CurSyPtr->SDataCount = CurSyPtr->SErrors = 0; // Position
        obj->SDim = ADim;
        obj->SDataType = static_cast<gdxSyType>(AType);
        obj->SUserInfo = AUserInfo;
        obj->SSetText = false;
        obj->SExplTxt = MakeGoodExplText(AText);
        obj->SIsCompressed = CompressOut && ADim > 0;
        obj->SCommentsList.clear();
        obj->SDomSymbols = nullptr;
        obj->SDomStrings = nullptr;
        obj->SSetBitMap = utils::in((gdxSyType)AType, dt_set, dt_alias) && ADim == 1 && StoreDomainSets ?
                std::make_unique<std::vector<bool>>() : nullptr;

        CurSyPtr->SSyNr = CurSyPtr->SSyNrActual = NameList->AddObject(AName, CurSyPtr); // +1 for universe
        FCurrentDim = ADim;
        // old case; we never write V6
        // V = 0..Dim which dimension changed
        // V = Dim+1 .. 254 change in last dimension = V - Dim
        // 255 is EOF
        DeltaForWrite = 255 - (VERSION <= 6 ? MaxDimV148 : FCurrentDim) - 1;

        DataSize = DataTypSize[AType];
        if(DataSize > 0)
            LastDataField = DataSize - 1;

        for(int D{}; D<FCurrentDim; D++) {
            LastElem[D] = INDEX_INITIAL;
            MinElem[D] = std::numeric_limits<int>::max();
            MaxElem[D] = 0;
            WrBitMaps[D] = nullptr;
        }

        FFile->SetCompression(CurSyPtr->SIsCompressed);
        return true;
    }

    bool TGXFileObj::MajorCheckMode(const std::string& Routine, TgxFileMode m) {
        MajContext = Routine;
        LastRepError = ERR_NOERROR;
        return !((TraceLevel >= TraceLevels::trl_some || fmode != m) && !CheckMode(Routine, m));
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
        return !(ErrorCondition(NameList->IndexOf(s) < 1, ERR_DUPLICATESYMBOL) ||
                 ErrorCondition(utils::indexOf<TAcronym>(AcronymList, [&s](auto acro) { return utils::sameText(acro.AcrName, s); }) == -1, ERR_DUPLICATESYMBOL) ||
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
            char s[GMS_SSSIZE];
            this->gdxErrorStr(N, s);
            std::cout << "Error = " << N << " : " << s << "\n";
        }
        SetError(N);
        LastRepError = N;
    }

    bool TGXFileObj::CheckMode(const std::string& Routine, TgxFileMode m) {
        if (fmode == m) {
            WriteTrace(Routine);
            return true;
        }
        SetError(ERR_BADMODE);
        std::cout << "**** Error: " << Routine << " called out of context\n";
        if (!MajContext.empty() && !utils::sameText(MajContext, Routine))
            std::cout << "     Previous major function called was " << MajContext << '\n';
        std::cout << "     Current context = " << fmode_str[fmode] << '\n';
        std::cout << "     Allowed = {";
        bool f{ true };
        for (int M{}; M < tgxfilemode_count; M++) {
            if (static_cast<TgxFileMode>(M) == m) {
                if (f) f = false;
                else std::cout << ',';
                std::cout << fmode_str[M];
            }
        }
        std::cout << "}\n";
        return false;
    }

    bool TGXFileObj::CheckMode(const std::string &Routine) {
        WriteTrace(Routine);
        return true;
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
        std::cout << "}\n";
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
        SortList = nullptr;

        if (!MajorCheckMode(Caller, fr_init)) {
            fmode = fr_init;
            return -1;
        }
        ReadUniverse = !SyNr;
        if (!ReadUniverse) {
            if (ErrorCondition(SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX)) return -1;
            CurSyPtr = *NameList->GetObject(SyNr);
            if (CurSyPtr->SDataType == dt_alias) {
                do {
                    SyNr = CurSyPtr->SUserInfo;
                    if (!SyNr) {
                        ReadUniverse = true;
                        break;
                    }
                    CurSyPtr = *NameList->GetObject(SyNr);
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
            LastDataField = DataSize - 1;
            NrRecs = UelCntOrig;
            UniverseNr = 0;
            CurSyPtr = nullptr;
        }
        else {
            FCurrentDim = CurSyPtr->SDim;
            FFile->SetCompression(CurSyPtr->SIsCompressed);
            FFile->SetPosition(CurSyPtr->SPosition);
            DataSize = DataTypSize[CurSyPtr->SDataType];
            if (DataSize > 0) LastDataField = DataSize - 1;
            NrRecs = CurSyPtr->SDataCount;
        }
        if(verboseTrace && TraceLevel >= TraceLevels::trl_some) {
            std::string ts {"Symbol = "s + std::to_string(SyNr)};
            if(CurSyPtr)
                ts += ", Dim = "s + std::to_string(CurSyPtr->SDim);
            WriteTrace(ts);
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
                //WriteLn('Changing mapped read to raw read');
                res = NrRecs;
                newmode = fr_mapr_data;
            } else {
                //WriteLn('Cannot do mapped read fast');
                try {
                    SortList = std::make_unique<LinkedDataType>(FCurrentDim, static_cast<int>(DataSize * sizeof(double)));
                    int FIDim = FCurrentDim; // First invalid dimension
                    TgdxValues Avals;
                    TIndex AElements{};
                    int AFDim, V;
                    bool AddNew{}, AddError{}, BadError{};
                    while(DoRead(Avals.data(), AFDim)) {
                        if(FIDim < AFDim) AFDim = FIDim;
                        FIDim = FCurrentDim;
                        int D;
                        for(D=AFDim-1; D<FCurrentDim; D++) {
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
                                    V = UELTable->GetUserMap(LastElem[D]);
                                    if(obj.DFilter->InFilter(V))
                                        AElements[D] = V;
                                    else {
                                        AddError = true;
                                        FIDim = D;
                                        break;
                                    }
                                    break;
                                case dm_strict:
                                    V = UELTable->GetUserMap(LastElem[D]);
                                    if(V >= 0) AElements[D] = V;
                                    else {
                                        AddError = true;
                                        FIDim = D;
                                    }
                                    break;
                                case dm_expand: {
                                        int EN = LastElem[D];
                                        V = ExpndList.GetMapping(EN);
                                        if(V >= 0) AElements[D] = V;
                                        else {
                                            V = UELTable->GetUserMap(EN);
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
                                        V = UELTable->NewUsrUel(-EN);
                                        AElements[D] = ExpndList[-EN] = V;
                                        NrMappedAdded++;
                                        // look for same mapping to be issued
                                        for(int D2{D+1}; D2 < FCurrentDim; D2++) {
                                            if(AElements[D2] == EN) AElements[D2] = V;
                                        }
                                    }
                                }
                                AddNew = false;
                            }
                            SortList->AddItem(AElements.data(), Avals.data());
                        }
                    }
                    ReadPtr = SortList->StartRead(nullptr);
                    res = static_cast<int>(SortList->Count());
                } catch(std::exception &e) {
                    std::cout << "Exception: " << e.what() << "\n";
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
            SortList = nullptr;
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

#ifdef YAML
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
#endif
    }

    /* we have to make these mask "constants" vars since we cannot
    * have large constants on input
    */
    bool HAVE_MEM;
    int64_t signMask {(int64_t)0x80000000 << 32},
            expoMask {(int64_t)0x7ff00000 << 32},
            mantMask {~(signMask | expoMask)};

    enum TDblClass {
        DBL_NAN,       /* any sort of NaN */
        DBL_NINF,      /* negative infinity */
        DBL_PINF,      /* positive infinity */
        DBL_FINITE     /* positive infinity */
    };

    TDblClass dblInfo(double x, int64_t &i);

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
        return !(index < 0 || index >= (int)bmap.size()) && bmap[index];
    }

    bool TGXFileObj::DoWrite(const int* AElements, const double* AVals)
    {
        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::cout << "DoWrite index: ";
            for(int D{}; D<FCurrentDim; D++)
                std::cout << std::to_string(AElements[D]) << (D+1 < FCurrentDim ? "," : "");
            std::cout << '\n';
        }

        int FDim{ FCurrentDim + 1 }, delta{};
        for (int D{}; D < FCurrentDim; D++) {
            if (WrBitMaps[D] && !accessBitMap(*WrBitMaps[D], AElements[D])) {
                ReportError(ERR_DOMAINVIOLATION);
                TgdxUELIndex ErrorUELs{};
                // FIXME: AS: Are index boundaries right here?
                for (int DD{}; DD < D; DD++)
                    ErrorUELs[DD] = AElements[DD];
                ErrorUELs[D] = -AElements[D];
                // see if there are more domain violations
                // FIXME: AS: Are index boundaries right here?
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
                        int v { static_cast<int>(std::round(X / Zvalacr)) };
                        int ix { utils::indexOf<TAcronym>(AcronymList, [&v](const TAcronym& acro) { return acro.AcrMap == v; }) };
                        if (ix == -1) AcronymList.push_back(TAcronym{ "", "", v });
                    }
                }
            }
            if(verboseTrace && TraceLevel >= TraceLevels::trl_all)
                std::cout << "level=" << AVals[GMS_VAL_LEVEL] << std::endl;
        }
        DataCount++;
        if (utils::in(CurSyPtr->SDataType, dt_set, dt_alias)) {
            if (AVals[GMS_VAL_LEVEL] != 0.0) CurSyPtr->SSetText = true;
            if (FCurrentDim == 1 && CurSyPtr->SSetBitMap) {
                auto& ssbm = *CurSyPtr->SSetBitMap;
                if ((int)ssbm.size() <= LastElem.front())
                    ssbm.resize(LastElem.front()+1, false);
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
                AVals[GMS_VAL_LEVEL] = 0.0;
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
                assert(D>=0);
                switch(ElemType[D]) {
                    case sz_integer: LastElem[D] = FFile->ReadInteger() + MinElem[D]; break;
                    case sz_word: LastElem[D] = FFile->ReadWord() + MinElem[D]; break;
                    case sz_byte: LastElem[D] = FFile->ReadByte() + MinElem[D]; break;
                }
            }
        }
        if(DataSize > 0) {
            for(int DV{GMS_VAL_LEVEL}; DV<=LastDataField; DV++) {
                uint8_t BSV;
                FFile->Read(&BSV, 1);
                TgdxIntlValTyp SV {static_cast<TgdxIntlValTyp>(BSV)};
                AVals[DV] = SV != vm_normal ? readIntlValueMapDbl[SV] : maybeRemap(DV, FFile->ReadDouble());
            }
            if(!MapSetText.empty() && AVals[GMS_VAL_LEVEL] != 0.0 && CurSyPtr->SDataType == dt_set) { // remap settext number
                double X {AVals[GMS_VAL_LEVEL]};
                int D {static_cast<int>(std::round(X))};
                if(std::abs(X-D) < 1e-12 && D >= 0 && D < SetTextList->size())
                    AVals[GMS_VAL_LEVEL] = MapSetText[D];
            }
            if(verboseTrace && TraceLevel >= TraceLevels::trl_all)
                std::cout << "level=" << AVals[GMS_VAL_LEVEL] << std::endl;
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

    void TGXFileObj::AddToErrorListDomErrs(const std::array<int, GLOBAL_MAX_INDEX_DIM> &AElements, const double * AVals) {
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
        ErrorList[ utils::asArrayN<int, GLOBAL_MAX_INDEX_DIM>(AElements, FCurrentDim) ] = utils::asArray<double, 5>(AVals);
    }

    bool TGXFileObj::ResultWillBeSorted(const int *ADomainNrs) {
        for(int D{}; D<FCurrentDim; D++) {
            switch(ADomainNrs[D]) {
                case DOMC_UNMAPPED: continue;
                case DOMC_EXPAND:
                    if(UELTable->GetMapToUserStatus() == map_unsorted) return false;
                    if(!D) {
                        if(UELTable->GetMapToUserStatus() >= map_sortgrow) continue;
                        else return false;
                    } else if(UELTable->GetMapToUserStatus() == map_sortfull) continue;
                    else return false;
                    break;
                case DOMC_STRICT:
                    if(UELTable->GetMapToUserStatus() == map_unsorted) return false;
                    break;
                default:
                    if(UELTable->GetMapToUserStatus() >= map_sorted) continue;
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
            Avals[GMS_VAL_LEVEL] = 0.0;
            break;
        case dt_var:
            ui = CurSyPtr->SUserInfo;
            memcpy(Avals, ui >= GMS_VARTYPE_UNKNOWN && ui <= GMS_VARTYPE_SEMIINT ? gmsDefRecVar[ui] : gmsDefRecVar[GMS_VARTYPE_UNKNOWN], sizeof(double)*5);
            break;
        case dt_equ:
            ui = CurSyPtr->SUserInfo;
            memcpy(Avals, ui >= GMS_EQUTYPE_E && ui <= GMS_EQUTYPE_E + (GMS_EQUTYPE_B + 1) ? gmsDefRecEqu[ui] : gmsDefRecEqu[GMS_EQUTYPE_E], sizeof(double)*5);
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
    int TGXFileObj::gdxErrorStr(int ErrNr, char *ErrMsg) {
        return gdxErrorStrStatic(ErrNr, ErrMsg);
    }

    int TGXFileObj::gdxErrorStrStatic(int ErrNr, char * ErrMsg) {
        const auto it = errorCodeToStr.find(ErrNr);
        utils::assignStrToBuf(it == errorCodeToStr.end() ? rtl::sysutils_p3::SysErrorMessage(ErrNr) : it->second, ErrMsg, GMS_SSSIZE);
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
    int TGXFileObj::gdxFileVersion(char *FileStr, char *ProduceStr) {
        utils::assignStrToBuf(FileSystemID, FileStr, GMS_SSSIZE);
        utils::assignStrToBuf(FProducer, ProduceStr, GMS_SSSIZE);
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
        if(SyId == "*"s) {
            SyNr = 0;
            return true;
        }
        if(NameList) {
            SyNr = NameList->IndexOf(SyId);
            return SyNr >= 1;
        }
        return false;
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
        if ((TraceLevel >= TraceLevels::trl_all || fmode != fr_str_data) && !CheckMode("DataReadStr", fr_str_data))
            return false;
        if (!DoRead(Values, DimFrst)) {
            gdxDataReadDone();
            return false;
        }
        else {
            for (int D{}; D < FCurrentDim; D++) {
                int LED{ LastElem[D] };
                utils::assignStrToBuf(LED >= 1 && LED <= (UELTable ? UELTable->size() : 0) ?
                    (*UELTable)[LED-1] :
                    BADUEL_PREFIX + std::to_string(LED),
                    KeyStr[D], GLOBAL_UEL_IDENT_SIZE);
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
        static const TgxModeSet AllowedMode {fr_init,fr_raw_data,fr_map_data,fr_mapr_data, fr_str_data,fr_slice};
        SortList = nullptr;
        CurSyPtr = nullptr;
        if(!MajorCheckMode("DataReadDone", AllowedMode)) {
            fmode = fr_init;
            return false;
        }
        if(fmode == fr_slice) {
            for(int D{}; D<GLOBAL_MAX_INDEX_DIM; D++) {
                SliceIndxs[D].clear();
                SliceRevMap[D].clear();
            }
        }
        if(NrMappedAdded) {
            int HighestIndex = UELTable->UsrUel2Ent.GetHighestIndex();
            for(int N{HighestIndex}; N >= HighestIndex - NrMappedAdded + 1; N--) {
                assert(N >= 1 && "Wrong entry number");
                int EN {UELTable->UsrUel2Ent.GetMapping(N)}; // nr in ueltable
#ifndef NDEBUG
                int d {UELTable->GetUserMap(EN)};
                assert((d == -1 || d == N) && "Mapped already");
#endif
                UELTable->SetUserMap(EN, N); // map to user entry
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
    int TGXFileObj::gdxSymbolInfo(int SyNr, char *SyId, int &Dim, int &Typ) {
        if (!SyNr) {
            SyId[0] = '*';
            SyId[1] = '\0';;
            Dim = 1;
            Typ = dt_set;
            return true;
        }

        if (NameList && !NameList->empty() && SyNr > 0 && SyNr <= NameList->size()) {
            const auto sym = *NameList->GetObject(SyNr);
            utils::assignStrToBuf(NameList->GetString(SyNr), SyId, GMS_SSSIZE);
            Dim = sym->SDim;
            Typ = sym->SDataType;
            return true;
        }

        SyId[0] = '\0';
        Dim = -1;
        Typ = dt_set;
        return false;
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
                                   utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>(DOMC_UNMAPPED).data(), fr_str_data);
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
        ReadPtr = std::nullopt;
        InitErrors();

        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::cout << "gdxOpenRead(" << Afn << ")" << std::endl;
        }

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
        FFile = std::make_unique<TMiBufferedStreamDelphi>(Afn, filemode);
        const std::string FileNameYML = utils::replaceSubstrs(Afn, ".gdx", ".yaml");
#ifdef YAML
        YFile = std::make_unique<yaml::TYAMLFile>(FileNameYML, writeAsYAML);
#endif
        ErrNr = FFile->GetLastIOResult();
        if(ErrNr) return FileNoGood();
        if(FFile->GoodByteOrder()) {
            ErrNr = ERR_BADDATAFORMAT;
            return FileNoGood();
        }
        if( ErrorCondition(FFile->ReadByte() == gdxHeaderNr, ERR_OPEN_FILEHEADER) ||
            ErrorCondition(utils::sameText(FFile->ReadString(), gdxHeaderId), ERR_OPEN_FILEMARKER)) return FileErrorNr();
        VersionRead = FFile->ReadInteger();
        if (ErrorCondition(VersionRead <= VERSION, ERR_OPEN_FILEVERSION)) return FileErrorNr();

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
        NameList = std::make_unique<TNameList>();
        NameList->OneBased = true;
        AcronymList.clear();
        FilterList.clear();
        const int NrElemsOfSym = NrElem;
        for(int N{1}; N<=NrElemsOfSym; N++) {
            std::string S {FFile->ReadString()};
            CurSyPtr = new TgdxSymbRecord {};
            CurSyPtr->SPosition = VersionRead <= 5 ? FFile->ReadInteger() : FFile->ReadInt64();
            CurSyPtr->SDim = FFile->ReadInteger();
            uint8_t B {FFile->ReadByte()};
            CurSyPtr->SDataType = static_cast<gdxSyType>( B );
            CurSyPtr->SUserInfo = FFile->ReadInteger();
            CurSyPtr->SDataCount = FFile->ReadInteger();
            CurSyPtr->SErrors = FFile->ReadInteger();
            B = FFile->ReadByte();
            CurSyPtr->SSetText = B;
            CurSyPtr->SExplTxt = FFile->ReadString();
            CurSyPtr->SIsCompressed = VersionRead > 5 && FFile->ReadByte();
            CurSyPtr->SDomSymbols = nullptr;
            if(VersionRead >= 7) {
                if(FFile->ReadByte()) {
                    CurSyPtr->SDomSymbols = std::make_unique<std::vector<int>>(CurSyPtr->SDim);
                    for(auto &SDomSym : *CurSyPtr->SDomSymbols)
                        SDomSym = FFile->ReadInteger();
                }
                NrElem = FFile->ReadInteger();
                if(NrElem) {
                    CurSyPtr->SCommentsList.emplace_back(FFile->ReadString());
                    NrElem--;
                }
            }
            CurSyPtr->SSetBitMap = nullptr;
            CurSyPtr->SDomStrings = nullptr;
            CurSyPtr->SSyNr = CurSyPtr->SSyNrActual = NameList->StoreObject(S, CurSyPtr);
        }

        // reading UEL table
        FFile->SetCompression(DoUncompress);
        FFile->SetPosition(UELPos);
        UELTable = std::make_unique<UELTableImplChoice>();

        if (ErrorCondition(FFile->ReadString() == MARK_UEL, ERR_OPEN_UELMARKER1))
            return FileErrorNr();

        NrElem = FFile->ReadInteger();
        //bug for pre 2002
        if (utils::substr(FileSystemID, 15, 4) == "2001") NrElem--;

        while (UELTable->size() < NrElem) {
            UELTable->StoreObject(FFile->ReadString(), -1);
        }
        UelCntOrig = UELTable->size(); // needed when reading universe

        if (ErrorCondition(FFile->ReadString() == MARK_UEL, ERR_OPEN_UELMARKER2)) return FileErrorNr();
        if (ReadMode % 2 == 0) { // reading text table
            FFile->SetCompression(DoUncompress);
            FFile->SetPosition(SetTextPos);
            SetTextList = std::make_unique<TSetTextList>();
            SetTextList->OneBased = false;
            if(ErrorCondition(FFile->ReadString() == MARK_SETT, ERR_OPEN_TEXTMARKER1)) return FileErrorNr();
            NrElem = FFile->ReadInteger();
            //SetTextList->reserve(NrElem);
            for (int N{}; N < NrElem; N++) {
                int TextNum{ SetTextList->AddObject(FFile->ReadString(), 0) };
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
            for (int i = 0; i < (int)AcronymList.size(); i++) {
                auto& obj = AcronymList[i];
                obj.AcrName = FFile->ReadString();
                obj.AcrText = FFile->ReadString();
                obj.AcrMap = FFile->ReadInteger();
            }
        }

        DomainStrList = std::make_unique<std::vector<std::string>>();
        if (VersionRead >= 7 && DomStrPos) {
            FFile->SetCompression(DoUncompress);
            FFile->SetPosition(DomStrPos);
            if(ErrorCondition(FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER1)) return FileErrorNr();
            DomainStrList->resize(FFile->ReadInteger());
            for (auto & i : *DomainStrList)
                i = FFile->ReadString();
            if (ErrorCondition(FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER2)) return FileErrorNr();
            while (true) {
                int SyNr = FFile->ReadInteger();
                if (SyNr <= 0) break;
                const auto sym = *NameList->GetObject(SyNr);
                sym->SDomStrings = std::make_unique<std::vector<int>>(sym->SDim);
                for (auto &SDomString : *sym->SDomStrings)
                    SDomString = FFile->ReadInteger();
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
        int SyNr1 { Id1 == "*" ? std::numeric_limits<int>::max() : NameList->IndexOf(Id1) };
        int SyNr2 { Id2 == "*" ? std::numeric_limits<int>::max() : NameList->IndexOf(Id2) };
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
        else if(ErrorCondition(utils::in((*NameList->GetObject(SyNr))->SDataType, dt_set, dt_alias), ERR_ALIASSETEXPECTED)) return false;
        if(!IsGoodNewSymbol(AName)) return false;
        auto SyPtr = new TgdxSymbRecord{};
        SyPtr->SDataType = dt_alias;
        SyPtr->SUserInfo = SyNr;
        if(!SyNr) {
            SyPtr->SDim = 1;
            SyPtr->SExplTxt = "Aliased with *"s;
        } else {
            SyPtr->SDim = (*NameList->GetObject(SyNr))->SDim;
            SyPtr->SExplTxt = "Aliased with "s + NameList->GetString(SyNr);
        }
        // TODO: Also the Delphi source does not set SSyNr here correctly, hence the helper field "SSyNrActual" is used
        SyPtr->SSyNrActual = NameList->AddObject(AName, SyPtr);
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
        if(!SetTextList || (TraceLevel >= TraceLevels::trl_all && !CheckMode("AddSetText")))
            return false;
        TxtNr = SetTextList->AddObject(MakeGoodExplText(Txt), 0);
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
        static const TgxModeSet AllowedModes{ fr_init,fw_init,fr_map_data, fr_mapr_data, fw_raw_data, fw_map_data,fw_str_data };
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
        if((TraceLevel >= TraceLevels::trl_all || fmode != fr_raw_data) && !CheckMode("DataReadRaw", fr_raw_data)) return false;
        if(!DoRead(Values, DimFrst)) gdxDataReadDone();
        else {
            //std::copy(LastElem.begin(), LastElem.begin()+FCurrentDim-1, KeyInt.begin());
            memcpy(KeyInt, LastElem.data(), FCurrentDim*sizeof(int));

            if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
                std::cout << "DataReadRaw index: ";
                for(int D{}; D<FCurrentDim; D++)
                    std::cout << std::to_string(KeyInt[D]) << (D+1 < FCurrentDim ? "," : "");
                std::cout << '\n';
            }

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
                                   utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>(DOMC_UNMAPPED).data(), fr_raw_data);
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
        if (fmode == fw_dom_raw) fmode = fw_raw_data;
        if ((TraceLevel >= TraceLevels::trl_some || fmode != fw_raw_data) && !CheckMode("DataWriteRaw", fw_raw_data)) return false;
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
    //    indx := Round(Vals[GMS_VAL_LEVEL]);
    //    if indx > 0
    //    then
    //       begin
    //       gdxGetElemText(indx, S, N);
    //       Write('txt = ', S, ' Node = ', N);
    //       end;
    //    WriteLn;
    //    end
    // </CODE>
    int TGXFileObj::gdxGetElemText(int TxtNr, char *Txt, int &Node) {
        Node = 0;
        if(!SetTextList) {
            Txt[0] = '\0';
            return false;
        }
        if(TraceLevel >= TraceLevels::trl_all && !CheckMode("GetElemText"))
            return false;
        if(TxtNr < 0 || TxtNr >= SetTextList->size()) {
            utils::assignStrToBuf(BADStr_PREFIX + std::to_string(TxtNr), Txt, GMS_SSSIZE);
            return false;
        } else {
            utils::assignStrToBuf(SetTextList->GetString(TxtNr), Txt, GMS_SSSIZE);
            Node = *SetTextList->GetObject(TxtNr);
            return true;
        }
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
    int TGXFileObj::gdxGetSpecialValues(double *AVals) {
        AVals[sv_valund] = intlValueMapDbl[vm_valund] ;
        AVals[sv_valna ] = intlValueMapDbl[vm_valna ] ;
        AVals[sv_valpin] = intlValueMapDbl[vm_valpin] ;
        AVals[sv_valmin] = intlValueMapDbl[vm_valmin] ;
        AVals[sv_valeps] = intlValueMapDbl[vm_valeps] ;
        AVals[sv_acronym] = Zvalacr;

        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::array svNames {"undef"s, "na"s, "posinf"s, "min"s, "eps"s};
            std::array svIndices {sv_valund, sv_valna, sv_valpin, sv_valmin, sv_valeps};
            for(int i=0; i<(int)svNames.size(); i++)
                std::cout << svNames[i] << "=" << AVals[svIndices[i]] << std::endl;
        }

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
    int TGXFileObj::gdxSetSpecialValues(const double *AVals) {
        TIntlValueMapDbl tmpDbl{ intlValueMapDbl };

        tmpDbl[vm_valund] = AVals[sv_valund];
        tmpDbl[vm_valna] = AVals[sv_valna];
        tmpDbl[vm_valpin] = AVals[sv_valpin];
        tmpDbl[vm_valmin] = AVals[sv_valmin];
        tmpDbl[vm_valeps] = AVals[sv_valeps];

        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::array svNames {"undef"s, "na"s, "posinf"s, "min"s, "eps"s};
            std::array svIndices {sv_valund, sv_valna, sv_valpin, sv_valmin, sv_valeps};
            for(int i=0; i<(int)svNames.size(); i++)
                std::cout << svNames[i] << "=" << AVals[svIndices[i]] << std::endl;
        }

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

        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::cout << "Read dump, readIntlValueMapDbl" << std::endl;
            std::array svNames {"undef"s, "na"s, "posinf"s, "min"s, "eps"s};
            std::array svIndices {sv_valund, sv_valna, sv_valpin, sv_valmin, sv_valeps};
            for(int i=0; i<(int)svNames.size(); i++)
                std::cout << svNames[i] << "=" << readIntlValueMapDbl[svIndices[i]] << std::endl;
        }

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
        if (ErrorCondition(SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX)) return false;
        PgdxSymbRecord SyPtr{ (*NameList->GetObject(SyNr)) };
        for (int D{}; D < SyPtr->SDim; D++)
            DomainSyNrs[D] = !SyPtr->SDomSymbols || (*SyPtr->SDomSymbols)[D] == 0 ? 0 : (*SyPtr->SDomSymbols)[D];
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
        if (ErrorCondition(!NameList->empty() && SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX)) return 0;
        PgdxSymbRecord SyPtr{ (*NameList->GetObject(SyNr)) };

        for(int D{}; D<SyPtr->SDim; D++) {
            DomainIDs[D][0] = '*';
            DomainIDs[D][1] = '\0';
        }

        int res{};

        if (SyPtr->SDomStrings) {
            for (int D{}; D<SyPtr->SDim; D++)
                if((*SyPtr->SDomStrings)[D])
                    utils::stocp((*DomainStrList)[(*SyPtr->SDomStrings)[D]-1], DomainIDs[D]);
            res = 2;
        }
        else if (!SyPtr->SDomSymbols)
            res = 1;
        else {
            for (int D{}; D < SyPtr->SDim; D++)
                if ((*SyPtr->SDomSymbols)[D])
                    utils::stocp(NameList->GetString((*SyPtr->SDomSymbols)[D]), DomainIDs[D]);
            res = 3;
        }

        if(verboseTrace && TraceLevel == TraceLevels::trl_all && utils::in(res, 2, 3)) {
            std::cout << "GetDomain SyNr=" << SyNr << std::endl;
            for (int D{}; D < SyPtr->SDim; D++) {
                if(res == 2)
                    std::cout << "SDomStrings[" << D << "]=" << (*SyPtr->SDomStrings)[D] << std::endl;
                else if(res == 3)
                    std::cout << "SDomSymbols[" << D << "]=" << (*SyPtr->SDomSymbols)[D] << std::endl;
                std::cout << "DomainIDs[" << D << "]=" << DomainIDs[D] << std::endl;
            }
        }

        return res;
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
        return !NameList || NameList->empty() || SyNr < 1 || SyNr > NameList->size() ? -1 : (*NameList->GetObject(SyNr))->SDim;
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
    int TGXFileObj::gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt) {
        if (!SyNr) {
            RecCnt = UelCntOrig;
            UserInfo = 0;
            utils::assignStrToBuf("Universe"s, ExplTxt, GMS_SSSIZE);
            return true;
        }
        else if (!NameList || NameList->empty() || SyNr < 1 || SyNr > NameList->size()) {
            RecCnt = UserInfo = 0;
            ExplTxt[0] = '\0';
            return false;
        }
        else {
            const auto obj = (*NameList->GetObject(SyNr));
            RecCnt = !obj->SDim ? 1 : obj->SDataCount; // scalar trick
            UserInfo = obj->SUserInfo;
            utils::assignStrToBuf(obj->SExplTxt, ExplTxt, GMS_SSSIZE);
            return true;
        }
    }

    // Summary:
    //   Define the domain of a symbol
    // Arguments:
    //   DomainIDs: array of identifiers or *
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
        int res{ false };
        static const TgxModeSet AllowedModes{ fw_dom_raw, fw_dom_map, fw_dom_str };
        if (!MajorCheckMode("SymbolSetDomain", AllowedModes) || !CurSyPtr) return res;

        if(verboseTrace && TraceLevel == TraceLevels::trl_all) {
            std::cout << "SetDomain" << std::endl;
            for(int D{}; D < CurSyPtr->SDim; D++)
                std::cout << "DomainID[" << D << "]=" << DomainIDs[D] << std::endl;
        }

        res = true;
        assert(!CurSyPtr->SDomSymbols && "SymbolSetDomain");
        CurSyPtr->SDomSymbols = std::make_unique<std::vector<int>>(CurSyPtr->SDim);
        for (int D{}; D < CurSyPtr->SDim; D++) {
            bool domap {true};
            int DomSy;
            if (!std::strcmp(DomainIDs[D], "*")) DomSy = 0;
            else {
                // Since SSyNr of alias symbol objects is 0
                // so symbol number of alias is deduced by helper field SSyNrActual
                DomSy = NameList->IndexOf(DomainIDs[D]);
                if (DomSy <= -1) {
                    ReportError(ERR_UNKNOWNDOMAIN);
                    DomSy = -1;
                    res = false;
                }
            }
            int SyNr;
            if (DomSy > 0) {
                SyNr = DomSy;
                do {
                    const auto *obj = (*NameList->GetObject(SyNr));
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
            (*CurSyPtr->SDomSymbols)[D] = DomSy;
            if (domap && DomSy > 0) {
                // this is the case for set i(i)
                if (CurSyPtr->SDim != 1 || CurSyPtr != *NameList->GetObject(DomSy)) {
                    auto thesym = (*NameList->GetObject(SyNr));
                    WrBitMaps[D] = thesym->SSetBitMap ? thesym->SSetBitMap.get() : nullptr;
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
        if (ErrorCondition(SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX)) return false;
        PgdxSymbRecord SyPtr = (*NameList->GetObject(SyNr));

        if(verboseTrace && TraceLevel == TraceLevels::trl_all) {
            std::cout << "SetDomainX SyNr=" << SyNr << std::endl;
            for(int D{}; D < SyPtr->SDim; D++)
                std::cout << "DomainID[" << D << "]=" << DomainIDs[D] << std::endl;
        }

        if (SyPtr->SDim > 0) {
            if (!SyPtr->SDomStrings)
                SyPtr->SDomStrings = std::make_unique<std::vector<int>>(SyPtr->SDim);
            for (int D{}; D < SyPtr->SDim; D++) {
                const std::string &S { DomainIDs[D] };
                if (S.empty() || S == "*" || !IsGoodIdent(S)) (*SyPtr->SDomStrings)[D] = 0;
                else {
                    (*SyPtr->SDomStrings)[D] = utils::indexOf<std::string>(*DomainStrList, [&](const auto &domStr) {
                        return utils::sameText(domStr, S);
                    }) + 1; // one based
                    if ((*SyPtr->SDomStrings)[D] <= 0) {
                        DomainStrList->push_back(S);
                        (*SyPtr->SDomStrings)[D] = (int) DomainStrList->size();
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
        UelCnt = UELTable ? (int)UELTable->size() : 0;
        SyCnt = NameList ? (int)NameList->size() : 0;
        return true;
    }

    // Brief:
    //   Finish registration of unique elements
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUELRegisterRawStart, gdxUELRegisterMapStart, gdxUELRegisterStrStart
    int TGXFileObj::gdxUELRegisterDone() {
        static const TgxModeSet AllowedModes{ f_raw_elem,f_map_elem,f_str_elem };
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
        if(verboseTrace && TraceLevel >= TraceLevels::trl_all)
            std::cout << "Uel=" << Uel << '\n';
        if ((TraceLevel >= TraceLevels::trl_all || fmode != f_raw_elem) && !CheckMode("UELRegisterRaw", f_raw_elem))
            return false;
        // AS: Directly use passed UEL if trimming is not needed (empty or last actual char is non-blank)
        auto optTrimmed = utils::maybeTrimRight(Uel);
        const std::string &SV = optTrimmed ? *optTrimmed : Uel;
        if (ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
        UELTable->AddObject(SV, -1); // should about existing mapping?
        return true;
    }

    // Brief:
    //   Start registering unique elements in raw mode
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxUELRegisterRaw, gdxUELRegisterDone
    int TGXFileObj::gdxUELRegisterRawStart() {
        if (!MajorCheckMode("UELRegisterRawStart", fw_init)) return false;
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
        if ((TraceLevel >= TraceLevels::trl_all || fmode != f_str_elem) && !CheckMode("UELRegisterStr", f_str_elem))
            return false;
        std::string SV{ utils::trimRight(Uel) };
        if (ErrorCondition(GoodUELString(SV), ERR_BADUELSTR)) return false;
        UelNr = UELTable->AddUsrNew(SV);
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
        static const TgxModeSet AllowedModes{ fr_init, fw_init };
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
    int TGXFileObj::gdxUMUelGet(int UelNr, char *Uel, int &UelMap) {
        if (UELTable && UelNr >= 1 && UelNr <= UELTable->size()) {
            utils::assignStrToBuf((*UELTable)[UelNr-1], Uel, GLOBAL_UEL_IDENT_SIZE);
            UelMap = UELTable->GetUserMap(UelNr);
            return true;
        }
        else {
            utils::assignStrToBuf(BADUEL_PREFIX + std::to_string(UelNr), Uel, GLOBAL_UEL_IDENT_SIZE);
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
            UelCnt = UELTable ? UELTable->size() : 0;
            HighMap = UELTable->UsrUel2Ent.GetHighestIndex(); // highest index
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
        if(!UELTable) return -1;
        std::string S{utils::trimRight(NewName)};
        if(!GoodUELString(S))
            return ERR_BADUELSTR;
        int N{UELTable->IndexOf(utils::trimRight(OldName))};
        if(N < 0)
            return 2;
        else if(UELTable->IndexOf(S) >= 0)
            return 3;
        UELTable->RenameEntry(N, S);
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
    int TGXFileObj::gdxGetUEL(int uelNr, char *Uel) {
        if(!UELTable) {
            Uel[0] = '\0';
            return false;
        }
        int EN = UELTable->UsrUel2Ent.GetMapping(uelNr);
        utils::assignStrToBuf(EN >= 1 ? (*UELTable)[EN-1] : BADUEL_PREFIX + std::to_string(uelNr), Uel, GLOBAL_UEL_IDENT_SIZE);
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
        SortList = std::make_unique<LinkedDataType>(FCurrentDim, static_cast<int>(DataSize * sizeof(double)));
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
        TIndex Keys;
        if(fmode == fw_dom_map)
            fmode = fw_map_data;
        if(TraceLevel >= TraceLevels::trl_all || fmode != fw_map_data) {
            if(!CheckMode("DataWriteMap", fw_map_data)) return false;
            std::cout << "   Index =";
            for(int D{}; D<FCurrentDim; D++) {
                std::cout << " " << std::to_string(KeyInt[D]);
                if(D+1 < FCurrentDim) std::cout << ",";
            }
        }
        for(int D{}; D<FCurrentDim; D++) {
            int KD = UELTable->UsrUel2Ent.GetMapping(KeyInt[D]);
            if(KD < 0) {
                ReportError(ERR_BADELEMENTINDEX);
                return false;
            }
            Keys[D] = KD;
            if(KD < MinElem[D]) MinElem[D] = KD;
            if(KD > MaxElem[D]) MaxElem[D] = KD;
        }
        SortList->AddItem(Keys.data(), Values);
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
        static const TgxModeSet AllowedModes {fr_init, fw_init};
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
        std::string SV {utils::trimRight(Uel)};
        if(TraceLevel >= TraceLevels::trl_all || fmode != f_map_elem) {
            if(!CheckMode("UELRegisterMap", f_map_elem)) return false;
            std::cout << "   Enter UEL: " << SV << " with number " << UMap << "\n";
        }
        if(ErrorCondition(GoodUELString(SV), ERR_BADUELSTR) ||
            ErrorCondition(UELTable->AddUsrIndxNew(SV, UMap) >= 0, ERR_UELCONFLICT)) return false;
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
        auto XDomains = utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>(DOMC_STRICT);
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
        static const TgxModeSet AllowedModes{fr_map_data, fr_mapr_data};
        if((TraceLevel >= TraceLevels::trl_all || !utils::in(fmode, AllowedModes)) && !CheckMode("DataReadMap", AllowedModes)) return false;
        if(CurSyPtr && CurSyPtr->SScalarFrst) {
            CurSyPtr->SScalarFrst = false;
            GetDefaultRecord(Values);
            DimFrst = 0;
            return true;
        }
        if(fmode == fr_map_data) {
            DimFrst = 0;
#ifdef TLD_LEGACY
            if (!ReadPtr || !SortList->GetNextRecord(&*ReadPtr, KeyInt, Values)) return false;
#else
            if (!ReadPtr || !SortList->GetNextRecord(/*&*/*ReadPtr, KeyInt, Values)) return false;
#endif
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
                        V = UELTable->GetUserMap(LastElem[D]);
                        if(obj.DFilter->InFilter(V)) KeyInt[D] = V;
                        else {
                            AddError = true;
                            FIDim = D+1;
                            loopDone = true;
                        }
                        break;
                    case dm_strict:
                        V = UELTable->GetUserMap(LastElem[D]);
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
                            V = UELTable->GetUserMap(EN);
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
                        V = UELTable->GetUserMap(LastElem[D]);
                        if(!DomainList[D].DFilter->InFilter(V))
                            LastElem[D] = -LastElem[D];
                        break;
                    case dm_strict:
                        V = UELTable->GetUserMap(LastElem[D]);
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
                    int V = UELTable->NewUsrUel(-EN);
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
        return static_cast<int>(AcronymList.size());
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
    int TGXFileObj::gdxAcronymGetInfo(int N, char *AName, char *Txt, int &AIndx) const {
        if(N < 1 || N > (int)AcronymList.size()) {
            AName[0] = Txt[0] = '\0';
            AIndx = 0;
            return false;
        }
        auto acr = AcronymList[N-1];
        utils::assignStrToBuf(acr.AcrName, AName, GMS_SSSIZE);
        utils::assignStrToBuf(acr.AcrText, Txt, GMS_SSSIZE);
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

        if(ErrorCondition(N >= 1 || N <= (int)AcronymList.size(), ERR_BADACRONUMBER)) return false;
        auto &obj = AcronymList[N-1];
        if(utils::in(fmode, AnyWriteMode) || obj.AcrAutoGen) {
            if(ErrorCondition(IsGoodNewSymbol(AName), ERR_BADACRONAME)) return false;
            if(obj.AcrAutoGen) {
                assert(obj.AcrReadMap == AIndx && "gdxAcronymSetInfo");
                obj.AcrAutoGen = false;
            }
            else if(ErrorCondition(AIndx == obj.AcrMap, ERR_BADACROINDEX)) return false;

            obj.AcrName = AName;
            obj.AcrText = MakeGoodExplText(Txt);

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
        if(ErrorCondition(N >= 1 || N <= (int)AcronymList.size(), ERR_BADACRONUMBER)) return false;
        const auto &obj = AcronymList[N-1];
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
    //   can later be used to specify a filter for an index position when reading data.
    int TGXFileObj::gdxFilterRegisterStart(int FilterNr) {
        if(!MajorCheckMode("FilterRegisterStart"s, fr_init) ||
            ErrorCondition(FilterNr >= 1, ERR_BAD_FILTER_NR)) return false;

        FilterList.AddFilter(TDFilter{ FilterNr, UELTable->UsrUel2Ent.GetHighestIndex() });
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
        if((TraceLevel >= TraceLevels::trl_all || fmode != fr_filter) &&
            !CheckMode("FilterRegister"s, fr_filter)) return false;
        auto &obj = *CurFilter;
        if(ErrorCondition(UelMap >= 1 && UelMap <= obj.FiltMaxUel, ERR_BAD_FILTER_INDX)) return false;
        int EN{UELTable->UsrUel2Ent.GetMapping(UelMap)};
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
        if(!MajorCheckMode("FilterRegisterDone"s, fr_filter)) return false;
        fmode = fr_init;
        CurFilter->FiltSorted = true;
        if(UELTable && UELTable->GetMapToUserStatus() == map_unsorted) {
            int LV {-1};
            for(int N{1}; N<=UELTable->size(); N++) {
                int V{UELTable->GetUserMap(N)};
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

    // Brief:
    //   Set the Node number for an entry in the string table
    // Arguments:
    //   TxtNr: Index number of the entry to be modified
    //   Node: The new Node value for the entry
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //  gdxAddSetText, gdxGetElemText
    // Description:
    //  After registering a string with AddSetText, we can assign
    //  a node number for later retrieval. The node number is any
    //  integer which is stored without further restrictions.
    int TGXFileObj::gdxSetTextNodeNr(int TxtNr, int Node)
    {
        if (!SetTextList || (TraceLevel >= TraceLevels::trl_all && !CheckMode("SetTextNodeNr"))) return false;
        auto& obj = *SetTextList;
        if (TxtNr >= 0 && TxtNr < obj.size() && !*obj.GetObject(TxtNr)) {
            *obj.GetObject(TxtNr) = Node;
            return true;
        }
        return false;
    }

    // Brief:
    //   Get the unique elements for a given dimension of a given symbol
    // Arguments:
    //   SyNr: The index number of the symbol, range 1..NrSymbols
    //   DimPos:  The dimension to use, range 1..dim
    //   FilterNr: Number of a previously registered filter or the value DOMC_EXPAND if no filter is wanted
    //   DP: Callback procedure which will be called once for each available element (can be nil)
    //   NrElem: Number of unique elements found
    //   UPtr: User pointer; will be passed to the callback procedure
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Description:
    //   Using the data of a symbol, get the unique elements for a given index position. To achieve this,
    //   the symbols data is read and a tally is kept for the elements in the given index position. When a filter
    //   is specified, records that have elements in the specified index position that are outside the filter will
    //   be added to the list of DataErrorRecords. See gdxDataErrorRecord
    // See Also:
    //   gdxDataErrorCount gdxDataErrorRecord
    // Example:
    // <CODE>
    //   var
    //      T0 : Cardinal;
    //      Cnt: integer;
    //
    //   procedure DataDomainCB(RawNr, MappedNr: integer; UPtr: pointer); stdcall;
    //   begin
    //   Write(RawNr, ' (', MappedNr, ')');
    //   end;
    //
    //   T0 := GetTickCount();
    //   gdxGetDomainElements(PGX, 1, 1, DOMC_EXPAND, nil, cnt);
    //   WriteLn('Domain count only = ',cnt ,' ', GetTickCount - T0, ' ms');
    //   T0 := GetTickCount();
    //   gdxGetDomainElements(PGX, 1, 1, DOMC_EXPAND, DataDomainCB, cnt);
    //   WriteLn('Get domain count = ',cnt ,' ', GetTickCount - T0, ' ms');
    //   T0 := GetTickCount();
    //   gdxGetDomainElements(PGX, 1, 1, 7, DataDomainCB, cnt);
    //   WriteLn('Using filter 7; number of records in error list = ', gdxDataErrorCount(PGX) );
    // </CODE>
    int TGXFileObj::gdxGetDomainElements(int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int& NrElem, void* UPtr)
    {
        if (ErrorCondition(SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX)) return false;
        int Dim{ (*NameList->GetObject(SyNr))->SDim };
        if (!Dim || ErrorCondition(DimPos >= 1 && DimPos <= Dim, ERR_BADDIMENSION)) return false;
        TDFilter *DFilter = FilterNr == DOMC_EXPAND ? nullptr : FilterList.FindFilter(FilterNr);
        if (FilterNr != DOMC_EXPAND && !DFilter) {
            ReportError(ERR_UNKNOWNFILTER);
            return false;
        }

        TIntegerMapping DomainIndxs;

        //-- Note: PrepareSymbolRead checks for the correct status
        TIndex XDomains = utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>(DOMC_UNMAPPED);

        // Following call also clears ErrorList
        PrepareSymbolRead("gdxGetDomain"s, SyNr, XDomains.data(), fr_raw_data);
        int AFDim;
        std::array<double, GMS_VAL_SCALE + 1> AVals;
        while (DoRead(AVals.data(), AFDim)) {
            int RawNr{ LastElem[DimPos-1] };
            if (DFilter) {
                int MapNr{ UELTable->GetUserMap(RawNr) };
                if (!DFilter->InFilter(MapNr)) {
                    //Register this record as a domain error (negative value indicates domain violation)
                    LastElem[DimPos - 1] = -LastElem[DimPos - 1];
                    AddToErrorListDomErrs(LastElem, AVals.data());    //unmapped
                    LastElem[DimPos - 1] = -LastElem[DimPos - 1];
                    //do not mark this element
                    continue;
                }
            }
            DomainIndxs.SetMapping(RawNr, 1);
        }
        gdxDataReadDone();
        NrElem = 0;
        TIndex Index;
        if (!DP) { // we only count
            for (int N{ 1 }; N <= DomainIndxs.GetHighestIndex(); N++)
                // N-1?
                if (DomainIndxs[N] == 1) NrElem++;
        }
        else { //should we have an option to return indices in Raw order or in Mapped order?
            gdlib::gmsdata::TTblGamsData SortL;
            std::array<double, 5> vf{};
            for (int N{ 1 }; N <= DomainIndxs.GetHighestIndex(); N++) {
                if (DomainIndxs[N] == 1) {
                    NrElem++;
                    Index.front() = UELTable->NewUsrUel(N);
                    vf.front() = N;
                    SortL[Index] = vf;
                }
            }
            SortL.sort();
            for (int N{}; N < SortL.size(); N++) {
                SortL.GetRecord(N, Index.data(), (int)Index.size(), vf.data());
                DP(static_cast<int>(vf.front()), Index.front(), UPtr);
            }
        }
        return NrElem >= 0;
    }

    // Brief:
    //   Set the amount of trace (debug) information generated
    // Arguments:
    //   N: Tracing level  N <= 0 no tracing  N >= 3 maximum tracing
    //   S: A string to be included in the trace output
    // Returns:
    //   Always non-zero
    // Description:
    //
    int TGXFileObj::gdxSetTraceLevel(int N, const std::string &s) {
        if(N <= 0) TraceLevel = TraceLevels::trl_none;
        else {
            switch(N) {
                case 1: TraceLevel = TraceLevels::trl_errors; break;
                case 2: TraceLevel = TraceLevels::trl_some; break;
                default: TraceLevel = TraceLevels::trl_all; break;
            }
            TraceStr = s;
        }
        //!! GetStdHandle(STD_OUTPUT_HANDLE) <> INVALID_HANDLE_VALUE;
        if(TraceLevel > TraceLevels::trl_errors) {
            std::cout << std::endl;
            WriteTrace("Tracing at level "s + std::to_string((int)TraceLevel));
        }
        return true;
    }

    // Summary:
    //   Add a new acronym entry
    // Arguments:
    //   AName: Name of the acronym
    //   Txt: Explanatory text of the acronym
    //   AIndx:  Index value of the acronym
    // Return Value:
    //    0 If the entry is not added because of a duplicate name using the same value fo the indx
    //   -1 If the entry is not added because of a duplicate name using a different value for the indx
    //   Otherwise the index into the acronym table (1..gdxAcronymCount)
    // See Also:
    //   gdxAcronymGetInfo, gdxAcronymCount
    // Description:
    //   This function can be used to add entries before data is written. When entries
    //   are added implicitly use gdxAcronymSetInfo to update the table.
    //
    int TGXFileObj::gdxAcronymAdd(const std::string &AName, const std::string &Txt, int AIndx) {
        for(int N{}; N<(int)AcronymList.size(); N++) {
            auto &obj = AcronymList[N];
            if(utils::sameText(obj.AcrName, AName)) {
                if(ErrorCondition(obj.AcrMap == AIndx, ERR_ACROBADADDITION)) return -1;
                return N;
            }
            if(ErrorCondition(obj.AcrMap != AIndx, ERR_ACROBADADDITION)) return -1;
        }
        int res{AcronymList.AddEntry(AName, Txt, AIndx)};
        AcronymList[res].AcrReadMap = AIndx;
        res++;  // one based for the user
        return res;
    }

    // Summary:
    //   Get index value of an acronym
    // Arguments:
    //   V: Input value possibly representing an acronym
    // Returns:
    //   Index of acronym value V; zero if V does not represent an acronym
    // See Also:
    //    gdxAcronymValue
    int TGXFileObj::gdxAcronymIndex(double V) const {
        return V < Zvalacr ? 0 : static_cast<int>(std::round(V / Zvalacr));
    }

    // Summary:
    //   Find the name of an acronym value
    // Arguments:
    //   V: Input value possibly containing an acronym
    //   AName: Name of acronym value or the empty string
    // Returns:
    //   Return non-zero if a name for the acronym is defined. Return
    //   zero if V does not represent an acronym value or a name
    //   is not defined. An unnamed acronym value will return a string
    //   of the form UnknownAcronymNNN; were NNN is the index of the acronym.
    // See Also:
    //    gdxAcronymIndex
    int TGXFileObj::gdxAcronymName(double V, char *AName) {
        int Indx {gdxAcronymIndex(V)};
        //not an acronym
        if(Indx <= 0) AName[0] = '\0';
        else {
            int N {AcronymList.FindEntry(Indx)};
            utils::assignStrToBuf(N < 0 ? "UnknownAcronym"s + std::to_string(Indx) : AcronymList[N].AcrName, AName, GMS_SSSIZE);
            return true;
        }
        return false;
    }

    // Summary:
    //   Create an acronym value based on the index
    // Arguments:
    //   AIndx: Index value; should be greater than zero
    // Returns:
    //   The calculated acronym value; zero if Indx is not positive
    // See Also:
    //    gdxAcronymIndex
    double TGXFileObj::gdxAcronymValue(int AIndx) const {
        return AIndx <= 0 ? 0.0 : Zvalacr * AIndx;
    }

    // Summary:
    //   Returns the value of the AutoConvert variable and sets the variable to nv
    // Arguments:
    //    nv: New value for AutoConvert
    // Return Value:
    //    Previous value of AutoConvert
    // Description:
    //    When we close a new gdx file, we look at the value of AutoConvert; if AutoConvert
    //    is non-zero, we look at the GDXCOMPRESS and GDXCONVERT environment variables to determine if
    //    conversion to an older file format is desired. We needed this logic so gdxcopy.exe
    //    can disable automatic file conversion.
    int TGXFileObj::gdxAutoConvert(int nv) {
        int res{AutoConvert};
        AutoConvert = nv;
        return res;
    }

    // Summary:
    //   Returns a version descriptor of the library
    // Arguments:
    //    V: Contains version string after return
    // Returns:
    //   Always returns non-zero
    // Description:
    //
    // See Also:
    int TGXFileObj::gdxGetDLLVersion(char *V) const {
        utils::assignStrToBuf(auditLine, V, GMS_SSSIZE);
        return true;
    }

    // Summary:
    //   Returns file format number and compression level used
    // Arguments:
    //    FileVer: File format number or zero if the file is not open
    //    ComprLev: Compression used; 0= no compression, 1=zlib
    // Returns:
    //   Always returns non-zero
    // Description:
    //
    // See Also:
    int TGXFileObj::gdxFileInfo(int &FileVer, int &ComprLev) const {
        switch(fstatus) {
            case stat_notopen:
                FileVer = ComprLev = 0;
                break;
            case stat_read:
                FileVer = VersionRead;
                ComprLev = fComprLev;
                break;
            case stat_write:
                FileVer = VERSION;
                ComprLev = fComprLev;
                break;
        }
        return true;
    }

    // Brief:
    //   Prepare for the reading of a slice of data from a data set
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Arguments:
    //   SyNr: Symbol number to read, range 1..NrSymbols
    //   ElemCounts: Array of integers, each position indicating the number of
    //             unique indices in that position
    // See Also:
    //   gdxDataReadSlice, gdxDataReadDone
    // Description:
    //   Prepare for the reading of a slice of data. The actual read of the data
    //   is done by calling gdxDataReadSlice. When finished reading, call gdxDataReadDone.
    int TGXFileObj::gdxDataReadSliceStart(int SyNr, int* ElemCounts)
    {
        //-- Note: PrepareSymbolRead checks for the correct status
        TgdxUELIndex XDomains = utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>(DOMC_UNMAPPED);
        SliceSyNr = SyNr;
        PrepareSymbolRead("DataReadSliceStart"s, SliceSyNr, XDomains.data(), fr_raw_data);

        memset(ElemCounts, 0, sizeof(int) * GLOBAL_MAX_INDEX_DIM);

        TgdxValues Values;
        int FDim;
        while (DoRead(Values.data(), FDim))
            for (int D{ 1 }; D <= FCurrentDim; D++)
                SliceIndxs[D].SetMapping(LastElem[D], 1);

        gdxDataReadDone();

        for (int D{ 1 }; D <= FCurrentDim; D++) {
            auto& obj = SliceIndxs[D];
            int Cnt{};
            for (int N{}; N <= obj.GetHighestIndex(); N++) {
                if (obj.GetMapping(N) >= 0) {
                    obj.SetMapping(N, Cnt); // we keep it zero based
                    SliceRevMap[D].SetMapping(Cnt, N);
                    Cnt++;
                }
            }
            ElemCounts[D] = Cnt;
        }
        fmode = fr_slice;
        return true;
    }

    // Brief:
    //   Read a slice of data from a data set
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Arguments:
    //   UelFilterStr: Each index can be fixed by setting the string for the unique
    //                  element. Set an index position to the empty string in order
    //                  not to fix that position.
    //   Dimen: The dimension of the index space; this is the number of index positions
    //         that is not fixed.
    //   DP: Callback procedure which will be called for each available data item
    // See Also:
    //   gdxDataReadSliceStart, gdxDataSliceUELS, gdxDataReadDone
    // Description:
    //   Read a slice of data, by fixing zero or more index positions in the data.
    //   When a data element is available, the callback procedure DP is called with the
    //   current index and the values. The indices used in the index vary from zero to
    //   the highest value minus one for that index position. This function can be called
    //   multiple times.
    int TGXFileObj::gdxDataReadSlice(const char** UelFilterStr, int& Dimen, TDataStoreProc_t DP) {
        if (!MajorCheckMode("DataReadSlice", fr_slice))
            return false;
        bool GoodIndx {true};
        Dimen = 0;
        TgdxUELIndex ElemNrs;
        for (int D{ 1 }; D <= FCurrentDim; D++) {
            SliceElems[D] = UelFilterStr[D];
            if (!strlen(UelFilterStr[D])) {
                ElemNrs[D] = -1;
                Dimen++;
            }
            else {
                ElemNrs[D] = UELTable->IndexOf(UelFilterStr[D]);
                if (ElemNrs[D] < 0) GoodIndx = false;
            }
        }
        fmode = fr_init;
        if (!GoodIndx) return false;
        TgdxUELIndex XDomains = utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>(DOMC_UNMAPPED);
        PrepareSymbolRead("DataReadSlice"s, SliceSyNr, XDomains.data(), fr_slice);
        TgdxValues Values;
        TgdxUELIndex HisIndx;
        int FDim;
        while (DoRead(Values.data(), FDim)) {
            GoodIndx = true;
            int HisDim = 0;
            for (int D{ 1 }; D <= FCurrentDim; D++) {
                if (ElemNrs[D] == -1) {
                    HisDim++;
                    HisIndx[HisDim] = SliceIndxs[D].GetMapping(LastElem[D]);
                }
                else if(ElemNrs[D] != LastElem[D])
                    GoodIndx = false;
            }
            if (GoodIndx) DP(HisIndx.data(), Values.data());
        }
        return true;
    }

    // Brief:
    //   Map a slice index in to the corresponding unique elements
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Arguments:
    //   SliceKeyInt: The slice index to be mapped to strings.
    //   KeyStr: Array of strings containg the unique elements
    // See Also:
    //   gdxDataReadSliceStart, gdxDataReadDone
    // Description:
    //   After calling DataReadSliceStart, each index position is mapped from 0 to N(d)-1.
    //   This function maps this index space back in to unique elements represented as
    //   strings.
    int TGXFileObj::gdxDataSliceUELS(const int* SliceKeyInt, char** KeyStr) {
        if (!MajorCheckMode("DataSliceUELS"s, fr_slice)) return false;
        int HisDim{};
        for (int D{ 1 }; D <= FCurrentDim; D++) {
            if (!SliceElems[D].empty())
                utils::assignStrToBuf(SliceElems[D], KeyStr[D]);
            else {
                HisDim++;
                int N = SliceRevMap[D].GetMapping(SliceKeyInt[HisDim]);
                if (N < 0) {
                    KeyStr[D][0] = '?';
                    KeyStr[D][1] = '\0';
                }
                else utils::assignStrToBuf((*UELTable)[N], KeyStr[D], GLOBAL_UEL_IDENT_SIZE);
            }
        }
        return true;
    }

    // Brief:
    //   Return the number of bytes used by the data objects
    // Arguments:
    //   None
    // Returns:
    //   The number of bytes used by the data objects
    int64_t TGXFileObj::gdxGetMemoryUsed() {
        // TODO: Actually return something meaningful!
        // ...
        return 0;
    }

    // Brief:
    //   Classify a value as a potential special value
    // Arguments:
    //   D: Value to classify
    //   sv: Classification
    // Returns:
    //   Returns non-zero if D is a special value, zero otherwise
    // See Also:
    //  gdxGetSpecialValues, gdxSetSpecialValues
    // Description:
    //
    int TGXFileObj::gdxMapValue(double D, int& sv)
    {
        int64_t i64 = dblToI64(D);
        if (i64 == intlValueMapI64[vm_valund]) sv = sv_valund;
        else if (i64 == intlValueMapI64[vm_valna]) sv = sv_valna;
        else if (i64 == intlValueMapI64[vm_valpin]) sv = sv_valpin;
        else if (i64 == intlValueMapI64[vm_valmin]) sv = sv_valmin;
        else if (i64 == intlValueMapI64[vm_valeps]) sv = sv_valeps;
        else {
            sv = sv_normal;
            return false;
        }
        return true;
    }

    // Brief:
    //   Open an existing gdx file for output
    // Arguments:
    //   FileName:  File name of the gdx file to be created
    //   Producer:  Name of program that appends to the gdx file
    //   ErrNr: Returns an error code or zero if there is no error
    // Returns:
    //   Returns non-zero if the file can be opened; zero otherwise
    // See Also:
    //   gdxOpenRead, gdxOpenWrite, gdxOpenWriteEx
    // Description:
    //   Open an existing gdx file for output. If a file extension is not
    //   supplied, the extension '.gdx' will be used. The return code is
    //   a system dependent I/O error.
    //   When appending to a gdx file, the symbol table, uel table etc will be read
    //   and the whole setup will be treated as if all symbols were just written to
    //   the gdx file. Replacing a symbol is not allowed; it will generate a duplicate
    //   symbol error.
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
    //   gdxOpenAppend(PGX,'c:\\mydata\\file1.gdx','Examples', ErrCode);
    //   if ErrCode <> 0
    //   then
    //      [ ... ]
    // </CODE>
    int TGXFileObj::gdxOpenAppend(const std::string& FileName, const std::string& Producer, int& ErrNr) {
        FProducer2 = Producer;
        AppendActive = true;
        int res{ gdxOpenReadXX(FileName, fmOpenReadWrite, 0, ErrNr) };
        if (!res || ErrNr != 0) return res;
        if (VersionRead < 7) {
            ReportError(ERR_FILETOOLDFORAPPEND);
            gdxClose();
            return res;
        }
        fmode = fw_init;
        fstatus = stat_write;
        FFile->SetPosition(NextWritePosition);
        CompressOut = DoUncompress;
        return res;
    }

    // Brief:
    //   Test if any of the elements of the set has an associated text
    // Arguments:
    //   SyNr: Set Symbol number (1..NrSymbols)
    // Returns:
    //   Non-zero if the Set contains at least one element that has associated text,
    //     zero otherwise
    // See Also:
    //   gdxSystemInfo, gdxSymbolInfo
    // Description:
    //
    int TGXFileObj::gdxSetHasText(int SyNr) {
        return NameList && !NameList->empty() && SyNr >= 1 && SyNr <= NameList->size() && (*NameList->GetObject(SyNr))->SSetText;
    }

    // Brief:
    //   Set the internal values for special values when reading a gdx file
    // Arguments:
    //   AVals: array of special values to be used for Eps, +Inf, -Inf, NA and Undef
    //          Note that the values do not have to be unique
    // Returns:
    //   Always non-zero
    // Note: Before calling this function, initialize the array of special values
    //   by calling gdxGetSpecialValues first
    // See Also:
    //  gdxSetSpecialValues, gdxResetSpecialValues, gdxGetSpecialValues
    // Description:
    //
    int TGXFileObj::gdxSetReadSpecialValues(const double *AVals) {
        readIntlValueMapDbl[vm_valund] = AVals[sv_valund];
        readIntlValueMapDbl[vm_valna] = AVals[sv_valna];
        readIntlValueMapDbl[vm_valpin] = AVals[sv_valpin];
        readIntlValueMapDbl[vm_valmin] = AVals[sv_valmin];
        readIntlValueMapDbl[vm_valeps] = AVals[sv_valeps];

        if(verboseTrace && TraceLevel >= TraceLevels::trl_all) {
            std::cout << "gdxSetReadSpecialValues, dump of readIntlValueMapDbl" << std::endl;
            std::array svNames {"undef"s, "na"s, "posinf"s, "min"s, "eps"s};
            std::array svIndices {sv_valund, sv_valna, sv_valpin, sv_valmin, sv_valeps};
            for(int i=0; i<(int)svNames.size(); i++)
                std::cout << svNames[i] << "=" << readIntlValueMapDbl[svIndices[i]] << std::endl;
        }

        return true;
    }

    // Summary:
    //   Returns the length of the longest UEL used for every index position for a given symbol
    // Arguments:
    //   SyNr: Symbol number
    //   LengthInfo: The longest length for each index position
    // Returns:
    //   The length of the longest UEL found in the data
    // See also:
    //   gdxUELMaxLength
    int TGXFileObj::gdxSymbIndxMaxLength(int SyNr, int* LengthInfo) {
        memset(LengthInfo, 0, GLOBAL_MAX_INDEX_DIM * sizeof(int));

        int NrRecs;
        if (((TraceLevel >= TraceLevels::trl_some || fmode != fr_init)
            && !CheckMode("SymbIndxMaxLength", fr_init))
            || (SyNr < 0 || SyNr > NameList->size()) || !gdxDataReadRawStart(SyNr, NrRecs))
            return 0;

        int res{};
        if (FCurrentDim > 0) {
            int UELTableCount = UELTable ? UELTable->size() : 0; // local copy for speed
            TgdxValues AVals;
            int AFDim;
            while (DoRead(AVals.data(), AFDim)) {
                for (int D{}; D < FCurrentDim; D++) {
                    int UEL = LastElem[D];
                    if (UEL >= 1 && UEL <= UELTableCount) {
                        auto L = static_cast<int>((*UELTable)[UEL-1].length());
                        if (L > LengthInfo[D]) LengthInfo[D] = L;
                    }
                }
            }
            for (int D{}; D < FCurrentDim; D++)
                if (LengthInfo[D] > res)
                    res = LengthInfo[D];
        }
        gdxDataReadDone();
        return res;
    }

    // Summary:
    //   Returns the length of the longest symbol name
    // Arguments:
    // Returns:
    //   The length of the longest symbol name
    int TGXFileObj::gdxSymbMaxLength() {
        int acc {};
        for(int N{1}; N<=NameList->Count(); N++)
            acc = std::max<int>(acc, (int)NameList->GetString(N).length());
        return acc;
    }

    // Summary:
    //   Add a line of comment text for a symbol
    // Arguments:
    //   SyNr: The symbol number (range 1..NrSymbols); if SyNr <= 0 the current symbol being written
    //   Txt: String to add
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxSymbolGetComment
    int TGXFileObj::gdxSymbolAddComment(int SyNr, const std::string& Txt)
    {
        if (!MajorCheckMode("SymbolAddComment"s, AnyWriteMode)) return false;
        PgdxSymbRecord SyPtr;
        if (SyNr <= 0) SyPtr = CurSyPtr;
        else SyPtr = NameList && !NameList->empty() && SyNr <= NameList->size() ? *NameList->GetObject(SyNr) : nullptr;
        if (!SyPtr) {
            ReportError(ERR_NOSYMBOLFORCOMMENT);
            return false;
        }
        SyPtr->SCommentsList.push_back(Txt);
        return true;
    }

    // Summary:
    //   Retrieve a line of comment text for a symbol
    // Arguments:
    //   SyNr: The symbol number (range 1..NrSymbols)
    //   N: Line number (1..Count)
    //   Txt: String containing the line requested
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxSymbolAddComment
    int TGXFileObj::gdxSymbolGetComment(int SyNr, int N, char *Txt) {
        if (NameList && !NameList->empty() && SyNr >= 1 && SyNr <= NameList->size()) {
            const auto obj = *NameList->GetObject(SyNr);
            if (!obj->SCommentsList.empty() && N >= 1 && N <= (int)obj->SCommentsList.size()) {
                utils::assignStrToBuf(obj->SCommentsList[N - 1], Txt, GMS_SSSIZE);
                return true;
            }
        }
        Txt[0] = '\0';
        return false;
    }

    // Summary:
    //   Returns the length of the longest UEL name
    // Arguments:
    // Returns:
    //   The length of the longest UEL name
    // See also:
    //   gdxSymbIndxMaxLength
    int TGXFileObj::gdxUELMaxLength() {
        return UELTable->GetMaxUELLength();
    }

    // Brief:
    //   Search for unique element by its string
    // Arguments:
    //   Uel: String to be searched
    //   UelNr: Internal unique element number or -1 if not found
    //   UelMap: User mapping for the element or -1 if not found or
    //         the element was never mapped
    // Returns:
    //   Non-zero if the element was found, zero otherwise
    int TGXFileObj::gdxUMFindUEL(const std::string& Uel, int& UelNr, int& UelMap) {
        UelMap = -1;
        if (!UELTable) {
            UelNr = -1;
            return false;
        }
        UelNr = UELTable->IndexOf(utils::trimRight(Uel));
        if (UelNr < 0) return false;
        UelMap = UELTable->GetUserMap(UelNr);
        return true;
    }

    int TGXFileObj::gdxStoreDomainSets() {
        return StoreDomainSets;
    }

    void TGXFileObj::gdxStoreDomainSetsSet(int x) {
        StoreDomainSets = x;
    }

    // Brief:
    //   Read a symbol in Raw mode while applying a filter using a callback procedure
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // Arguments:
    //   UelFilterStr: Each index can be fixed by setting the string for the unique
    //                  element. Set an index position to the empty string in order
    //                  not to fix that position.
    //   DP: Callback procedure which will be called for each available data item
    // See Also:
    //   gdxDataReadRawFast, gdxDataReadSliceStart, gdxDataSliceUELS, gdxDataReadDone,
    // Description:
    //   Read a slice of data, by fixing zero or more index positions in the data.
    //   When a data element is available, the callback procedure DP is called with the
    //   current index (as raw numbers) and the values.
    // Example:
    // <CODE>
    // function DPCallBack(const Indx: TgdxUELIndex; const Vals: TgdxValues; Uptr: Pointer): integer; stdcall;
    // var
    //    s: ShortString;
    //    UelMap: integer;
    // begin
    // Result := 1;
    // gdxUMUelGet(Uptr, Indx[2], s, UelMap);
    // WriteLn(s, ' ', Vals[GMS_VAL_LEVEL]);
    // end;
    //
    // var
    //    pgx  : PGXFile;
    //    Msg  : ShortString;
    //    ErrNr: integer;
    //    IndxS: TgdxStrIndex;
    //
    // IndxS[1] := 'i200'; IndxS[2] := '';
    // gdxDataReadRawFastFilt(pgx, 1, IndxS, DPCallBack);
    // </CODE>
    int TGXFileObj::gdxDataReadRawFastFilt(int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP) {
        bool res{};
        // -- Note: PrepareSymbolRead checks for the correct status
        int NrRecs { PrepareSymbolRead("gdxDataReadRawFastFilt", SyNr,
                                       utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>(DOMC_UNMAPPED).data(),
                                       fr_raw_data) };
        if(NrRecs >= 0) {
            bool GoodIndx = true;
            int FiltDim {};
            TgdxUELIndex ElemDim, ElemNrs;
            for(int D{}; D<FCurrentDim; D++) {
                if(strlen(UelFilterStr[D])) {
                    FiltDim++;
                    ElemDim[FiltDim] = D+1;
                    ElemNrs[FiltDim] = UELTable->IndexOf(UelFilterStr[D]);
                    if(ElemNrs[FiltDim] < 0) GoodIndx = false;
                }
            }
            if(GoodIndx) {
                TgdxValues Values;
                int AFDim;
                while(DoRead(Values.data(), AFDim)) {
                    GoodIndx = true;
                    for(int D{1}; D<=FiltDim; D++) {
                        if(LastElem[ElemDim[D]] != ElemNrs[D]) {
                            GoodIndx = false;
                            break;
                        }
                    }
                    // FIXME: Actual call here needed?
                    if(GoodIndx && false/* && !gdxDataReadRawFastFilt_DP_FC(LastElem, Values, this)*/) {
                        break;
                    }
                }
                res = true;
            }
        }
        gdxDataReadDone();
        return res;
    }

    // Brief:
    //   Read a symbol in Raw mode using a callback procedure
    // Arguments:
    //   SyNr: The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
    //   DP: Procedure that will be called for each data record
    //   NrRecs: The number of records available for reading
    // Returns:
    //   Non-zero if the operation is possible, zero otherwise
    // See Also:
    //   gdxDataReadRaw, gdxDataReadMapStart, gdxDataReadStrStart, gdxDataReadDone, gdxDataReadRawFastFilt
    // Description:
    //   Use a callback function to read a symbol in raw mode. Using a callback procedure
    //   to read the data is faster because we no longer have to check the context for each
    //   call to read a record.
    int TGXFileObj::gdxDataReadRawFast(int SyNr, TDataStoreProc_t DP, int &NrRecs) {
        NrRecs = PrepareSymbolRead("gdxDataReadRawFast"s, SyNr, utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>(DOMC_UNMAPPED).data(), fr_raw_data);
        std::array<double, GMS_VAL_SCALE + 1> AVals {};
        int AFDim;
        while(DoRead(AVals.data(), AFDim))
            DP(LastElem.data(), AVals.data());
        gdxDataReadDone();
        return NrRecs >= 0;
    }

    std::string TGXFileObj::getImplName() const {
        return "tgxfileobj"s;
    }

#ifdef CPP_HASHMAP
    void TUELTable::clear() {
        UsrUel2Ent.clear();
        nameToIndexNum.clear();
    }

    int TUELTable::size() const {
        return static_cast<int>(nameToIndexNum.size());
    }

    int TUELTable::IndexOf(const std::string &s) {
        const auto it = nameToIndexNum.find(s);
        return it == nameToIndexNum.end() ? -1 : it->second.index;
    }

    int TUELTable::AddObject(const std::string &id, int mapping) {
        const auto &[it, wasNew] = nameToIndexNum.try_emplace(id, IndexNumPair{mapping});
        int ix;
        if (wasNew) {
            it->second.index = ix = static_cast<int>(nameToIndexNum.size());
#ifdef STABLE_REFS
            insertOrder.push_back(it);
#else
            insertOrder.push_back(id);
#endif
        }
        else ix = it->second.index;
        return ix;
    }

    int TUELTable::StoreObject(const std::string& id, int mapping) {
        int ix{static_cast<int>(nameToIndexNum.size())+1};
#ifdef STABLE_REFS
        auto [it,wasNew] = nameToIndexNum.emplace(id, IndexNumPair{ ix, mapping });
        assert(wasNew);
        insertOrder.push_back(it);
#else
        nameToIndexNum[id] = IndexNumPair{ix, mapping};
        insertOrder.push_back(id);
#endif
        return ix;
    }

    bool TUELTable::empty() const {
        return !size();
    }

    std::string TUELTable::operator[](int index) const {
#ifdef STABLE_REFS
        return insertOrder[index]->first;
#else
        return insertOrder[index];
#endif
    }

    int TUELTable::GetUserMap(int i) {
#ifdef STABLE_REFS
        return insertOrder[i-1]->second.num;
#else
        return nameToIndexNum[insertOrder[i-1]].num;
#endif
    }

    void TUELTable::SetUserMap(int EN, int N) {
#ifdef STABLE_REFS
        insertOrder[EN-1]->second.num = N;
#else
        nameToIndexNum[insertOrder[EN-1]].num = N;
#endif
    }

    void TUELTable::ResetMapToUserStatus() {
        FMapToUserStatus = map_unknown;
    }

    int TUELTable::NewUsrUel(int EN) {
        int res = GetUserMap(EN);
        if (res < 0) {
            res = UsrUel2Ent.GetHighestIndex() + 1;
            SetUserMap(EN, res);
            UsrUel2Ent[res] = EN;
        }
        ResetMapToUserStatus();
        return res;
    }

    int TUELTable::AddUsrNew(const std::string &s) {
        int EN{ AddObject(s, -1) };
#ifdef STABLE_REFS
        int res{ insertOrder[EN-1]->second.num };
#else
        int res{ nameToIndexNum[insertOrder[EN-1]].num };
#endif
        if (res < 0) {
            res = UsrUel2Ent.GetHighestIndex() + 1;
            SetUserMap(EN, res);
            UsrUel2Ent[res] = EN;
        }
        ResetMapToUserStatus();
        return res;
    }

    // FIXME: How does this affect the ordering / sort list?
    // Should renaming change the index?
    void TUELTable::RenameEntry(int N, const std::string &s) {

#ifdef STABLE_REFS
        auto old = insertOrder[N-1];
        auto oldPair = old->second;
        nameToIndexNum.erase(old->first);
        auto [it, wasNew] = nameToIndexNum.emplace(s, oldPair);
        assert(wasNew);
        insertOrder[N - 1] = it;
#else
        auto oldPair = nameToIndexNum[insertOrder[N-1]];
        nameToIndexNum.erase(insertOrder[N-1]);
        nameToIndexNum[s] = oldPair;
        insertOrder[N - 1] = s;
#endif
    }

    int TUELTable::GetMaxUELLength() const {
        int maxUelLength {};
        for(auto &pair : nameToIndexNum) {
            maxUelLength = std::max<int>((int)pair.first.length(), maxUelLength);
        }
        return maxUelLength;
    }

    void TUELTable::Reserve(int n) {
        UsrUel2Ent.reserve(n);
        nameToIndexNum.reserve(n);
    }

    int TUELTable::AddUsrIndxNew(const std::string &s, int UelNr) {
        int EN {AddObject(s, -1)};
#ifdef STABLE_REFS
        auto& itsNum = insertOrder[EN - 1]->second.num;
#else
        auto& itsNum = nameToIndexNum[insertOrder[EN-1]].num;
#endif
        int res {itsNum};
        if (res < 0) {
            itsNum = res = UelNr;
            UsrUel2Ent[res] = EN;
        }
        else if (res != UelNr)
            res = -1;
        ResetMapToUserStatus();
        return res;
    }

    TUELTable::TUELTable() {
#ifdef GOOGLE_HASHMAP
        nameToIndexNum.set_empty_key("");
#endif
        Reserve(10000);
    }
#endif

    int TAcronymList::FindEntry(int Map) const {
        const auto it = std::find_if(begin(), end(), [&](const auto &item) {
            return item.AcrMap == Map;
        });
        return it == end() ? -1 : static_cast<int>(std::distance(begin(), it));
    }

    int TAcronymList::AddEntry(const std::string &Name, const std::string &Text, int Map) {
        TAcronym acr {Name, Text, Map};
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
        for (int N{}; N < (int)size(); N++) {
            if ((*this)[N].FiltNumber == F.FiltNumber) {
                this->erase(this->begin() + N);
                break;
            }
        }
        this->push_back(F);
    }

    int TIntegerMapping::GetHighestIndex() const {
        return FHighestIndex;
    }

    void TIntegerMapping::SetMapping(int F, int T) {
        if(F >= (int)Map.size())
            growMapping(F);
        Map[F] = T;
        FHighestIndex = std::max<int>(FHighestIndex, F);
    }

    int TIntegerMapping::GetMapping(int F) const {
        return F >= 0 && F < (int)Map.size() ? Map[F] : -1;
    }

    void TIntegerMapping::clear() {
        Map.clear();
        FHighestIndex = 0;
    }

    int &TIntegerMapping::operator[](int index) {
        if(index >= (int)Map.size())
            growMapping(index);
        FHighestIndex = std::max<int>(FHighestIndex, index);
        return Map[index];
    }

    int TIntegerMapping::MemoryUsed() {
        return static_cast<int>(Map.size() * sizeof(int));
    }

    bool TIntegerMapping::empty() const {
        return Map.empty();
    }

    void TIntegerMapping::reserve(int n) {
        Map.reserve(n);
    }

    void TIntegerMapping::growMapping(int F) {
#ifndef NDEBUG
        bool at_max_capacity {FMAXCAPACITY <= (int64_t)Map.size()};
        assert(!at_max_capacity && "Already at maximum capacity: cannot grow TIntegerMapping");
#endif
        int64_t currCap, delta{};
        for(currCap = (int64_t)Map.size(); F >= currCap;
            currCap += delta, currCap = std::min<int64_t>(FMAXCAPACITY, currCap)) {
            if(currCap >= 1024 * 1024) delta = currCap / 2;
            else if(currCap <= 0) delta = 1024;
            else delta = currCap;
        }
        Map.resize(currCap, -1);
    }

    void TUELTableLegacy::clear() {
        Clear();
        UsrUel2Ent.clear();
    }

    int TUELTableLegacy::size() const {
        return FCount;
    }

    bool TUELTableLegacy::empty() const {
        return !FCount;
    }

    int TUELTableLegacy::GetUserMap(int i) {
        return GetObject(i)->num;
    }

    void TUELTableLegacy::SetUserMap(int EN, int N) {
        GetObject(EN)->num = N;
    }

    void TUELTableLegacy::ResetMapToUserStatus() {
        FMapToUserStatus = map_unknown;
    }

    int TUELTableLegacy::NewUsrUel(int EN) {
        int res = GetObject(EN)->num;
        if(res < 0) {
            res = UsrUel2Ent.GetHighestIndex() + 1;
            GetObject(EN)->num = res;
            UsrUel2Ent[res] = EN;
        }
        ResetMapToUserStatus();
        return res;
    }

    int TUELTableLegacy::AddUsrNew(const std::string &s) {
        int EN {AddObject(s, -1)};
        int res {GetObject(EN)->num};
        if(res < 0) {
            res = UsrUel2Ent.GetHighestIndex() + 1;
            GetObject(EN)->num = res;
            UsrUel2Ent[res] = EN;
        }
        ResetMapToUserStatus();
        return res;
    }

    int TUELTableLegacy::AddUsrIndxNew(const std::string &s, int UelNr) {
        int EN {AddObject(s, -1)};
        int res {GetObject(EN)->num};
        if(res < 0) {
            res = UelNr;
            GetObject(EN)->num = res;
            UsrUel2Ent[res] = EN;
        } else if(res != UelNr) {
            res = -1;
        }
        ResetMapToUserStatus();
        return res;
    }

    int TUELTableLegacy::GetMaxUELLength() const {
        int maxLen{};
        for(auto &bucket : Buckets)
            maxLen = std::max<int>(static_cast<int>(strlen(bucket.StrP)), maxLen);
        return maxLen;
    }

    void TUELTableLegacy::Reserve(int n) {
        UsrUel2Ent.reserve(n);
        Buckets.reserve(n);

    }

    TUELTableLegacy::TUELTableLegacy() {
        Buckets.reserve(10000);
        OneBased = true;
        ResetMapToUserStatus();
    }

    int TUELTableLegacy::IndexOf(const std::string &s) {
        return gdlib::strhash::TXStrHashList<gxfile::IndexNumPair>::IndexOf(s);
    }

    int TUELTableLegacy::AddObject(const std::string &id, int mapping) {
        IndexNumPair p{mapping};
        return gdlib::strhash::TXStrHashList<gxfile::IndexNumPair>::AddObject(id, p);
    }

    int TUELTableLegacy::StoreObject(const std::string& id, int mapping) {
        IndexNumPair p{ mapping };
        return gdlib::strhash::TXStrHashList<gxfile::IndexNumPair>::StoreObject(id, p);
    }

    const std::string TUELTableLegacy::operator[](int index) const {
        return GetString(index+1);
    }

    void TUELTableLegacy::RenameEntry(int N, const std::string &s) {
        gdlib::strhash::TXStrHashList<gxfile::IndexNumPair>::RenameEntry(N, s);
    }

    TUELUserMapStatus IUELTable::GetMapToUserStatus() {
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

    int VecSetTextList::size() const { return (int)entries.size(); }

    int VecSetTextList::Count() const { return (int)entries.size(); }

    void VecSetTextList::resize(int n) { entries.resize(n); }

    int VecSetTextList::AddObject(const std::string &s, int n) {
        int ix = utils::indexOf<SetText>(entries, [&s](const SetText &elem) {
            return elem.text == s;
        });
        if(ix == -1) {
            entries.emplace_back(s, n);
            return (int) entries.size() - 1;
        }
        return ix;
    }

    const std::string &VecSetTextList::GetString(int i) const {
        return entries[i].text;
    }

    int *VecSetTextList::GetObject(int i) {
        return &entries[i].node;
    }

    TgxModeSet::TgxModeSet(const std::initializer_list<TgxFileMode> modes) : modeActive(tgxfilemode_count) {
        std::fill(modeActive.begin(), modeActive.end(), false);
        count = 0;
        for (const auto mode : modes) {
            modeActive[mode] = true;
            count++;
        }
    }

    inline bool TgxModeSet::contains(const TgxFileMode& mode) const {
        return modeActive[mode];
    }

    inline bool TgxModeSet::empty() const {
        return !count;
    }
}