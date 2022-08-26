#include "gxfile.h"
#include "rtl/sysutils_p3.h"
#include "utils.h"
#include "gdlib/gmsstrm.h"
#include "global/modhead.h"

using namespace gdxinterface;
using namespace gdlib::gmsstrm;

namespace gxfile {

    const int   VERSION = 7,
                gdxHeaderNr = 123;
    const std::string gdxHeaderId = "GAMSGDX";

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

    int GetEnvCompressFlag() {
        std::string s{ rtl::sysutils_p3::QueryEnvironmentVariable("GDXCOMPRESS") };
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
            FFile.reset();
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
        FFile->WriteByte(gdxHeaderNr);
        FFile->WriteString(gdxHeaderId);
        VersionRead = VERSION;
        FFile->WriteInteger(VersionRead);
        FFile->WriteInteger(Compr);
        // FIXME: Call actual function (see cppmex palxxx::gdlaudit)
        FileSystemID = ""; //gdlGetAuditLine();
        FFile->WriteString(FileSystemID);
        FProducer = Producer;
        FProducer2.clear();
        FFile->WriteString(FProducer);
        // Reserve some space for positions
        MajorIndexPosition = FFile->GetPosition();
        for(int N{1}; N<=10; N++) FFile->WriteInt64(0);
        // ...
        return 0;
    }

    int TGXFileObj::gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ,
                                         int UserInfo) {
        // ...
        STUBWARN();
        return 0;
    }

    int TGXFileObj::gdxDataWriteStr(const TgdxStrIndex &KeyStr, const TgdxValues &Values) {
        // ...
        STUBWARN();
        return 0;
    }

    int TGXFileObj::gdxDataWriteDone() {
        // ...
        STUBWARN();
        return 0;
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
        // ...
        STUBWARN();
        return 0;
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
        return 0;
    }
}