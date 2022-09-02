#include "xpwrap.h"
#include "expertapi/gdxcc.h"
#include <cassert>
#include <cstring>

namespace xpwrap {
    class CharBuf {
        std::array<char, 256> buf;

    public:
        char* get() { return buf.data(); }
        std::string str() const {
            return buf.data();
        };

        operator std::string() const {
            return str();
        }

        int size() const { return (int)buf.size(); }
    };

    class StrIndexBuffers {
        std::array<std::array<char, 256>, 20> bufContents{};
        std::array<char*, 20> bufPtrs{};
    public:
        StrIndexBuffers(const gxdefs::TgdxStrIndex *strIndex = nullptr) {
            for (int i{}; i < bufPtrs.size(); i++) {
                bufPtrs[i] = bufContents[i].data();
                if (strIndex) {
                    strcpy(bufPtrs[i], (*strIndex)[i].c_str());
                }
            }
        }

        char **ptrs() { return bufPtrs.data(); }
        const char** cptrs() { return (const char **)bufPtrs.data(); }

        gxdefs::TgdxStrIndex strs() const {
            gxdefs::TgdxStrIndex res;
            for (int i{}; i < res.size(); i++) {
                res[i].assign(bufPtrs[i]);
            }
            return res;
        }
    };

    int GDXFile::gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) {
        return ::gdxOpenWrite(pgx, FileName.c_str(), Producer.c_str(), &ErrNr);
    }

    int GDXFile::gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) {
        return ::gdxOpenWriteEx(pgx, FileName.c_str(), Producer.c_str(), Compr, &ErrNr);
    }

    int GDXFile::gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) {
        return ::gdxDataWriteStrStart(pgx, SyId.c_str(), ExplTxt.c_str(), Dim, Typ, UserInfo);
    }

    int GDXFile::gdxDataWriteStr(const gxdefs::TgdxStrIndex &KeyStr, const gxdefs::TgdxValues &Values) {
        static StrIndexBuffers keys{ &KeyStr };
        return ::gdxDataWriteStr(pgx, keys.cptrs(), Values.data());
    }

    int GDXFile::gdxDataWriteDone() {
        return ::gdxDataWriteDone(pgx);
    }

    GDXFile::GDXFile() {
        CharBuf msgBuf;
        assert(::gdxCreate(&pgx, msgBuf.get(), msgBuf.size()));
    }

    GDXFile::~GDXFile() {
        ::gdxFree(&pgx);
    }

    int GDXFile::gdxClose() {
        return ::gdxClose(pgx);
    }

    int GDXFile::gdxOpenRead(const std::string &FileName, int &ErrNr) {
        return ::gdxOpenRead(pgx, FileName.c_str(), &ErrNr);
    }

    int GDXFile::gdxFileVersion(std::string &FileStr, std::string &ProduceStr) {
        static CharBuf fsBuf, psBuf;
        int rc = ::gdxFileVersion(pgx, fsBuf.get(), psBuf.get());
        FileStr = fsBuf;
        ProduceStr = psBuf;
        return rc;
    }

    int GDXFile::gdxFindSymbol(const std::string &SyId, int &SyNr) {
        return ::gdxFindSymbol(pgx, SyId.c_str(), &SyNr);
    }

    int GDXFile::gdxDataReadStr(gxdefs::TgdxStrIndex &KeyStr, gxdefs::TgdxValues &Values, int &DimFrst) {
        static StrIndexBuffers keys;
        int rc {::gdxDataReadStr(pgx, keys.ptrs(), Values.data(), &DimFrst)};
        KeyStr = keys.strs();
        return rc;
    }

    int GDXFile::gdxDataReadDone() {
        return ::gdxDataReadDone(pgx);
    }

    int GDXFile::gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) {
        static CharBuf SyIdBuf;
        int rc{::gdxSymbolInfo(pgx, SyNr, SyIdBuf.get(), &Dim, &Typ)};
        SyId = SyIdBuf;
        return rc;
    }

    int GDXFile::gdxDataReadStrStart(int SyNr, int &NrRecs) {
        return ::gdxDataReadStrStart(pgx, SyNr, &NrRecs);
    }

    int GDXFile::gdxAddAlias(const std::string &Id1, const std::string &Id2) {
        return ::gdxAddAlias(pgx, Id1.c_str(), Id2.c_str());
    }

    int GDXFile::gdxAddSetText(const std::string &Txt, int &TxtNr) {
        return ::gdxAddSetText(pgx, Txt.c_str(), &TxtNr);
    }

    int GDXFile::gdxDataErrorCount() {
        return ::gdxDataErrorCount(pgx);
    }

    int GDXFile::gdxDataErrorRecord(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values) {
        return ::gdxDataErrorRecord(pgx, RecNr, KeyInt.data(), Values.data());
    }
    int GDXFile::gdxDataErrorRecordX(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values) {
        return ::gdxDataErrorRecordX(pgx, RecNr, KeyInt.data(), Values.data());
    }

    int GDXFile::gdxDataReadRaw(gxdefs::TgdxUELIndex &KeyInt, gxdefs::TgdxValues &Values, int &DimFrst) {
        return ::gdxDataReadRaw(pgx, KeyInt.data(), Values.data(), &DimFrst);
    }

    int GDXFile::gdxDataReadRawStart(int SyNr, int &NrRecs) {
        return ::gdxDataReadRawStart(pgx, SyNr, &NrRecs);
    }

    int GDXFile::gdxDataWriteRaw(const gxdefs::TgdxUELIndex &KeyInt, const gxdefs::TgdxValues &Values) {
        return ::gdxDataWriteRaw(pgx, KeyInt.data(), Values.data());
    }

    int GDXFile::gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                      int UserInfo) {
        return ::gdxDataWriteRawStart(pgx, SyId.c_str(), ExplTxt.c_str(), Dimen, Typ, UserInfo);
    }

    int GDXFile::gdxErrorCount() {
        return ::gdxErrorCount(pgx);
    }
    
    int GDXFile::gdxErrorStr(int ErrNr, std::string &ErrMsg) {
        static CharBuf ErrMsgBuf;
        int rc{ ::gdxErrorStr(pgx, ErrNr, ErrMsgBuf.get()) };
        ErrMsg = ErrMsgBuf;
        return rc;
    }

    int GDXFile::gdxGetElemText(int TxtNr, std::string &Txt, int &Node) {
        static CharBuf TxtBuf;
        int rc{ ::gdxGetElemText(pgx, TxtNr, TxtBuf.get(), &Node) };
        Txt = TxtBuf;
        return rc;

    }

    int GDXFile::gdxGetLastError() {
        return ::gdxGetLastError(pgx);
    }

    int GDXFile::gdxGetSpecialValues(gxdefs::TgdxSVals &Avals) {
        return ::gdxGetSpecialValues(pgx, Avals.data());
    }

    int GDXFile::gdxSetSpecialValues(const gxdefs::TgdxSVals &AVals) {
        return ::gdxSetSpecialValues(pgx, AVals.data());
    }

    int GDXFile::gdxSymbolGetDomain(int SyNr, gxdefs::TgdxUELIndex &DomainSyNrs) {
        return ::gdxSymbolGetDomain(pgx, SyNr, DomainSyNrs.data());
    }

    int GDXFile::gdxSymbolGetDomainX(int SyNr, gxdefs::TgdxStrIndex &DomainIDs) {
        static StrIndexBuffers domainIdBufs;
        int rc{ ::gdxSymbolGetDomainX(pgx, SyNr, domainIdBufs.ptrs()) };
        DomainIDs = domainIdBufs.strs();
        return rc;
    }

    int GDXFile::gdxSymbolDim(int SyNr) {
        return ::gdxSymbolDim(pgx, SyNr);
    }

    int GDXFile::gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, std::string &ExplTxt) {
        static CharBuf explTxtBuf;
        int rc = ::gdxSymbolInfoX(pgx, SyNr, &RecCnt, &UserInfo, explTxtBuf.get());
        ExplTxt = explTxtBuf;
        return rc;
    }

    int GDXFile::gdxSymbolSetDomain(const gxdefs::TgdxStrIndex &DomainIDs) {
        static StrIndexBuffers domainIdBufs{ &DomainIDs };
        return ::gdxSymbolSetDomain(pgx, domainIdBufs.cptrs());
    }

    int GDXFile::gdxSymbolSetDomainX(int SyNr, const gxdefs::TgdxStrIndex &DomainIDs) {
        static StrIndexBuffers domainIdBufs{ &DomainIDs };
        return ::gdxSymbolSetDomainX(pgx, SyNr, domainIdBufs.cptrs());
    }

    int GDXFile::gdxSystemInfo(int &SyCnt, int &UelCnt) {
        return ::gdxSystemInfo(pgx, &SyCnt, &UelCnt);
    }

    int GDXFile::gdxUELRegisterDone() {
        return ::gdxUELRegisterDone(pgx);
    }

    int GDXFile::gdxUELRegisterRaw(const std::string &Uel) {
        return ::gdxUELRegisterRaw(pgx, Uel.c_str());
    }

    int GDXFile::gdxUELRegisterRawStart() {
        return ::gdxUELRegisterRawStart(pgx);
    }

    int GDXFile::gdxUELRegisterStr(const std::string &Uel, int &UelNr) {
        return ::gdxUELRegisterStr(pgx, Uel.c_str(), &UelNr);
    }

    int GDXFile::gdxUELRegisterStrStart() {
        return ::gdxUELRegisterStrStart(pgx);
    }

    int GDXFile::gdxUMUelGet(int UelNr, std::string &Uel, int &UelMap) {
        CharBuf uelBuf;
        int rc{ ::gdxUMUelGet(pgx, UelNr, uelBuf.get(), &UelMap)};
        Uel = uelBuf;
        return rc;
    }

    int GDXFile::gdxUMUelInfo(int &UelCnt, int &HighMap) {
        return ::gdxUMUelInfo(pgx, &UelCnt, &HighMap);
    }

    int GDXFile::gdxCurrentDim() {
        return ::gdxCurrentDim(pgx);
    }

    int GDXFile::gdxRenameUEL(const std::string &OldName, const std::string &NewName) {
        return ::gdxRenameUEL(pgx, OldName.c_str(), NewName.c_str());
    }
}