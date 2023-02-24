/*
* Important remarks by André:
* At first I did not fully port the gmsstrms but instead tried to do a simpler solution just using C++ standard library IO streams
* There I struggled with porting all features like encryption and compression.
* Then I added new classes with suffix Delphi, these should replicate the Delphi classes in full detail
* Long term I want to merge the simplified and full ports of the stream classes in order to get all functionality will still cleaning stuff up.
*/

#include <cassert>
#include <sstream>
#include <cstring>

#include <string>
#include <fstream>
#include <utility>

// only supported by MSVC so far :(
//#include <format>

#include "../expertapi/gclgms.h"

#include "../rtl/p3utils.h"
#include "../rtl/sysutils_p3.h"

#include "../utils.h"

#include "gmsstrm.h"

using namespace rtl::p3utils;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdlib::gmsstrm {

    const uint8_t signature_header = 0xFF;
    const std::string signature_gams = "*GAMS*"s;
    const int verify_offset = 100;

    union TDoubleVar {
        //bool realNumOrBytes;
        double V;
        std::array<uint8_t, 8> VA;
    };

    // take the bytes of psrc and return them - in reverse order - in pdest
    // sz should be at most 8, but we do not promise to check this!
    void reverseBytesMax8(void *psrc, void *pdest, int sz) {
        std::array<uint8_t, 8> orig{}, flip{};
        int n{std::min(sz-1, 7)};
        memcpy(orig.data(), (const char *)psrc, n+1);
        for(int k{}; k<=n; k++)
            flip[k] = orig[n-k];
        memcpy((char *)pdest, flip.data(), n+1);
    }

    TBinaryTextFileIO *TBinaryTextFileIO::FromString(const std::string &contents, int &ErrNr) {
        return new TBinaryTextFileIO {contents, ErrNr};
    }

    int TBinaryTextFileIO::GetLastIOResult() const {
        return FLastIOResult;
    }

    // Corresponds to gmsstrm.TBinaryTextFileIO.OpenForRead from CMEX
    // AS: Not sure if it makes sense to duplicate the whole gmsstrm type hierarchy, since std::streams already do some of the low level lifting
    TBinaryTextFileIO::TBinaryTextFileIO(const std::string& fn, const std::string& PassWord, int& ErrNr,
        std::string& errmsg) : frw{}, NrWritten{} {
        offsetInBuffer = std::nullopt;
        std::string Msg;
        FCanCompress = true;
        ErrNr = strmErrorNoError;
        FS = std::make_unique<std::fstream>(fn, std::ios::in | std::ios::binary);
        if (FS->rdstate() & std::ifstream::failbit) {
            std::cout << "Unable to open " + fn + " for reading!" << std::endl;
            FLastIOResult = 1;
        }
        else FLastIOResult = 0;
        ErrNr = FLastIOResult;
        if (ErrNr) {
            errmsg = rtl::sysutils_p3::SysErrorMessage(ErrNr);
            ErrNr = strmErrorIOResult;
            return;
        }
        uint8_t B1 = ReadByte(), B2 = ReadByte();
        if (B1 == 31 && B2 == 139) {
            FFileSignature = fsign_gzip;
            gzFS = std::make_unique<TGZipInputStream>(fn, errmsg);
            if (!errmsg.empty()) ErrNr = 1;
            return;
        }
        std::string srcBuf;
        srcBuf.resize(B2);
        if (B1 == signature_header) Read(srcBuf.data(), B2);
        if (B1 != signature_header || srcBuf != signature_gams) {
            utils::tBomIndic fileStart = { B1, B2, ReadByte(), ReadByte() };
            int BOMOffset;
            if (!utils::checkBOMOffset(fileStart, BOMOffset, errmsg)) {
                ErrNr = strmErrorEncoding;
                return;
            }
            // TODO: Investigate of SetPosition does additional relevant stuff apart from doing seek(0), Nr{Read,Loaded}=0
            FS->seekg(BOMOffset);
            NrRead = NrLoaded = FRewindPoint = BOMOffset;
            FMajorVersionRead = 0;
            FMinorVersionRead = 0;
            FFileSignature = fsign_text;
            errmsg.clear();
            return;
        }
        ErrNr = strmErrorGAMSHeader;
        errmsg = "GAMS header not found";
        char b{};
        Read(&b, 1);
        FFileSignature = (TFileSignature)( (int)b - (int)'A' );
        ReadString();
        FMajorVersionRead = ReadByte();
        FMinorVersionRead = ReadByte();
        char Ch{ ReadChar() };
        bool hasPswd = Ch == 'P';
        if (!hasPswd && Ch != 'p') return;
        Ch = ReadChar();
        bool hasComp = Ch == 'C';
        if (!hasComp && Ch != 'c') return;
        if (hasPswd && PassWord.empty()) {
            ErrNr = strmErrorNoPassWord;
            errmsg = "A Password is required";
            return;
        }
        ErrNr = strmErrorIntegrity;
        errmsg = "Integrity check failed";
        if (hasPswd) {
            SetPassword(PassWord);
            std::string src = ReadString(), targ;
            ApplyPassword(src, targ, verify_offset);
            if (targ != RandString((int)src.length())) return;
        }
        // keep buffer logic going
        FRewindPoint = (int)FS->tellg();
        SetCompression(true);
        FS->seekg(FRewindPoint);
        if (!hasComp) SetCompression(false);
        if (ReadString() != signature_gams) return;
        ErrNr = strmErrorNoError;
        errmsg.clear();
        if(!noBuffering) offsetInBuffer = std::make_optional<uint64_t>(0);
    }

    /*
     * Structure of the file header
           header uncompressed / no password applied
           B   #255
           S   '*GAMS*'
           B   file type
           S   producer info
           B   major version
           B   minor version
           C   P/p password used
           C   C/c compression used
           S   A 'random' ShortString of the length of the password based on the password length
           S   '*GAMS*' encrypted compressed
    */
    // Corresponds to gmsstrm.TBinaryTextFileIO.OpenForWrite from CMEX
    TBinaryTextFileIO::TBinaryTextFileIO(const std::string& fn,
                                         const std::string& Producer,
                                         const std::string& PassWord,
                                         const TFileSignature signature,
                                         bool comp,
                                         int& ErrNr,
                                         std::string& errmsg)
             : FMajorVersionRead{}, FMinorVersionRead{}, FRewindPoint{}, NrLoaded{}
     {
        NrRead = NrWritten = 0;
        FCanCompress = true;
        FFileSignature = signature;
        frw = fm_write;
        ErrNr = strmErrorNoError;
        FS = std::make_unique<std::fstream>(fn, std::ios::out | std::ios::binary);
        if (FS->rdstate() & std::ifstream::failbit) {
            std::cout << "Unable to open " + fn + " for writing!" << std::endl;
            FLastIOResult = 1;
        }
        else FLastIOResult = 0;
        if(signature != fsign_text || !PassWord.empty() || comp) {
            WriteByte(signature_header);
            WriteString(signature_gams);
            WriteByte(signature + 'A');
            WriteString(Producer);
            WriteByte(1); // version
            WriteByte(1); // sub version
            WriteByte(PassWord.empty() ? 'p' : 'P');
            WriteByte(comp ? 'C' : 'c');
            if(!PassWord.empty()) {
                FS->flush();
                SetPassword(PassWord);
                std::string src = RandString((int)PassWord.length()), targ;
                ApplyPassword(src, targ, verify_offset);
                SetPassword("");
                WriteString(targ);
            }
            if(comp) SetCompression(true);
            else FS->flush();
            SetPassword(PassWord);
            // write a few bytes to be recognized later (compression / password is now active)
            WriteString(signature_gams);
        }
        ErrNr = FLastIOResult;
        if(!ErrNr) {
            ErrNr = strmErrorNoError;
            errmsg.clear();
        } else errmsg = rtl::sysutils_p3::SysErrorMessage(ErrNr);
    }

    TBinaryTextFileIO::~TBinaryTextFileIO() = default;

    int TBinaryTextFileIO::Read(char* Buffer, int Count/*, bool WithoutMoving*/) {
        /*if(WithoutMoving && Count == 1) {
            *Buffer = (char)FS.peek();
            return *Buffer != std::char_traits<char>::eof() ? 1 : 0;
        }*/

        if (FFileSignature == fsign_gzip)
            return gzFS->Read(Buffer, Count);

        int numBytesRetrieved{Count};

        // offsetInBuffer < 0 skips buffering
        if(!noBuffering && offsetInBuffer) {
            maybeFillReadBuffer();
            int bytesRemaining = (int)(readBuffer.size() - *offsetInBuffer);
            if(bytesRemaining < Count) numBytesRetrieved = bytesRemaining;
            memcpy(Buffer, &readBuffer.data()[*offsetInBuffer], numBytesRetrieved);
            *offsetInBuffer += numBytesRetrieved;
            NrRead += numBytesRetrieved;
        } else {
            FS->read(Buffer, Count);
            numBytesRetrieved = static_cast<int>(FS->gcount());
            NrRead += Count;
        }

        //FS->read(Buffer, Count);
        //int numBytesReceived = static_cast<int>(FS->gcount());

        /*if(WithoutMoving) {
            FS.seekg(-Count, std::fstream::cur);
        }*/

        return numBytesRetrieved;
    }

    char TBinaryTextFileIO::ReadCharacter() {
        char ch;
        /*FS->read(&ch, 1);
        NrRead++;*/
        Read(&ch, 1);
        return ch;
    }

    bool TBinaryTextFileIO::UsesPassWord() {
        return !FPassword.empty();
    }

    // FIXME: The behavior of this ReadLine differs from the corresponding Delphi method
    // This should be replaced by TBinaryTextFileIODelphi, which more closely resembles the Delphi code soon!
    void TBinaryTextFileIO::ReadLine(std::string &Buffer, int &Len, char &LastChar) {
        /*Buffer = utils::getLineWithSep(FS);
        LastChar = Buffer.back(); // should be carriage return or line feed
        assert(LastChar == '\r' || LastChar == '\n');
        utils::removeTrailingCarriageReturnOrLineFeed(Buffer);
        Len = static_cast<int>(Buffer.length());*/

        if (FFileSignature == fsign_gzip) {
            std::vector<uint8_t> vecBuf;
            gzFS->ReadLine(vecBuf, std::numeric_limits<int>::max(), LastChar);
            Buffer.assign((char *)vecBuf.data());
            return;
        }

        std::set<char> termChars = {'\r', '\n', std::char_traits<char>::eof() };
        for(Buffer.clear(), Len = 0; !utils::in(LastChar, termChars) && Len < 80001; Len++) {
            Buffer += LastChar;
            if(Read(&LastChar, 1) <= 0)
                LastChar = std::char_traits<char>::eof();
        }
    }

    int TBinaryTextFileIO::Write(const char* Buffer, int Count) {
        assert(frw == fm_write && "TBinaryTextFileIO.Write");
        if(!FS) return -1;
        FS->write(Buffer, Count);
        return Count;
    }

    void TBinaryTextFileIO::ReWind() {
        FS->clear();
        FS->seekg(0);
        if(offsetInBuffer && lastReadCount > 0) lastReadCount = 0;
    }

    uint8_t TBinaryTextFileIO::ReadByte()
    {
        if (Paranoid) ParCheck(rw_byte);
        char buf{};
        Read(&buf, 1);
        return buf;
    }

    char TBinaryTextFileIO::ReadChar()
    {
        if (Paranoid) ParCheck(rw_byte);
        char buf{};
        Read(&buf, 1);
        return buf;
    }

    void TBinaryTextFileIO::ParCheck(RWType T)
    {
        char B{};
        Read(&B, 1);
        if (B != T) {
            std::string suffix = B > rw_count ? "???" + std::to_string(B) : RWTypeText[B];
            throw std::runtime_error("Stream check failed: Expected = " + RWTypeText[T] + " Read = " + suffix);
        }
    }

    std::string TBinaryTextFileIO::ReadString()
    {
        if (Paranoid) ParCheck(rw_string);
        char len { 0 };
        if (!Read(&len, 1) || !len) return ""s;
        std::string res;
        res.resize(len);
        Read(res.data(), len);
        return res;
    }

    TBinaryTextFileIO::TBinaryTextFileIO(const std::string &contents, int &ErrNr)
     : frw{}, FFileSignature{}, NrRead{}, NrWritten{}, FCanCompress{}
     {
        ErrNr = strmErrorNoError;
        FS = std::make_unique<std::stringstream>(contents, std::fstream::in);
    }

    void TBinaryTextFileIO::SetPassword(const std::string &s) {
        FPassword.clear();
        if(s.empty()) return;
        bool BB{};
        for(int K{}; K<(int)s.length(); K++) {
            if(s[K] != ' ') BB = false;
            else {
                if(BB) continue;
                BB = true;
            }
            char B = s[K];
            if(!(B & 1)) B >>= 1;
            else B = 0x80 + (B >> 1);
            FPassword += B;
        }
    }

    void TBinaryTextFileIO::ApplyPassword(std::string &PR, std::string &PW, int64_t Offs) {
        const auto L = FPassword.length();
        int FPwNxt = static_cast<int>(Offs % L);
        std::transform(std::cbegin(PR), std::cend(PR), std::begin(PW), [&](const char c) {
            return static_cast<char>(utils::excl_or(c,FPassword[FPwNxt++ % L]));
        });
    }

    std::string TBinaryTextFileIO::RandString(int L) {
        int Seed{};
        auto RandCh = [&]() {
            Seed = (Seed * 12347 + 1023) & 0x7FFFFFF;
            return static_cast<char>(Seed & 0xFF);
        };
        Seed = 1234 * L;
        return utils::constructStr(L, [&](int i) { return RandCh(); });
    }

    void TBinaryTextFileIO::WriteByte(uint8_t B) {
        if(Paranoid) ParWrite(rw_byte);
        FS->write((const char *)&B, 1);
    }

    void TBinaryTextFileIO::ParWrite(RWType T) {
        uint8_t B = T;
        FS->write((const char *)&B, 1);
    }

    void TBinaryTextFileIO::WriteString(const std::string_view s) {
        static std::array<char, 256> buf {};
        if(Paranoid) ParWrite(rw_string);
        utils::strConvCppToDelphi(s, buf.data());
        FS->write(buf.data(), s.length()+1);
    }

    void TBinaryTextFileIO::SetCompression(bool V) {
        if((FCompress || V) && NrWritten > 0) FS->flush();
        if(FCompress != V)
            NrLoaded = NrRead = 0;
        FCompress = V;
    }

    void TBinaryTextFileIO::maybeFillReadBuffer() {
        if(!lastReadCount || *offsetInBuffer >= readBuffer.size()) {
            FS->read(readBuffer.data(), BufferSize);
            lastReadCount = FS->gcount();
            *offsetInBuffer = 0;
        }
    }

    void CompressTextFile(const std::string& fn, const std::string& fo, const std::string& PassWord, bool Comp, int& ErrNr, std::string& ErrMsg) {
        TBinaryTextFileIODelphi Fin{ fn, "", ErrNr, ErrMsg };
        if (!ErrMsg.empty()) return;

        TBinaryTextFileIODelphi Fout{ fo, "CompressTextFile", PassWord, fsign_text, Comp, ErrNr, ErrMsg };
        if (!ErrMsg.empty()) return;

        std::array<char, 4096> Buffer{};
        int NrRead{};
        do {
            NrRead = Fin.Read(Buffer.data(), (int)Buffer.size());
            if (!NrRead) break;
            Fout.Write(Buffer.data(), NrRead);
        } while (NrRead >= (int)Buffer.size());
    }

    void UnCompressTextFile(const std::string& fn, const std::string& fo, const std::string& PassWord, int& ErrNr, std::string& ErrMsg) {
        TBinaryTextFileIODelphi Fin{ fn, PassWord, ErrNr, ErrMsg };
        if (!ErrMsg.empty()) return;

        TBinaryTextFileIODelphi Fout{ fo, "", "", fsign_text, false, ErrNr, ErrMsg };
        if (!ErrMsg.empty()) return;

        const int BufSize = 4096;
        char Buffer[BufSize];

        int NrRead{};
        do {
            NrRead = Fin.Read(Buffer, BufSize);
            if (!NrRead) break;
            Fout.Write(Buffer, NrRead);
        } while (NrRead >= BufSize);
    }

    TGZipInputStream::TGZipInputStream(const std::string& fn, std::string& ErrMsg) {
        pgz = gzopen(fn.c_str(), "r");
        if(!pgz) ErrMsg = "Cannot open file";
        else {
            ErrMsg.clear();
            Buf.resize(BufferSize);
            NrRead = NrLoaded = 0;
        }
    }

    TGZipInputStream::~TGZipInputStream() {
        gzclose(pgz);
    }

    global::delphitypes::LongWord TGZipInputStream::Read(void *buffer, unsigned int Count) {
        std::function<bool()> FillBuffer = [&]() {
            NrLoaded = gzread(pgz, Buf.data(), (int)this->Buf.size());
            NrRead = 0;
            return NrLoaded > 0;
        };

        if(Count <= NrLoaded - NrRead) {
            memcpy(&Buf[NrRead], buffer, Count);
            NrRead += Count;
            return Count;
        } else {
            int UsrReadCnt {};
            for(unsigned int NrBytes{}; Count > 0; NrRead += NrBytes, UsrReadCnt += NrBytes, Count -= NrBytes) {
                if(NrRead >= NrLoaded && !FillBuffer()) break;
                NrBytes = static_cast<unsigned int>(NrLoaded - NrRead);
                if(NrBytes > Count) NrBytes = Count;
                memcpy(&Buf[NrRead], &((uint8_t *)buffer)[UsrReadCnt], NrBytes);
            }
            return UsrReadCnt;
        }
    }

    void TGZipInputStream::ReadLine(std::vector<uint8_t> &buffer, int MaxInp, char &LastChar) {
        const char subst = (char)0x1A;
        while(!utils::in<char>(LastChar, '\r', '\n', subst) || (int)buffer.size() == MaxInp) {
            buffer.push_back(LastChar);
            if(NrLoaded - NrRead >= 1)  {
                LastChar = (char)Buf[NrRead++];
            } else if(Read(&LastChar, 1) <= 0) LastChar = subst;
        }
    }

    void TXStreamDelphi::ParWrite(RWType T) {
        Write(&T, 1);
    }

    void TXStreamDelphi::ParCheck(RWType T) {
        uint8_t B;
        Read(&B, 1);
        if(B != T)
            throw std::runtime_error("Stream check failed: Expected = " + RWTypeText[T] + " Read = "
                + (B > RWType::rw_count ? ("???" + std::to_string(B)) : RWTypeText[B]));
    }

    void TXStreamDelphi::WriteString(const std::string_view s) {
        static int cnt{};
        static std::array<char, 256> buf{};
        if(fstext)
            *fstext << "WriteString@" << GetPosition() << "#" << ++cnt << ": " << s << "\n";
        if (Paranoid) ParWrite(rw_string);
        utils::strConvCppToDelphi(s, buf.data());
        Write(buf.data(), (uint32_t)s.length()+1);
    }

    void TXStreamDelphi::WriteDouble(double x) {
        static int cnt {};
        if (fstext)
            *fstext << "WriteDouble@"<< GetPosition()  << "#" << ++cnt << ": " << utils::asdelphifmt(x, 12) << '\n';
        WriteValue(rw_double, x);
    }

    void TXStreamDelphi::WriteInteger(int n) {
        static int cnt {};
        if (fstext)
            *fstext << "WriteInteger@"<< GetPosition() << "#" << ++cnt << ": " << n << '\n';
        WriteValue(rw_integer, n);
    }

    void TXStreamDelphi::WriteInt64(int64_t N) {
        static int cnt{};
        if (fstext)
            *fstext << "WriteInt64@"<< GetPosition() << "#"<< ++cnt << ": " << N << '\n';
        WriteValue(rw_int64, N);
    }

    void TXStreamDelphi::WriteByte(uint8_t b) {
        static int cnt{};
        if(fstext)
            *fstext << "WriteByte@" << GetPosition() << "#" << ++cnt << ": " << std::to_string(b) << '\n';
        WriteValue(rw_byte, b);
    }

    void TXStreamDelphi::WriteWord(uint16_t W) {
        static int cnt{};
        if (fstext)
            *fstext << "WriteWord@" << GetPosition() << "#" << ++cnt << ": " << W << '\n';
        WriteValue(rw_word, W);
    }

    void TXStreamDelphi::WriteBool(bool B) {
        static int cnt{};
        if (fstext)
            *fstext << "WriteBool@" << GetPosition() << "#" << ++cnt << ": " << (B ? "True" : "False") << '\n';
        WriteValue(rw_bool, B);
    }

    void TXStreamDelphi::WriteChar(char C) {
        static int cnt {};
        if (fstext)
            *fstext << "WriteChar@" << GetPosition() << "#" << ++cnt << ": " << C << '\n';
        WriteValue(rw_char, C);
    }

    void TXStreamDelphi::WritePChar(const char *s, int L) {
        if(Paranoid) ParWrite(rw_pchar);
        WriteInteger(L);
        if(L > 0) Write(s, L);
    }

    std::string TXStreamDelphi::ReadString() {
        if(Paranoid) ParCheck(rw_string);
        uint8_t len;
        if(!Read(&len, 1)) return ""s;
        std::string s;
        s.resize(len+1);
        Read(s.data(), len);
        s.resize(len);
        return s;
    }

    double TXStreamDelphi::ReadDouble() {
        return ReadValue<double>(rw_double);
    }

    int TXStreamDelphi::ReadInteger() {
        return ReadValue<int>(rw_integer);
    }

    uint8_t TXStreamDelphi::ReadByte() {
        return ReadValue<uint8_t>(rw_byte);
    }

    uint16_t TXStreamDelphi::ReadWord() {
        return ReadValue<uint16_t>(rw_word);
    }

    int64_t TXStreamDelphi::ReadInt64() {
        return ReadValue<int64_t>(rw_int64);
    }

    bool TXStreamDelphi::ReadBool() {
        return ReadValue<bool>(rw_bool);
    }

    char TXStreamDelphi::ReadChar() {
        return ReadValue<char>(rw_char);
    }

    void TXStreamDelphi::ReadPChar(char *P, int &L) {
        if(Paranoid) ParCheck(rw_pchar);
        L = ReadInteger();
        if(L <= 0) P = nullptr;
        else {
            P = new char[L];
            Read(P, L);
        }
    }

    void TXStreamDelphi::ActiveWriteOpTextDumping(const std::string &dumpFilename) {
        fstext = std::make_unique<std::ofstream>(dumpFilename);
    }

    void TXFileStreamDelphi::SetLastIOResult(int V) {
        if(!FLastIOResult) FLastIOResult = V;
    }

    void TXFileStreamDelphi::SetPassWord(const std::string &s) {
        FPassWord.clear();
        if(s.empty()) return;
        bool BB{};
        for(int K{}; K<(int)s.length(); K++) {
            if(s[K] != ' ') BB = false;
            else {
                if(BB) continue;
                BB = true;
            }
            char B = s[K];
            if(!(B & 1)) B >>= 1;
            else B = 0x80 + (B >> 1);
            FPassWord += B;
        }
    }

    bool TXFileStreamDelphi::GetUsesPassWord() {
        return !FPassWord.empty();
    }

    std::string TXFileStreamDelphi::RandString(int L) {
        int Seed{};
        auto RandCh = [&]() {
            Seed = (Seed * 12347 + 1023) & 0x7FFFFFF;
            return static_cast<char>(Seed & 0xFF);
        };
        Seed = 1234 * L;
        return utils::constructStr(L, [&](int i) { return RandCh(); });
    }

    int64_t TXFileStreamDelphi::GetSize()
    {
        int64_t res;
        SetLastIOResult(rtl::p3utils::p3FileGetSize(FS.get(), res));
        return res;
    }

    int64_t TXFileStreamDelphi::GetPosition()
    {
        return PhysPosition;
    }

    void TXFileStreamDelphi::SetPosition(int64_t P)
    {
        PhysPosition = P;
        //int64_t NP;
        FS->seekp(P);
        SetLastIOResult(FS->bad() ? 1 : 0);
        //SetLastIOResult(P3FileSetPointer(FS, P, NP, P3_FILE_BEGIN));
    }

    TXFileStreamDelphi::TXFileStreamDelphi(std::string AFileName, const FileAccessMode AMode)
        : FS{}, FFileName{std::move( AFileName )}, FPassWord{}, FLastIOResult{}, PhysPosition{}
    {
        rtl::p3utils::Tp3FileOpenAction FMode{ p3OpenRead };
        switch (AMode) {
        case fmCreate:
        case fmOpenWrite:
            FMode = p3OpenWrite;
            break;
        case fmOpenRead:
            FMode = p3OpenRead;
            break;
        case fmOpenReadWrite:
            FMode = p3OpenReadWrite;
            break;
        default:
            throw std::runtime_error("TXFileStream.Create = "s + std::to_string(AMode));
        }
        FS = std::make_unique<std::fstream>();
        SetLastIOResult(p3FileOpen(FFileName, FMode, FS.get()));
        FileIsOpen = !FLastIOResult;
    }

    TXFileStreamDelphi::~TXFileStreamDelphi()
    {
        if (FileIsOpen) {
            FS->close();
            SetLastIOResult(!FS->good() ? 1 : 0);
        }
    }

    void TXFileStreamDelphi::ApplyPassWord(const char *PR, char *PW, int Len, int64_t Offs)
    {
        const int L = (int)FPassWord.length();
        int FPwNxt = (int)Offs % L;
        for (int N{}; N < Len; N++) {
            FPwNxt++;
            if (FPwNxt > L) FPwNxt = 1;
            PW[N] = (char)utils::excl_or(PR[N], FPassWord[FPwNxt]);
        }
    }

    uint32_t TXFileStreamDelphi::Read(void* Buffer, uint32_t Count)
    {
        uint32_t res;
        if (FPassWord.empty())
            SetLastIOResult(p3FileRead(FS.get(), (char *)Buffer, Count, res));
        else {
            auto PW = (char *)Buffer;
            std::vector<char> PR(Count);
            SetLastIOResult(p3FileRead(FS.get(), PR.data(), Count, res));
            ApplyPassWord(PR.data(), PW, (int)Count, PhysPosition);
        }
        PhysPosition += res;
        return res;
    }

    uint32_t TXFileStreamDelphi::Write(const void* Buffer, uint32_t Count) {
        if (FPassWord.empty()) {
            FS->write((const char *)Buffer, Count);
        }
        else {
            auto PR = (const char *)Buffer;
            std::vector<char> PW(Count);
            ApplyPassWord(PR, PW.data(), (int)Count, PhysPosition);
        }
        SetLastIOResult(FS->bad() ? 1 : 0);
        PhysPosition += Count;
        return Count;
    }

    int TXFileStreamDelphi::GetLastIOResult() {
        int res{FLastIOResult};
        FLastIOResult = 0;
        return res;
    }

    std::string TXFileStreamDelphi::GetFileName() const {
        return FFileName;
    }

    bool TBufferedFileStreamDelphi::FillBuffer() {
        if(!FCompress) NrLoaded = TXFileStreamDelphi::Read(BufPtr.data(), BufSize);
        else if(!FCanCompress) {
            NrLoaded = 0;
            FLastIOResult = -100044; // check with gxdefs.pas
        } else {
            uint16_t RLen = TXFileStreamDelphi::Read(&CBufPtr->cxHeader, sizeof(TCompressHeader));
            if(RLen < sizeof(TCompressHeader)) NrLoaded = 0;
            else {
                uint16_t WLen = (CBufPtr->cxHeader.cxB1 << 8) + CBufPtr->cxHeader.cxB2;
                if(!CBufPtr->cxHeader.cxTyp) NrLoaded = TXFileStreamDelphi::Read(BufPtr.data(), WLen);
                else {
                    TXFileStreamDelphi::Read(&CBufPtr->cxData, WLen);
                    unsigned long XLen = BufSize; // we need a var parameter
                    uncompress(BufPtr.data(), &XLen, &CBufPtr->cxData, WLen);
                    NrLoaded = XLen;
                }
            }
        }
        NrRead = NrWritten = 0;
        return NrLoaded > 0;
    }

    int64_t TBufferedFileStreamDelphi::GetPosition() {
        if(!NrWritten) return PhysPosition - NrLoaded + NrRead;
        if(FCompress) FlushBuffer();
        return PhysPosition + NrWritten;
    }

    void TBufferedFileStreamDelphi::SetPosition(int64_t p) {
        if(NrWritten > 0) {
            if(p == PhysPosition + NrWritten && !FCompress) return;
            FlushBuffer();
        }
        if(NrLoaded > 0 && !FCompress) {
            int64_t StartOfBuf {PhysPosition-NrLoaded};
            if(p >= StartOfBuf && p < PhysPosition) {
                NrRead = (uint32_t)(p - StartOfBuf);
                return;
            }
        }
        TXFileStreamDelphi::SetPosition(p);
        NrLoaded = NrRead = 0;
    }

    int64_t TBufferedFileStreamDelphi::GetSize() {
        int64_t res {TXFileStreamDelphi::GetSize()};
        if(NrWritten > 0) res = std::max(res, (int64_t)(PhysPosition + NrWritten));
        return res;
    }

    TBufferedFileStreamDelphi::TBufferedFileStreamDelphi(const std::string& FileName, uint16_t Mode)
        : TXFileStreamDelphi{ FileName, (FileAccessMode)Mode },
        NrLoaded{},
        NrRead{},
        NrWritten{},
        BufSize{ BufferSize },
        CBufSize{ (uint32_t)std::round((double)BufferSize * 12.0 / 10.0) + 20 },
        BufPtr(BufferSize),
        CBufPtr{ (PCompressBuffer)malloc(sizeof(TCompressHeader)+CBufSize) },
        FCompress{},
        FCanCompress{true} // no longer a fatal error
    {
    }

    TBufferedFileStreamDelphi::~TBufferedFileStreamDelphi() {
        if(NrWritten > 0)
            FlushBuffer();
        free(CBufPtr);
    }

    bool TBufferedFileStreamDelphi::FlushBuffer()
    {
        bool res{true};
        uint32_t ActWritten;
        if(!NrWritten) return res;
        if(!FCompress || !FCanCompress) {
            ActWritten = TXFileStreamDelphi::Write(BufPtr.data(), NrWritten);
            res = NrWritten == ActWritten;
        } else {
            unsigned long Len = CBufSize - sizeof(TCompressHeader);
            compress(&CBufPtr->cxData, &Len, BufPtr.data(), NrWritten);
            if(Len < NrWritten) {
                CBufPtr->cxHeader.cxTyp = 1; // indicates compressed
                CBufPtr->cxHeader.cxB1 = (uint8_t)(Len >> 8);
                CBufPtr->cxHeader.cxB2 = Len & 0xFF;
                Len += sizeof(TCompressHeader);
                ActWritten = TXFileStreamDelphi::Write(&CBufPtr->cxHeader.cxTyp, Len);
                res = Len == ActWritten;
            } else {
                CBufPtr->cxHeader.cxTyp = 0; // indicates no compression
                CBufPtr->cxHeader.cxB1 = NrWritten >> 8;
                CBufPtr->cxHeader.cxB2 = NrWritten & 0xFF;
                TXFileStreamDelphi::Write(&CBufPtr->cxHeader.cxTyp, sizeof(TCompressHeader));
                ActWritten = TXFileStreamDelphi::Write(BufPtr.data(), NrWritten);
                res = NrWritten == ActWritten;
            }
        }
        NrWritten = NrLoaded = NrRead = 0;
        return res;
    }

    uint32_t TBufferedFileStreamDelphi::Read(void* Buffer, uint32_t Count)
    {
        if(NrWritten > 0) FlushBuffer();
        if(Count <= NrLoaded - NrRead) {
            memcpy(Buffer, &BufPtr[NrRead], Count);
            NrRead += Count;
            return Count;
        } else {
            char *UsrPtr = static_cast<char *>(Buffer);
            uint32_t UsrReadCnt = 0;
            while(Count > 0) {
                if(NrRead >= NrLoaded && !FillBuffer()) break;
                uint32_t NrBytes = std::min(Count, NrLoaded - NrRead);
                memcpy(&UsrPtr[UsrReadCnt], &BufPtr[NrRead], NrBytes);
                NrRead += NrBytes;
                UsrReadCnt += NrBytes;
                Count -= NrBytes;
            }
            return UsrReadCnt;
        }
    }

    char TBufferedFileStreamDelphi::ReadCharacter()
    {
        const char substChar = 0x1A; // 26
        if(NrWritten > 0) FlushBuffer();
        if(NrRead >= NrLoaded && !FillBuffer()) return substChar;
        return (char)BufPtr[NrRead++];
    }

    uint32_t TBufferedFileStreamDelphi::Write(const void* Buffer, uint32_t Count)
    {
        if(NrLoaded > 0) { // we have been reading ahead
            TXFileStreamDelphi::SetPosition(PhysPosition - NrLoaded + NrRead);
            NrLoaded = NrRead = 0;
        }
        if(Count <= BufSize - NrWritten) { // the simple case
            memcpy(&BufPtr[NrWritten], Buffer, Count);
            NrWritten += Count;
            return Count;
        }
        else {
            const char *UsrPtr = (const char *)Buffer;
            int UsrWriteCnt{}; // total number of bytes written
            while(Count > 0) {
                auto NrBytes = std::min(Count, BufSize - NrWritten);
                if(NrBytes > 0)
                    memcpy(&BufPtr[NrWritten], &UsrPtr[UsrWriteCnt], NrBytes);
                NrWritten += NrBytes;
                UsrWriteCnt += (int)NrBytes;
                Count -= NrBytes;
                if(NrWritten >= BufSize && !FlushBuffer()) break;
            }
            return UsrWriteCnt;
        }
    }

    bool TBufferedFileStreamDelphi::IsEof() {
        return NrRead >= NrLoaded && GetPosition() >= GetSize();
    }

    void TBufferedFileStreamDelphi::SetCompression(bool V)
    {
        if((FCompress || V) && NrWritten > 0) FlushBuffer();
        if(FCompress != V)
            NrLoaded = NrRead = 0;
        FCompress = V;
    }

    bool TBufferedFileStreamDelphi::GetCompression() const { return FCompress; }

    bool TBufferedFileStreamDelphi::GetCanCompress() const { return FCanCompress; }


    void TMiBufferedStreamDelphi::DetermineByteOrder()
    {
        initOrderCommon<uint16_t>(order_word, size_word, PAT_WORD);
        initOrderCommon<int>(order_integer, size_integer, PAT_INTEGER);
        initOrderCommon<double>(order_double, size_double, PAT_DOUBLE);
    }

    TMiBufferedStreamDelphi::TMiBufferedStreamDelphi(const std::string& FileName, uint16_t Mode) : TBufferedFileStreamDelphi{FileName, Mode}
    {
        if(FLastIOResult) return;
        if(Mode != fmCreate) DetermineByteOrder(); // we cannot update a mixed environment file!
        else { // avoid using writebyte so Paranoid flag works
            uint8_t B = sizeof(uint16_t); Write(&B, sizeof(uint8_t));
            uint16_t W = PAT_WORD; Write(&W, sizeof(uint16_t));
            B = sizeof(int); Write(&B, sizeof(uint8_t));
            int I = PAT_INTEGER; Write(&I, sizeof(int));
            B = sizeof(double); Write(&B, sizeof(uint8_t));
            double D = PAT_DOUBLE; Write(&D, sizeof(double));
        }
        TDoubleVar X{};
        X.V = 1.0;
        NormalOrder = !X.VA.front();
    }

    //note: this only works when src and dest point to different areas
    void TMiBufferedStreamDelphi::ReverseBytes(void* psrc, void* pdest, int sz)
    {
        char *pdestc = (char *)pdest;
        char *psrcc = (char *)psrc;
        pdestc += sz-1;
        for(int k{0}; k<sz; k++) {
            *pdestc = *psrcc;
            psrcc++;
            pdestc--;
        }
    }

    int TMiBufferedStreamDelphi::GoodByteOrder() const
    {
        int res{};
        if(order_word == PAT_BAD_SIZE) res += 1;
        if(order_word == PAT_BAD_ORDER) res += 2;
        if(order_integer == PAT_BAD_SIZE) res += 4;
        if(order_integer == PAT_BAD_ORDER) res += 8;
        if(order_double == PAT_BAD_SIZE) res += 16;
        if(order_double == PAT_BAD_ORDER) res += 32;
        return res;
    }

    double TMiBufferedStreamDelphi::ReadDouble()
    {
        return ReadValueOrdered<double>(rw_double, order_double);
    }

    int TMiBufferedStreamDelphi::ReadInteger()
    {
        return ReadValueOrdered<int>(rw_integer, order_integer);
    }

    uint16_t TMiBufferedStreamDelphi::ReadWord()
    {
        return ReadValueOrdered<uint16_t>(rw_word, order_word);
    }

    int64_t TMiBufferedStreamDelphi::ReadInt64()
    {
        return ReadValueOrdered<int64_t>(rw_int64, order_integer);
    }

    bool TMiBufferedStreamDelphi::WordsNeedFlip() const
    {
        return order_word;
    }

    bool TMiBufferedStreamDelphi::IntsNeedFlip() const
    {
        return order_integer;
    }

    void TMiBufferedStreamDelphi::WriteGmsInteger(int N) {        
        uint8_t B;
        if(N >= 0) B = 0;
        else {
            B = 128;
            N *= -1;
        }
        B |= (N & 15);
        N >>= 4;
        int C{};
        std::array<uint8_t, 5> W{};
        while(N) {
            W[++C] = N & 255;
            N >>= 8;
        }
        W[0] = B | (C << 4);
        Write(W.data(), C+1);
    }

    enum tgmsvalue { xvreal, xvund, xvna, xvpin, xvmin, xveps, xvacr };
    static tgmsvalue mapval(double x) {
        if (x < GMS_SV_UNDEF) return xvreal;
        if (x >= GMS_SV_ACR) return xvacr;
        x /= GMS_SV_UNDEF;
        int k = static_cast<int>(std::round(x));
        if (std::abs(k - x) > 1.0e-5)
            return xvund;
        constexpr std::array<tgmsvalue, 5> kToRetMapping = {
                xvund, xvna, xvpin, xvmin, xveps
        };
        return k >= 1 && k <= (int)kToRetMapping.size() ? kToRetMapping[k-1] : xvacr;
    }

    void TMiBufferedStreamDelphi::WriteGmsDouble(double D)
    {
        tgmsvalue gv = mapval(D);        
        uint8_t B = gv;
        if(gv == xvreal) {
            if(D == 0.0) B = 7;
            else if(D == 1.0) B = 8;
            else if(D == -1.0) B = 9;
        }
        if(B) {
            Write(&B, 1);
            if(gv == xvacr) WriteGmsInteger((int)std::round(D/GMS_SV_ACR));
            return;
        }
        int C{};
        TDoubleVar Z{};
        Z.V = D;
        if(NormalOrder) {
            for(auto &cell : Z.VA) {
                if(!cell) C++;
                else break;
            }
            B = 128 | C;
            Write(&B, 1);
            Write(&Z.VA[C+1], (uint32_t)Z.VA.size()-C);
        } else {
            for(int i{(int)Z.VA.size()-1}; i>=0; i--) {
                if(!Z.VA[i]) C++;
                else break;
            }
            B = 128 | C;
            Write(&B, 1);
            for(int i=8-C-1; i>=0; i--)
                Write(&Z.VA[i], 1);
        }
    }

    int TMiBufferedStreamDelphi::ReadGmsInteger()
    {
        uint8_t B;
        Read(&B, 1);
        std::array<uint8_t, 5> W{};
        W[0] = B & 15;
        bool Neg = B >= 128;
        global::delphitypes::Bounded<int, 0, 6> C {(B >> 4) & 7};
        if(C > 0) Read(&W[1], C);
        int res{};
        while(C >= 1) {
            res = (res << 8) | W[C];
            C--;
        }
        res = (res << 4) | W[0];
        if(Neg) res *= -1;
        return res;
    }

    double TMiBufferedStreamDelphi::ReadGmsDouble() {
        const static std::array<double, 9> bToRes { GMS_SV_UNDEF, GMS_SV_NA, GMS_SV_PINF, GMS_SV_MINF, GMS_SV_EPS, GMS_SV_ACR, 0.0, 1.0, -1.0};
        auto B {ReadByte()};
        if(!(B & 128)) return B >= 1 && B <= 9 ? (B == 6 ? ReadGmsInteger() : 1.0) * bToRes[B] : 0.0;
        TDoubleVar Z{};
        auto C = B & 127;
        if(NormalOrder) {
            for(auto &cell : Z.VA) {
                if(!C) cell = ReadByte();
                else {
                    cell = 0;
                    C--;
                }
            }
        } else {
            for(int i{(int)Z.VA.size()-1}; i>=0; i--) {
                if(!C) Z.VA[i] = ReadByte();
                else {
                    Z.VA[i] = 0;
                    C--;
                }
            }
        }
        return Z.V;
    }

    TBinaryTextFileIODelphi::TBinaryTextFileIODelphi(const std::string &fn, const std::string &PassWord, int &ErrNr, std::string &errMsg)
      : frw {fm_read}
    {
        FS = std::make_unique<TBufferedFileStreamDelphi>(fn, fmOpenRead);
        ErrNr = FS->GetLastIOResult();
        if (ErrNr) {
            errMsg = rtl::sysutils_p3::SysErrorMessage(ErrNr);
            ErrNr = strmErrorIOResult;
            return;
        }
        auto B1 = FS->ReadByte(), B2 = FS->ReadByte();
        if (B1 == 31 && B2 == 139) { //header for gzip
            //assume it is GZIP format
            FFileSignature = fsign_gzip;
            FS = nullptr;
            gzFS = std::make_unique<TGZipInputStream>(fn, errMsg);
            if (!errMsg.empty()) ErrNr = 1;
            return;
        }

        std::string srcBuf;
        srcBuf.resize(B2);
        if (B1 == signature_header) Read(srcBuf.data(), B2);
        if (B1 != signature_header || srcBuf != signature_gams) { // nothing special
            utils::tBomIndic fileStart {B1, B2, FS->ReadByte(), FS->ReadByte()};
            int BOMOffset;
            if(!utils::checkBOMOffset(fileStart, BOMOffset, errMsg)) {
                ErrNr = strmErrorEncoding;
                return;
            }
            FS->SetPosition(BOMOffset);
            FRewindPoint = BOMOffset;
            FMajorVersionRead = 0;
            FMinorVersionRead = 0;
            FFileSignature = fsign_text;
            errMsg.clear();
            return;
        }
        ErrNr = strmErrorGAMSHeader;
        errMsg = "GAMS header not found";
        FFileSignature = static_cast<TFileSignature>(FS->ReadByte() - 'A');
        FS->ReadString();
        FMajorVersionRead = FS->ReadByte();
        FMinorVersionRead = FS->ReadByte();
        char Ch { (char)FS->ReadByte() };

        bool hasPswd;
        if(Ch == 'P') hasPswd = true;
        else if(Ch == 'p') hasPswd = false;
        else return;

        Ch = (char)FS->ReadByte();
        bool hasComp;
        if(Ch == 'C') hasComp = true;
        else if(Ch == 'c') hasComp = false;
        else return;

        if(hasPswd && PassWord.empty()) {
            ErrNr = strmErrorNoPassWord;
            errMsg = "A Password is required";
            return;
        }

        ErrNr = strmErrorIntegrity;
        errMsg = "Integrity check failed";

        if(hasPswd) {
            FS->SetPassWord(PassWord);
            std::string src = FS->ReadString();
            std::array<char, 256> targBuf{};
            FS->ApplyPassWord(src.c_str(), targBuf.data(), (int)src.length(), verify_offset);
            if(FS->RandString((int)src.length()) != targBuf.data()) return;
        }

        FRewindPoint = FS->GetPosition();
        FS->SetCompression(true);
        FS->SetPosition(FRewindPoint);
        if(!hasComp) FS->SetCompression(false);
        if(FS->ReadString() != signature_gams) return;
        ErrNr = strmErrorNoError;
        errMsg.clear();
    }

    TBinaryTextFileIODelphi::TBinaryTextFileIODelphi(const std::string& fn, const std::string& Producer, const std::string& PassWord, TFileSignature signature, bool comp, int& ErrNr, std::string& errMsg)
        : FMajorVersionRead{}, FMinorVersionRead{}, FRewindPoint{} {
        FFileSignature = signature;
        frw = fm_write;
        FS = std::make_unique<TBufferedFileStreamDelphi>(fn, fmCreate);
        if (signature != fsign_text || !PassWord.empty() || comp) {
            FS->WriteByte(signature_header);
            FS->WriteString(signature_gams);
            FS->WriteByte(signature + 'A');
            FS->WriteString(Producer);
            FS->WriteByte(1); // version
            FS->WriteByte(1); // sub-version
            FS->WriteByte(PassWord.empty() ? 'p' : 'P');
            FS->WriteByte(comp ? 'C' : 'c');
            if(!PassWord.empty()) {
                FS->FlushBuffer();
                FS->SetPassWord(PassWord);
                std::string src = FS->RandString((int)PassWord.length());
                std::array<char, 256> targBuf{};
                FS->ApplyPassWord(src.c_str(), targBuf.data(), (int)src.length(), verify_offset);
                FS->SetPassWord("");
                FS->WriteString(targBuf.data());
            }
            if(comp) FS->SetCompression(true);
            else FS->FlushBuffer();
            FS->SetPassWord(PassWord);
            //write a few bytes to be recognized later (compression / password is now active)
            FS->WriteString(signature_gams);
        }
        ErrNr = FS->GetLastIOResult();
        if(!ErrNr) {
            ErrNr = strmErrorNoError;
            errMsg.clear();
        } else {
            errMsg = rtl::sysutils_p3::SysErrorMessage(ErrNr);
            FS = nullptr;
        }
    }

    int TBinaryTextFileIODelphi::Read(char* Buffer, int Count) {
        return FFileSignature == fsign_gzip ? (int)gzFS->Read(Buffer, Count) : (int)FS->Read(Buffer, Count);
    }

    char TBinaryTextFileIODelphi::ReadCharacter() {
        const char substChar = 0x1A;
        if (FFileSignature == fsign_gzip) {
            char res;
            if (!gzFS->Read(&res, 1)) return substChar;
            return res;
        }
        else return FS->ReadCharacter();
    }

    void TBinaryTextFileIODelphi::ReadLine(std::vector<uint8_t> &Buffer, int MaxInp, char& LastChar) {
        const char substChar = 0x1A;
        // moved here for performance reasons
        // reading a single byte at a time is avoided this way
        if (FFileSignature == fsign_gzip)
            gzFS->ReadLine(Buffer, MaxInp, LastChar);
        else {
            Buffer.clear();
            while (!(utils::in(LastChar, substChar, '\n', '\r') || (int)Buffer.size() == MaxInp)) {
                Buffer.push_back(LastChar);
                if (FS->NrLoaded - FS->NrRead >= 1) { // the simple case
                    LastChar = (char)FS->BufPtr[FS->NrRead];
                    FS->NrRead++;
                }
                // we should we fill the buffer???
                else if (FS->Read(&LastChar, 1) <= 0) LastChar = substChar;
            }
        }
    }

    int TBinaryTextFileIODelphi::Write(const char* Buffer, int Count) {
        assert(frw == fm_write && "TBinaryTextFileIO.Read");
        return static_cast<int>(!FS ? -1 : FS->Write(Buffer, Count));
    }

    bool TBinaryTextFileIODelphi::UsesPassWord() {
        return FS && FS->GetUsesPassWord();
    }

    void TBinaryTextFileIODelphi::ReWind() {
        assert(frw == fm_read && "TBinaryTextFileIO.ReWind1");
        assert(FS && "TBinaryTextFileIO.ReWind2");
        FS->SetPosition(FRewindPoint);
        if (FS->GetCompression()) FS->ReadString(); // skip verification string
    }

    int TBinaryTextFileIODelphi::GetLastIOResult() {
        return FS->GetLastIOResult();
    }
}
