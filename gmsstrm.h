#pragma once

// Look at the gmsstrm.cpp file to get some comments on why there are *Delphi classes.

#include <map>
#include <fstream>
#include <memory>
#include <vector>
#include <array>
#include <optional>
#include <cstring>

#include "zlib/zlib.h"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::gmsstrm {
    std::string SysErrorMessage(int errorCore);

    const bool Paranoid = false;

    const int
        // TXStream seek origins
        soFromBeginning  = 0,
        soFromCurrent    = 1,
        soFromEnd        = 2,

        BufferSize = 32 * 1024,    //internal maximum for compression

        strmErrorNoError    = 0,
        strmErrorIOResult   = 1,
        strmErrorGAMSHeader = 2,
        strmErrorNoPassWord = 3,
        strmErrorIntegrity  = 4,
        strmErrorZLib       = 5,
        strmErrorEncoding = 6;
    
    const uint16_t PAT_WORD = 0x1234u;
    const /*uint32_t*/ int PAT_INTEGER = 0x12345678 /*u*/;
    const double PAT_DOUBLE = 3.1415926535897932385;
    const int PAT_BAD_ORDER = 254;
    const int PAT_BAD_SIZE = 255;

    // File Mode creation constants
    enum FileAccessMode {
        fmCreate = 0xFFFF,
        fmOpenRead = 0x0000,
        fmOpenWrite = 0x001,
        fmOpenReadWrite = 0x002
    };

    // File Mode creation constants
    const std::map<FileAccessMode, std::string> modeStrs = {
        {fmCreate, "w"},
        {fmOpenRead, "r"},
        {fmOpenWrite, "w"},
        {fmOpenReadWrite, "w+"}
    };
    
    enum RWType {rw_byte, rw_bool, rw_char, rw_word, rw_integer, rw_int64, rw_double, rw_string, rw_pchar, rw_pstring, rw_count};
    const std::array<std::string, 10> RWTypeText = { "Byte", "Bool", "Char", "Word", "Integer", "Int64", "Double", "String", "PChar", "PString" };

    class TXStream {
    public:
        virtual size_t Write(const char *buf, size_t count) = 0;
        virtual size_t Read(char *buf, size_t count) const = 0;

        virtual void WriteInteger(int N) = 0;
        virtual void WriteString(const std::string& s) = 0;
        virtual void WritePChar(const std::string& s) = 0;

        virtual void WriteByte(uint8_t b) = 0;
        virtual void WriteBool(bool B) = 0;

        virtual std::string ReadString() const = 0;
        virtual int ReadInteger() const = 0;
        virtual uint8_t ReadByte() const = 0;
        virtual uint16_t ReadWord() const = 0;
        virtual bool ReadBool() const = 0;

        virtual uint64_t GetPosition() const = 0;
        virtual void SetPosition(int P) = 0;
    };

    class TMiBufferedFileStream : public TXStream {
    public:
        virtual bool WordsNeedFlip() const = 0;
    };

    /**
     * Defines the base class for a stream. Only to be used for defining derived objects.
     */
    class TXStreamDelphi {
        std::unique_ptr<std::ofstream> fstext {};
    protected:
        void ParWrite(RWType T);
        void ParCheck(RWType T);

        template<typename T>
        void WriteValue(RWType rwt, T &v) {
            if(Paranoid) ParWrite(rwt);
            Write(&v, sizeof(T));
        }

        template<typename T>
        T ReadValue(RWType rwt) {
            if(Paranoid) ParCheck(rwt);
            T res;
            Read(&res, sizeof(T));
            return res;
        }
    
        virtual int64_t GetPosition() = 0;
        virtual void SetPosition(int64_t P) = 0;
        virtual int64_t GetSize() = 0;

    public:
        virtual uint32_t Read(void *Buffer, uint32_t Count) = 0;
        virtual uint32_t Write(const void *Buffer, uint32_t Count) = 0;

        void WriteString(std::string_view s);

        inline void WriteString(const char *s) {
            WriteString(std::string_view{s, std::strlen(s)});
        }

        void WriteDouble(double x);
        void WriteInteger(int n);
        void WriteInt64(int64_t N);
        void WriteByte(uint8_t b);
        void WriteWord(uint16_t W);
        void WriteBool(bool B);
        void WriteChar(char C);
        void WritePChar(const char *s, int L);

        std::string ReadString();
        virtual double ReadDouble();
        virtual int ReadInteger();
        uint8_t  ReadByte();
        virtual uint16_t ReadWord();
        virtual int64_t ReadInt64();
        bool ReadBool();
        char ReadChar();
        void ReadPChar(char *P, int &L);

        void ActiveWriteOpTextDumping(const std::string &dumpFilename);
    };

    class TXFileStreamDelphi : public TXStreamDelphi {
        friend class TBinaryTextFileIODelphi;

        std::unique_ptr<std::fstream> FS{};
        bool FileIsOpen{};
        std::string FFileName{}, FPassWord{};

        static std::string RandString(int L);

    protected:
        int FLastIOResult{};
        int64_t PhysPosition{};

        int64_t GetSize() override;
        int64_t GetPosition() override;
        void SetPosition(int64_t P) override;

    public:
        TXFileStreamDelphi(std::string AFileName, FileAccessMode AMode);
        virtual ~TXFileStreamDelphi();

        void ApplyPassWord(const char *PR, char *PW, int Len, int64_t Offs);
        uint32_t Read(void *Buffer, uint32_t Count) override;
        uint32_t Write(const void *Buffer, uint32_t Count) override;

        void SetLastIOResult(int V);
        int GetLastIOResult();

        void SetPassWord(const std::string& s);
        bool GetUsesPassWord();

        std::string GetFileName() const;
    };

    struct TCompressHeader {
        uint8_t cxTyp; // 0=no compression, 1=zlib
        uint8_t cxB1, cxB2;
    };

    struct TCompressBuffer {
        TCompressHeader cxHeader;
        uint8_t cxData;
    };
    using PCompressBuffer = TCompressBuffer*;

    class TBufferedFileStreamDelphi : public TXFileStreamDelphi {
        friend class TBinaryTextFileIODelphi;

        uint32_t NrLoaded, NrRead, NrWritten, BufSize, CBufSize;

        std::vector<uint8_t> BufPtr;
        PCompressBuffer CBufPtr;

        bool FCompress, FCanCompress;

        bool FillBuffer();
        
    protected:
        int64_t GetSize() override;
    public:
        TBufferedFileStreamDelphi(const std::string &FileName, uint16_t Mode);
        ~TBufferedFileStreamDelphi() override;
        bool FlushBuffer();
        uint32_t Read(void *Buffer, uint32_t Count) override;
        char ReadCharacter();
        uint32_t Write(const void *Buffer, uint32_t Count) override;
        bool IsEof();
        bool GetCompression() const;
        void SetCompression(bool V);
        bool GetCanCompress() const;

        int64_t GetPosition() override;

        void SetPosition(int64_t p) override;
    };

    void reverseBytesMax8(void *psrc, void *pdest, int sz);

    class TMiBufferedStreamDelphi : public TBufferedFileStreamDelphi {
        uint8_t order_word{}, order_integer{}, order_double{}, size_word{}, size_integer{}, size_double{};
        bool NormalOrder{};

        template<typename T>
        T ReadValueOrdered(RWType rwt, bool order_type) {
            if(Paranoid) ParCheck(rwt);
            T res;
            if(!order_type) Read(&res, sizeof(T));
            else {
                T tmp;
                Read(&tmp, sizeof(T));
                reverseBytesMax8(&tmp, &res, sizeof(T));
            }
            return res;
        }

        template<typename T>
        void initOrderCommon(uint8_t &order_type, uint8_t &size_type, T patConstant) {
            T v1, v2;
            Read(&size_type, sizeof(uint8_t));
            if(size_type != sizeof(T)) {
                order_type = PAT_BAD_SIZE;
                SetPosition(GetPosition() + size_type);
            } else {
                Read(&v1, sizeof(T));
                order_type = 0;
                if(v1 != patConstant) {
                    order_type = 1;
                    reverseBytesMax8(&v1, &v2, sizeof(T));
                    if(v2 != patConstant) order_type = PAT_BAD_ORDER;
                }
            }
        };

        void DetermineByteOrder();
    public:
        TMiBufferedStreamDelphi(const std::string &FileName, uint16_t Mode);
        static void ReverseBytes(void *psrc, void *pdest, int sz);
        int GoodByteOrder() const;
        double ReadDouble() override;
        int ReadInteger() override;
        uint16_t ReadWord() override;
        int64_t ReadInt64() override;
        bool WordsNeedFlip() const;
        bool IntsNeedFlip() const;
        void WriteGmsInteger(int N);
        void WriteGmsDouble(double D);
        int ReadGmsInteger();
        double ReadGmsDouble();
    };

    enum TFileSignature {
        fsign_text,
        fsign_blocktext,
        fsign_gzip
    };

    class TGZipInputStream {
        gzFile pgz;
        std::vector<uint8_t> Buf;
        unsigned NrLoaded, NrRead;

    public:
        TGZipInputStream(const std::string& fn, std::string& ErrMsg);
        virtual ~TGZipInputStream();

        unsigned Read(void *buffer, unsigned int Count);

        void ReadLine(std::vector<uint8_t> &buffer, int MaxInp, char &LastChar);
    };

    class TBinaryTextFileIODelphi {
        std::unique_ptr<TBufferedFileStreamDelphi> FS;
        std::unique_ptr<TGZipInputStream> gzFS;
        enum {fm_read, fm_write} frw;
        TFileSignature FFileSignature;
        uint8_t FMajorVersionRead, FMinorVersionRead;
        int64_t FRewindPoint;
    public:
        // OpenForRead
        TBinaryTextFileIODelphi(const std::string &fn, const std::string &PassWord, int &ErrNr, std::string &errMsg);
        // OpenForWrite
        TBinaryTextFileIODelphi(const std::string &fn, const std::string &Producer, const std::string &PassWord, TFileSignature signature, bool comp, int &ErrNr, std::string &errMsg);

        int Read(char *Buffer, int Count);
        char ReadCharacter();
        void ReadLine(std::vector<uint8_t> &Buffer, int MaxInp, char &LastChar);
        int Write(const char *Buffer, int Count);
        bool UsesPassWord();
        void ReWind();
        int GetLastIOResult();
    };

    class TBinaryTextFileIO {
        std::unique_ptr<std::iostream> FS;
        std::unique_ptr<TGZipInputStream> gzFS;
        enum {
            fm_read,
            fm_write
        } frw;
        TFileSignature FFileSignature{};
        uint8_t  FMajorVersionRead{}, FMinorVersionRead{};
        int FRewindPoint{};

        int NrLoaded{}, NrRead, NrWritten;

        int FLastIOResult{};

        bool FCanCompress, FCompress{};
        std::string FPassword{};

        const bool noBuffering {false};
        std::array<char, BufferSize> readBuffer{};
        uint64_t lastReadCount{};
        std::optional<uint64_t> offsetInBuffer{};
        void maybeFillReadBuffer();

        int GetLastIOResult() const;

        static std::string RandString(int L) ;

        void SetCompression(bool V);

        // Read/write from/to string buffer that is filled with a copy of contents
        explicit TBinaryTextFileIO(const std::string &contents, int &ErrNr);
    public:
        static TBinaryTextFileIO *FromString(const std::string &contents, int &ErrNr);

        // Read/write from/to actual file
        TBinaryTextFileIO(const std::string &fn, const std::string &PassWord, int &ErrNr, std::string &errmsg);
        TBinaryTextFileIO(const std::string &fn, const std::string &Producer, const std::string &PassWord, TFileSignature signature, bool comp, int &ErrNr, std::string &errmsg);
        ~TBinaryTextFileIO();

        int Read(char *Buffer, int Count/*, bool WithoutMoving = false*/);
        char ReadCharacter();
        void ReadLine(std::string &Buffer, int &Len, char &LastChar);
        int Write(const char *Buffer, int Count);
        bool UsesPassWord();
        void ReWind();

        //RProp(TBinaryTextFileIO, int, LastIOResult, GetLastIOResult);

        uint8_t ReadByte();
        char ReadChar();
        void ParCheck(RWType T);
        std::string ReadString();

        void WriteByte(uint8_t B);
        void ParWrite(RWType T);
        void WriteString(std::string_view s);

        void SetPassword(const std::string &s);
        void ApplyPassword(std::string &PR, std::string &PW, int64_t Offs);
    };

    void CompressTextFile(const std::string& fn, const std::string& fo, const std::string& PassWord, bool Comp, int& ErrNr, std::string& ErrMsg);
    void UnCompressTextFile(const std::string& fn, const std::string& fo, const std::string& PassWord, int& ErrNr, std::string& ErrMsg);
}
