/*
 * GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2023 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2023 GAMS Development Corp. <support@gams.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "gmsstrm.h"

#ifdef GAMSBUILD
#include <palmcc.h>
#endif

#include "gdx.h"
#include "utils.h"  // for assignPCharToBuf, in, trimRight, assignStr...
#include <algorithm>// for fill_n, max, fill, sort
#include <cassert>  // for assert
#include <cmath>    // for round, isinf, isnan, isnormal, abs
#include <cstdint>  // for int64_t, uint8_t
#include <cstdio>   // for sprintf
#include <cstdlib>  // for system, free, getenv, malloc, realloc, WEX...
#include <exception>// for exception
#include <iostream> // for operator<<, basic_ostream, cout, ostream
#include <map>      // for map, operator==, _Rb_tree_const_iterator
#include <utility>  // for pair
#include <vector>   // for vector

#if defined( _WIN32 )
#include <Windows.h>
#undef max
#undef GetObject
#endif

using namespace gdx::gmsstrm;
using namespace std::literals::string_literals;

namespace gdx
{

std::string QueryEnvironmentVariable( const std::string &Name );
std::string QueryEnvironmentVariable( const std::string &Name )
{
#if defined( _WIN32 )
   int len = GetEnvironmentVariableA( Name.c_str(), nullptr, 0 );
   if( !len ) return ""s;
   else
   {
      auto buf { std::unique_ptr<char[]>{ new char[len] } };
      GetEnvironmentVariableA( Name.c_str(), buf.get(), len );
      std::string val(buf.get(), len-1); // no terminating zero
      if( val.length() > 255 ) val = val.substr( 0, 255 );
      return val;
   }
#else
   const char *s = std::getenv( Name.c_str() );
   std::string sout = s == nullptr ? ""s : s;
   if( sout.length() > 255 ) sout = sout.substr( 0, 255 );
   return sout;
#endif
}

int64_t dblToI64( double x );

bool CanBeQuoted( const char *s )
{
   if( !s ) return false;
   bool saw_single{}, saw_double{};
   for( int i{}; true; i++ )
   {
      char Ch = s[i];
      if( s[i] == '\0' ) return true;
      if( Ch == '\'' )
      {
         if( saw_double ) return false;
         saw_single = true;
      }
      else if( Ch == '\"' )
      {
         if( saw_single ) return false;
         saw_double = true;
      }
      else if( static_cast<unsigned char>( Ch ) < ' ' )
         return false;
   }
   return true;
}

bool GoodUELString( const char *s, size_t slen )
{
   return slen < GLOBAL_UEL_IDENT_SIZE && CanBeQuoted( s );
}

const int MaxDimV148 = 10;
using TIndex = std::array<int, GLOBAL_MAX_INDEX_DIM>;

#ifdef GAMSBUILD
static std::string gdlSetSystemName()
{
   palHandle_t pal;
   std::array<char, 256> msg{};
   if( !palCreate( &pal, msg.data(), static_cast<int>( msg.size() ) ) )
      printf( "error" );
   palSetSystemName( pal, "GDX Library" );
   palGetAuditLine( pal, msg.data() );
   palFree( &pal );
   return msg.data();
}
static const std::string auditLine{ gdlSetSystemName() };
#else
static const std::string auditLine{ "GDX Library      00.0.0 ffffffff May  4, 1970  (AUDIT) XYZ arch xybit/myOS" };
#endif

using UELTableImplChoice = TUELTable;

//version = 5 has 32 bit offsets and no compression
//version = 6 introduced compression
//version 7 introduces:
//      acronyms
//      longer symbol names
//      aliases
//later without bumping version
//      relaxed domains
const int VERSION = 7,    //--file version
        gdxHeaderNr = 123;//--patterns to recognize
const std::string gdxHeaderId = "GAMSGDX";

const int MARK_BOI = 19510624;

const std::string
        MARK_UEL = "_UEL_"s,
        MARK_SYMB = "_SYMB_"s,
        MARK_DATA = "_DATA_"s,
        MARK_SETT = "_SETT_"s,
        MARK_ACRO = "_ACRO_"s,
        MARK_DOMS = "_DOMS_"s;

const int INDEX_INITIAL = -256;

const std::array fmode_str{
        "FileNotOpen"s, //f_not_open
        "ReadCommand"s, //fr_init
        "WriteCommand"s,//fw_init
        "Write-Dom-Raw"s,
        "Write-Dom-Map"s,
        "Write-Dom-Str"s,
        "Write-Raw"s,   //fw_raw_data
        "Write-Map"s,   //fw_Map_data
        "Write-Str"s,   //fw_str_data
        "Regis-Raw"s,   //f_raw_elem
        "Regis-Map"s,   //f_Map_elem
        "Regis-Str"s,   //f_str_elem
        "Read-Raw"s,    //fr_raw_data
        "Read-Map"s,    //fr_Map_data
        "Read_MapR"s,   //fr_MapR_data
        "Read-Str"s,    //fr_str_data
        "Regis-Filter"s,//fr_filter
        "Read-Slice"s   //fr_slice
};

std::string DLLLoadPath{};

const int
        ERR_NOERROR = 0,
        ERR_NOFILE = -100000,
        ERR_FILEERROR = -100001,
        ERR_BADMODE = -100002,
        ERR_BADDIMENSION = -100003,
        ERR_BADELEMENTINDEX = -100004,
        ERR_BADSYMBOLINDEX = -100005,
        ERR_ELEMENTSEQUENCE = -100006,
        ERR_DUPLICATESYMBOL = -100007,
        ERR_DATANOTSORTED = -100008,
        ERR_DATADUPLICATE = -100009,
        ERR_UNKNOWNFILTER = -100010,
        ERR_BADSTRINGFORMAT = -100011,
        ERR_BADIDENTFORMAT = -100012,
        ERR_UELCONFLICT = -100013,
        ERR_DUPLICATESPECVAL = -100014,
        ERR_BADERRORRECORD = -100015,
        ERR_DUPLICATEUEL = -100016,
        ERR_BADUELSTR = -100017,
        ERR_UNDEFUEL = -100018,
        ERR_UELSECONDWRITE = -100019,
        ERR_UELNOTEMPTY = -100020,
        ERR_BAD_FILTER_NR = -100021,
        ERR_BAD_FILTER_INDX = -100022,
        ERR_FILTER_UNMAPPED = -100023,
        ERR_OBSOLETE_FUNCTION = -100024,
        ERR_RAWNOTSORTED = -100025,
        ERR_BAD_ALIAS_DIM = -100026,
        ERR_BADDATAMARKER_DATA = -100029,
        ERR_BADDATAMARKER_DIM = -100030,
        ERR_OPEN_BOI = -100031,
        ERR_OPEN_FILEHEADER = -100032,
        ERR_OPEN_FILEVERSION = -100033,
        ERR_OPEN_FILEMARKER = -100034,
        ERR_OPEN_SYMBOLMARKER1 = -100035,
        ERR_OPEN_SYMBOLMARKER2 = -100036,
        ERR_OPEN_UELMARKER1 = -100037,
        ERR_OPEN_UELMARKER2 = -100038,
        ERR_OPEN_TEXTMARKER1 = -100039,
        ERR_OPEN_TEXTMARKER2 = -100040,
        ERR_BADDATAFORMAT = -100041,
        //    ERR_NEXT_ERROR           = -100042,
        ERR_OUT_OF_MEMORY = -100043,
        ERR_ZLIB_NOT_FOUND = -100044,//check with gmsstrm
        ERR_OPEN_ACROMARKER1 = -100045,
        ERR_OPEN_ACROMARKER2 = -100046,
        ERR_BADACROINDEX = -100047,
        ERR_BADACRONUMBER = -100048,
        ERR_BADACRONAME = -100049,
        ERR_ACRODUPEMAP = -100050,
        ERR_ACROBADADDITION = -100051,
        ERR_UNKNOWNDOMAIN = -100052,
        ERR_BADDOMAIN = -100053,
        ERR_NODOMAINDATA = -100054,
        ERR_ALIASSETEXPECTED = -100055,
        ERR_BADDATATYPE = -100056,
        ERR_NOSYMBOLFORCOMMENT = -100057,
        ERR_DOMAINVIOLATION = -100058,
        ERR_FILEALREADYOPEN = -100059,
        ERR_FILETOOLDFORAPPEND = -100060,
        ERR_OPEN_DOMSMARKER1 = -100061,
        ERR_OPEN_DOMSMARKER2 = -100062,
        ERR_OPEN_DOMSMARKER3 = -100063,

        //Errors from gdxcopy
        ERR_GDXCOPY = -100100;
const int
        ERR_PARAMETER = ERR_GDXCOPY - 1,
        ERR_DLL_NOT_FOUND = ERR_GDXCOPY - 2,
        ERR_CREATE_DIR = ERR_GDXCOPY - 3,
        ERR_FILE_OPEN = ERR_GDXCOPY - 4,
        ERR_FILE_WRITE = ERR_GDXCOPY - 5,
        ERR_UEL_LENGTH = ERR_GDXCOPY - 6,
        ERR_UEL_REGISTER = ERR_GDXCOPY - 7,
        ERR_EXPL_TEXT = ERR_GDXCOPY - 8,
        ERR_DIMENSION = ERR_GDXCOPY - 9,
        ERR_WRITE_SYMBOL = ERR_GDXCOPY - 10,
        ERR_CLOSE_FILE = ERR_GDXCOPY - 11,
        ERR_CANNOT_DELETE = ERR_GDXCOPY - 12,
        ERR_CANNOT_RENAME = ERR_GDXCOPY - 13;

int GetEnvCompressFlag();

#ifndef NO_GAMSDIST
static int SystemP( const std::string &cmd, int &ProgRC )
{
   int res{ std::system( cmd.c_str() ) };
#if defined( _WIN32 )
   ProgRC = 0;
#else
   if( WIFEXITED( res ) )
   {
      ProgRC = WEXITSTATUS( res );
      if( ProgRC == 127 )
      {
         ProgRC = 0;
         return 127;
      }
      if( ProgRC == 126 )
      {
         ProgRC = 0;
         return 126;
      }
   }
   else if( WIFSIGNALED( res ) )
   {
      ProgRC = WTERMSIG( res );
      return 1;
   }
   else
   {
      ProgRC = 0;
      return 2;
   }
#endif
   return res;
}
#endif

int ConvertGDXFile( const std::string &fn, const std::string &MyComp )
{
   std::string Conv{ utils::trim( utils::uppercase( QueryEnvironmentVariable( strGDXCONVERT ) ) ) };
   if( Conv.empty() ) Conv = "V7"s;
   std::string Comp = Conv == "V5" ? ""s : ( !GetEnvCompressFlag() ? "U" : "C" );
   if( utils::sameText( Conv + Comp, "V7"s + MyComp ) ) return 0;
#ifdef NO_GAMSDIST
   int progRC {}, res {};
#else
   int progRC, res { SystemP( "gdxcopy -"s + Conv + Comp + " -Replace "s + utils::quoteWhitespace( fn, '\"' ), progRC ) };
#endif
   return progRC ? ERR_GDXCOPY - progRC : res;
}

// If both single and double quotes appear in the string, replace
// each quote by the first quote seen
// Replace control character with a question mark
int MakeGoodExplText( char *s )
{
   if( !s ) return 0;
   char q{ '\0' };
   int i;
   for( i = 0; s[i] != '\0'; i++ )
   {
      if( !utils::in( s[i], '\"', '\'' ) )
      {
         if( s[i] < ' ' ) s[i] = '?';
      }
      else
      {
         if( q == '\0' ) q = s[i];
         s[i] = q;
      }
   }
   return i;
}

static inline bool isLetter( char c )
{
   return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' );
}

static inline bool isLetterDigitOrUnderscore( char c )
{
   return isLetter( c ) || ( c >= '0' && c <= '9' ) || ( c == '_' );
}

bool IsGoodIdent( const char *S )
{
   if( S[0] == '\0' || !isLetter( S[0] ) ) return false;
   int i;
   for( i = 1; S[i] != '\0'; i++ )
      if( !isLetterDigitOrUnderscore( S[i] ) ) return false;
   return i < GLOBAL_UEL_IDENT_SIZE;
}

static TgdxElemSize GetIntegerSize( int N )
{
   if( N <= 0 ) return TgdxElemSize::sz_integer;
   else if( N <= 255 )
      return TgdxElemSize::sz_byte;
   else if( N <= 65535 )
      return TgdxElemSize::sz_word;
   return TgdxElemSize::sz_integer;
}

union TI64Rec
{
   double x;
   int64_t i64;
};

// for double input x, return the bits as i64
int64_t dblToI64( double x )
{
   TI64Rec i64Rec{ x };
   return i64Rec.i64;
}

void copyIntlMapDblToI64( const TIntlValueMapDbl &dMap, TIntlValueMapI64 &iMap );

void copyIntlMapDblToI64( const TIntlValueMapDbl &dMap, TIntlValueMapI64 &iMap )
{
   for( int k = 0; k < (int) iMap.size(); k++ )
   {
      iMap[k] = dblToI64( dMap[k] );
   }
}

int GetEnvCompressFlag()
{
   std::string s{ QueryEnvironmentVariable( strGDXCOMPRESS ) };
   // Note: the default is disabled
   if( s.empty() ) return 0;
   char c{ utils::toupper( s.front() ) };
   return c == 'N' || c == '0' ? 0 : 1;
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
int TGXFileObj::gdxOpenWrite( const char *FileName, const char *Producer, int &ErrNr )
{
   return gdxOpenWriteEx( FileName, Producer, GetEnvCompressFlag(), ErrNr );
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
int TGXFileObj::gdxOpenWriteEx( const char *FileName, const char *Producer, int Compr, int &ErrNr )
{
   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
      std::cout << "gdxOpenWrite("s << FileName << ")\n"s;

   if( fmode != f_not_open )
   {
      ErrNr = ERR_FILEALREADYOPEN;
      return false;
   }
   if( FileName[0] == '\0' )
   {
      ErrNr = ERR_NOFILE;
      LastError = ErrNr;
      return false;
   }
   FFile = std::make_unique<TMiBufferedStreamDelphi>( FileName, FileAccessMode::fmCreate );
   ErrNr = FFile->GetLastIOResult();
   if( ErrNr )
   {
      FFile = nullptr;
      if( ErrNr == strmErrorZLib ) ErrNr = ERR_ZLIB_NOT_FOUND;
      LastError = ErrNr;
      return false;
   }

   Compr &= ( FFile->GetCanCompress() ? 1 : 0 );
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
   AcronymList = std::make_unique<TAcronymList>();
   FilterList = std::make_unique<TFilterList>();
   FFile->WriteByte( gdxHeaderNr );
   FFile->WriteString( gdxHeaderId );
   VersionRead = VERSION;
   FFile->WriteInteger( VersionRead );
   FFile->WriteInteger( Compr );
   FileSystemID = auditLine;
   FFile->WriteString( FileSystemID );
   FProducer = Producer;
   FProducer2.clear();
   FFile->WriteString( FProducer );
   // Reserve some space for positions
   MajorIndexPosition = FFile->GetPosition();
   for( int N{ 1 }; N <= 10; N++ ) FFile->WriteInt64( 0 );
   SetTextList = std::make_unique<TSetTextList>();
   SetTextList->OneBased = false;
   SetTextList->Add( "", 0 );
   gdxResetSpecialValues();
   NextWritePosition = FFile->GetPosition();
   fmode = fw_init;
   fstatus = stat_write;
   DomainStrList = std::make_unique<TDomainStrList>();
   DomainStrList->OneBased = true;
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
int TGXFileObj::gdxDataWriteStrStart( const char *SyId, const char *ExplTxt, int Dim, int Typ, int UserInfo )
{
   if( !PrepareSymbolWrite( "DataWriteStrStart"s, SyId, ExplTxt, Dim, Typ, UserInfo ) ) return false;
   for( int D{}; D < FCurrentDim; D++ )
      LastStrElem[D].front() = std::numeric_limits<char>::max();
   SortList = std::make_unique<LinkedDataType>( FCurrentDim, DataSize * static_cast<int>( sizeof( double ) ) );
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
int TGXFileObj::gdxDataWriteStr( const char **KeyStr, const double *Values )
{
   if( fmode == fw_dom_str ) fmode = fw_str_data;
   if( TraceLevel >= TraceLevels::trl_all || fmode != fw_str_data )
   {
      if( !CheckMode( "DataWriteStr"s, fw_str_data ) ) return false;
      std::cout << "  Index =\n";
      for( int D{}; D < FCurrentDim; D++ )
         std::cout << " " << KeyStr[D] << ( D + 1 < FCurrentDim ? "," : "" ) << "\n";
   }
   // Could actually be GLOBAL_UEL_IDENT_SIZE but is ShortString in Delphi
   static std::array<char, GMS_SSSIZE> SVstorage;
   for( int D{}; D < FCurrentDim; D++ )
   {
      int SVlen;
      const char *SV{ utils::trimRight( KeyStr[D], SVstorage.data(), SVlen ) };
      if( LastStrElem[D].front() == std::numeric_limits<char>::max() || std::strcmp( SV, LastStrElem[D].data() ) )
      {
         // -1=not found, >=1 found
         int KD{ UELTable->IndexOf( SV ) };
         if( KD == -1 )
         {
            if( ErrorCondition( GoodUELString( SV, SVlen ), ERR_BADUELSTR ) ) return false;
            KD = UELTable->AddObject( SV, SVlen, -1 );
         }
         LastElem[D] = KD;
         utils::assignPCharToBuf( SV, SVlen, LastStrElem[D].data(), LastStrElem[D].size() );
         if( KD < MinElem[D] ) MinElem[D] = KD;
         if( KD > MaxElem[D] ) MaxElem[D] = KD;
      }
   }
   SortList->AddItem( LastElem.data(), Values );
   return true;
}

// Brief:
//   Finish a write operation
// Returns:
//   Non-zero if the operation is possible, zero otherwise
// See Also:
//  gdxDataErrorCount, gdxDataWriteRawStart, gdxDataWriteMapStart, gdxDataWriteStrStart,
// Description:
int TGXFileObj::gdxDataWriteDone()
{
   static const TgxModeSet AllowedModes{ fw_raw_data, fw_map_data, fw_str_data, fw_dom_raw, fw_dom_map, fw_dom_str };
   if( !MajorCheckMode( "DataWriteDone"s, AllowedModes ) ) return false;
   if( !utils::in( fmode, fw_raw_data, fw_dom_raw ) )
   {
      InitDoWrite( SortList->Count() );
      ReadPtr = SortList->StartRead( nullptr );
      TIndex AElements;
      TgdxValues AVals;
      while( ReadPtr && SortList->GetNextRecord( &*ReadPtr, AElements.data(), AVals.data() ) )
         DoWrite( AElements.data(), AVals.data() );
      SortList = nullptr;
   }
   FFile->WriteByte( 255 );// end of data
   NextWritePosition = FFile->GetPosition();
   CurSyPtr->SDataCount = DataCount;// reflects dupes
   CurSyPtr->SErrors = ErrCnt;
   ErrCnt = 0;
   fmode = fw_init;
   FFile->SetCompression( false );
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
int TGXFileObj::gdxClose()
{
   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
      std::cout << "gdxClose("s << ( FFile ? FFile->GetFileName() : ""s ) << ")\n"s;

   std::string fnConv;
   if( utils::in( fmode, fw_raw_data, fw_map_data, fw_str_data ) )// unfinished write
      gdxDataWriteDone();
   if( fmode == fw_init )
   {
      fnConv = FFile->GetFileName();
      FFile->SetCompression( CompressOut );
      FFile->SetPosition( NextWritePosition );
      int64_t SymbPos = NextWritePosition;
      FFile->WriteString( MARK_SYMB );
      FFile->WriteInteger( static_cast<int>( NameList->size() ) );
      for( int N{ 1 }; N <= NameList->Count(); N++ )
      {
         FFile->WriteString( NameList->GetString( N ) );
         const auto PSy = *NameList->GetObject( N );
         FFile->WriteInt64( PSy->SPosition );
         FFile->WriteInteger( PSy->SDim );
         FFile->WriteByte( PSy->SDataType );
         FFile->WriteInteger( PSy->SUserInfo );
         FFile->WriteInteger( PSy->SDataCount );
         FFile->WriteInteger( PSy->SErrors );
         FFile->WriteByte( PSy->SSetText );
         FFile->WriteString<GMS_SSSIZE>( PSy->SExplTxt );
         FFile->WriteByte( PSy->SIsCompressed );
         FFile->WriteByte( PSy->SDomSymbols ? 1 : 0 );

         if( PSy->SDomSymbols )
         {
            for( int D{}; D < PSy->SDim; D++ )
            {
               FFile->WriteInteger( PSy->SDomSymbols[D] );
            }
         }

         int CommCnt{ PSy->SCommentsList ? static_cast<int>( PSy->SCommentsList->size() ) : 0 };
         FFile->WriteInteger( CommCnt );
         if( CommCnt )
         {
            for( int Cnt{}; Cnt < CommCnt; Cnt++ )
            {
               auto comment = ( *PSy->SCommentsList )[Cnt];
               FFile->WriteString( comment );
            }
         }
      }
      FFile->WriteString( MARK_SYMB );

      auto SetTextPos{ static_cast<int64_t>( FFile->GetPosition() ) };
      FFile->SetCompression( CompressOut );
      FFile->WriteString( MARK_SETT );
      FFile->WriteInteger( static_cast<int>( SetTextList ? SetTextList->size() : 0 ) );
      if( SetTextList )
         for( int N{}; N < static_cast<int>( SetTextList->Count() ); N++ )
            FFile->WriteString( SetTextList->GetName( N ) );
      FFile->WriteString( MARK_SETT );

      auto UELPos{ static_cast<int64_t>( FFile->GetPosition() ) };
      FFile->SetCompression( CompressOut );
      FFile->WriteString( MARK_UEL );
      UELTable->SaveToStream( *FFile );
      FFile->WriteString( MARK_UEL );

      auto AcronymPos{ static_cast<int64_t>( FFile->GetPosition() ) };
      FFile->SetCompression( CompressOut );
      FFile->WriteString( MARK_ACRO );
      AcronymList->SaveToStream( *FFile );
      FFile->WriteString( MARK_ACRO );

      auto DomStrPos{ static_cast<int64_t>( FFile->GetPosition() ) };
      FFile->SetCompression( CompressOut );
      FFile->WriteString( MARK_DOMS );
      DomainStrList->SaveToStream( *FFile );
      FFile->WriteString( MARK_DOMS );

      for( int N{ 1 }; N <= NameList->Count(); N++ )
      {
         const auto *PSy = *( *NameList )[N];
         if( PSy->SDomStrings )
         {
            FFile->WriteInteger( N );
            for( int D{}; D < PSy->SDim; D++ )
            {
               FFile->WriteInteger( PSy->SDomStrings[D] );
            }
         }
      }
      FFile->WriteInteger( -1 );
      FFile->WriteString( MARK_DOMS );

      // This must be at the very end!!!
      FFile->SetPosition( MajorIndexPosition );
      FFile->SetCompression( false );
      FFile->WriteInteger( MARK_BOI );
      // Note that we have room for 10 indices; if we need more, create an overflow link in the 10th position.
      for( int64_t offset: { SymbPos, UELPos, SetTextPos, AcronymPos, NextWritePosition, DomStrPos } )
         FFile->WriteInt64( offset );
   }

   int res{ FFile ? FFile->GetLastIOResult() : 1 };

   // Many free operations. Some not necessary anymore due to RAII pattern (out of scope -> destroy)
   if( NameList )
   {
      for( int N{ 1 }; N <= NameList->Count(); N++ )
      {
         const auto PSy = *NameList->GetObject( N );
         delete PSy;
      }
      NameList = nullptr;
   }

   FFile = nullptr;
   SetTextList = nullptr;
   UELTable = nullptr;
   SortList = nullptr;
   DomainStrList = nullptr;
   ErrorList = nullptr;
   FilterList = nullptr;
   AcronymList = nullptr;
   MapSetText = nullptr;

   fmode = f_not_open;
   fstatus = stat_notopen;

   if( AutoConvert && !fnConv.empty() )
   {
      res = ConvertGDXFile( fnConv, CompressOut ? "C" : "U" );
      if( res > 0 ) res += 100;
   }
   return res;
}

void TGXFileObj::InitErrors()
{
   ErrCnt = ErrCntTotal = 0;
   LastError = LastRepError = ERR_NOERROR;
}

TGXFileObj::TGXFileObj( std::string &ErrMsg )
{
   ErrMsg.clear();
   gdxResetSpecialValues();
}

TGXFileObj::~TGXFileObj()
{
   if( fmode != f_not_open )
   {
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
int TGXFileObj::gdxResetSpecialValues()
{
   intlValueMapDbl[vm_valund] = GMS_SV_UNDEF;
   intlValueMapDbl[vm_valna] = GMS_SV_NA;
   intlValueMapDbl[vm_valpin] = GMS_SV_PINF;
   intlValueMapDbl[vm_valmin] = GMS_SV_MINF;
   intlValueMapDbl[vm_valeps] = GMS_SV_EPS;
   intlValueMapDbl[vm_zero] = 0.0;
   intlValueMapDbl[vm_one] = 1.0;
   intlValueMapDbl[vm_mone] = -1.0;
   intlValueMapDbl[vm_half] = 0.5;
   intlValueMapDbl[vm_two] = 2.0;
   readIntlValueMapDbl = intlValueMapDbl;

   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
   {
      std::cout << "reset special vals, dump of readIntlValueMapDbl\n"s;
      const std::array<std::pair<std::string, int>, 5> svNameIndexPairs{
              { { "undef"s, sv_valund },
                { "na"s, sv_valna },
                { "posinf"s, sv_valpin },
                { "min"s, sv_valmin },
                { "eps"s, sv_valeps } } };
      for( auto &[svName, svIndex]: svNameIndexPairs )
         std::cout << svName << "="s << readIntlValueMapDbl[svIndex] << '\n';
   }

   copyIntlMapDblToI64( intlValueMapDbl, intlValueMapI64 );
   Zvalacr = GMS_SV_ACR;
   return true;
}

bool TGXFileObj::PrepareSymbolWrite( const std::string_view Caller,
                                     const char *AName,
                                     const std::string_view AText,
                                     int ADim,
                                     int AType,
                                     int AUserInfo )
{
   CurSyPtr = nullptr;
   ErrorList = nullptr;
   SortList = nullptr;

   if( !MajorCheckMode( Caller, fw_init ) ) return false;

   if( TraceLevel >= TraceLevels::trl_some )
      WriteTrace( "Symbol = "s + AName + ", Dim = "s + std::to_string( ADim ) );

   if( !IsGoodNewSymbol( AName ) ) return false;

   if( ErrorCondition( ADim >= 0 && ADim <= GLOBAL_MAX_INDEX_DIM, ERR_BADDIMENSION ) ||
       ErrorCondition( AType >= 0 && AType <= dt_equ, ERR_BADDATATYPE ) ) return false;
   CurSyPtr = new TgdxSymbRecord{};
   auto &obj = CurSyPtr;
   obj->SPosition = CurSyPtr->SDataCount = CurSyPtr->SErrors = 0;// Position
   obj->SDim = ADim;
   obj->SDataType = static_cast<gdxSyType>( AType );
   obj->SUserInfo = AUserInfo;
   obj->SSetText = false;
   utils::assignViewToBuf( AText, obj->SExplTxt.data(), GMS_SSSIZE );
   MakeGoodExplText( obj->SExplTxt.data() );
   obj->SIsCompressed = CompressOut && ADim > 0;
   obj->SCommentsList = std::nullopt;
   obj->SDomSymbols = nullptr;
   obj->SDomStrings = nullptr;
   obj->SSetBitMap = utils::in( (gdxSyType) AType, dt_set, dt_alias ) && ADim == 1 && StoreDomainSets ? std::make_unique<TSetBitMap>() : nullptr;

   CurSyPtr->SSyNr = NameList->AddObject( AName, std::strlen( AName ), CurSyPtr );// +1 for universe
   FCurrentDim = ADim;
   // old case; we never write V6
   // V = 0..Dim which dimension changed
   // V = Dim+1 .. 254 change in last dimension = V - Dim
   // 255 is EOF
   DeltaForWrite = 255 - ( VERSION <= 6 ? MaxDimV148 : FCurrentDim ) - 1;

   DataSize = DataTypSize[AType];
   if( DataSize > 0 )
      LastDataField = static_cast<tvarvaltype>( DataSize - 1 );

   for( int D{}; D < FCurrentDim; D++ )
   {
      LastElem[D] = INDEX_INITIAL;
      MinElem[D] = std::numeric_limits<int>::max();
      MaxElem[D] = 0;
      WrBitMaps[D] = nullptr;
   }

   FFile->SetCompression( CurSyPtr->SIsCompressed );
   return true;
}

bool TGXFileObj::MajorCheckMode( const std::string_view Routine, TgxFileMode m )
{
   MajContext = Routine;
   LastRepError = ERR_NOERROR;
   return !( ( TraceLevel >= TraceLevels::trl_some || fmode != m ) && !CheckMode( Routine, m ) );
}

bool TGXFileObj::MajorCheckMode( const std::string_view Routine, const TgxModeSet &MS )
{
   MajContext = Routine;
   LastRepError = ERR_NOERROR;
   return !( ( TraceLevel >= TraceLevels::trl_some || !utils::in( fmode, MS ) ) && !CheckMode( Routine, MS ) );
}

void TGXFileObj::WriteTrace( const std::string_view s ) const
{
   std::cout << "gdxTrace " << TraceStr << ": " << s << '\n';
}

bool TGXFileObj::IsGoodNewSymbol( const char *s )
{
   return !( ErrorCondition( NameList->IndexOf( s ) < 1, ERR_DUPLICATESYMBOL ) ||
             ErrorCondition( AcronymList->FindName( s ) < 0, ERR_DUPLICATESYMBOL ) ||
             ErrorCondition( IsGoodIdent( s ), ERR_BADIDENTFORMAT ) );
}

bool TGXFileObj::ErrorCondition( bool cnd, int N )
{
   if( !cnd ) ReportError( N );
   return !cnd;
}

void TGXFileObj::ReportError( int N )
{
   if( TraceLevel >= TraceLevels::trl_errors && N != LastRepError )
   {
      if( !MajContext.empty() )
         std::cout << "Error after call to " << MajContext << '\n';
      std::array<char, GMS_SSSIZE> s{};
      this->gdxErrorStr( N, s.data() );
      std::cout << "Error = " << N << " : " << s.data() << "\n";
   }
   SetError( N );
   LastRepError = N;
}

bool TGXFileObj::CheckMode( const std::string_view Routine, TgxFileMode m )
{
   const TgxModeSet singleMode{ m };
   return CheckMode( Routine, singleMode );
}

bool TGXFileObj::CheckMode( const std::string_view Routine )
{
   static const TgxModeSet noMode{};
   return CheckMode( Routine, noMode );
}

bool TGXFileObj::CheckMode( const std::string_view Routine, const TgxModeSet &MS )
{
   if( MS.empty() || utils::in( fmode, MS ) )
   {
      WriteTrace( Routine );
      return true;
   }
   SetError( ERR_BADMODE );
   std::cout << "**** Error: " << Routine << " called out of context\n";
   if( !MajContext.empty() && !utils::sameText( MajContext, Routine ) )
      std::cout << "     Previous major function called was " << MajContext << '\n';
   std::cout << "     Current context = " << fmode_str[fmode] << '\n';
   std::cout << "     Allowed = {";
   bool f{ true };
   for( int M{}; M < tgxfilemode_count; M++ )
   {
      if( utils::in( static_cast<TgxFileMode>( M ), MS ) )
      {
         if( f ) f = false;
         else
            std::cout << ',';
         std::cout << fmode_str[M];
      }
   }
   std::cout << "}\n";
   return false;
}

int TGXFileObj::PrepareSymbolRead( const std::string_view Caller, int SyNr, const int *ADomainNrs, TgxFileMode newmode )
{
   if( utils::in( fmode, fr_str_data, fr_map_data, fr_mapr_data, fr_raw_data ) )
      gdxDataReadDone();
   NrMappedAdded = 0;
   TIntegerMapping ExpndList;
   ErrorList = nullptr;
   CurSyPtr = nullptr;
   SortList = nullptr;

   if( !MajorCheckMode( Caller, fr_init ) )
   {
      fmode = fr_init;
      return -1;
   }
   ReadUniverse = !SyNr;
   if( !ReadUniverse )
   {
      if( ErrorCondition( SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX ) ) return -1;
      CurSyPtr = *NameList->GetObject( SyNr );
      if( CurSyPtr->SDataType == dt_alias )
      {
         do {
            SyNr = CurSyPtr->SUserInfo;
            if( !SyNr )
            {
               ReadUniverse = true;
               break;
            }
            CurSyPtr = *NameList->GetObject( SyNr );
         } while( CurSyPtr->SDataType == dt_alias );
         if( !ReadUniverse )
         {
            assert( CurSyPtr->SDataType == dt_set && "Bad aliased set-1" );
         }
      }
   }
   int NrRecs;
   if( ReadUniverse )
   {
      FCurrentDim = 1;
      DataSize = DataTypSize[dt_set];
      LastDataField = static_cast<tvarvaltype>( DataSize - 1 );
      NrRecs = UelCntOrig;
      UniverseNr = 0;
      CurSyPtr = nullptr;
   }
   else
   {
      FCurrentDim = CurSyPtr->SDim;
      FFile->SetCompression( CurSyPtr->SIsCompressed );
      FFile->SetPosition( CurSyPtr->SPosition );
      DataSize = DataTypSize[CurSyPtr->SDataType];
      if( DataSize > 0 ) LastDataField = static_cast<tvarvaltype>( DataSize - 1 );
      NrRecs = CurSyPtr->SDataCount;
   }

   if( verboseTrace && TraceLevel >= TraceLevels::trl_some )
      WriteTrace( "Symbol = "s + std::to_string( SyNr ) +
                  ( CurSyPtr ? ", Dim = "s + std::to_string( CurSyPtr->SDim ) : ""s ) );

   DeltaForRead = VersionRead <= 6 ? MaxDimV148 : FCurrentDim;
   for( int D{}; D < FCurrentDim; D++ )
   {
      auto &obj = DomainList[D];
      obj.DFilter = nullptr;
      switch( ADomainNrs[D] )
      {
         case DOMC_UNMAPPED:
            obj.DAction = TgdxDAction::dm_unmapped;
            break;
         case DOMC_EXPAND:
            obj.DAction = TgdxDAction::dm_expand;
            break;
         case DOMC_STRICT:
            obj.DAction = TgdxDAction::dm_strict;
            break;
         default:// filter number
            obj.DFilter = FilterList->FindFilter( ADomainNrs[D] );
            if( obj.DFilter ) obj.DAction = TgdxDAction::dm_filter;
            else
            {
               ReportError( ERR_UNKNOWNFILTER );
               return -1;
            }
            break;
      }
   }

   if( !ReadUniverse )
   {
      if( ErrorCondition( FFile->ReadString() == MARK_DATA, ERR_BADDATAMARKER_DATA ) ||
          ErrorCondition( FFile->ReadByte() == FCurrentDim, ERR_BADDATAMARKER_DIM ) ) return -1;
      FFile->ReadInteger();//skip record counter
   }

   if( !FCurrentDim && !NrRecs )
   {
      CurSyPtr->SScalarFrst = true;
      fmode = newmode;
      return 1;
   }

   if( !ReadUniverse )
   {
      CurSyPtr->SScalarFrst = false;
      std::fill_n( LastElem.begin(), FCurrentDim, INDEX_INITIAL );
      std::fill_n( PrevElem.begin(), FCurrentDim, -1 );
      for( int D{}; D < FCurrentDim; D++ )
      {
         MinElem[D] = FFile->ReadInteger();
         MaxElem[D] = FFile->ReadInteger();
         ElemType[D] = GetIntegerSize( MaxElem[D] - MinElem[D] + 1 );
      }
   }
   bool AllocOk{ true };

   int res{ -1 };
   if( utils::in( newmode, fr_raw_data, fr_str_data, fr_slice ) )
      res = NrRecs;
   else
   {
      assert( newmode == fr_map_data && "Expect to read mapped data" );
      if( ResultWillBeSorted( ADomainNrs ) )
      {
         //WriteLn('Changing mapped read to raw read');
         res = NrRecs;// ignores filtering etc
         newmode = fr_mapr_data;
      }
      else
      {
         //WriteLn('Cannot do mapped read fast');
         try
         {
            SortList = std::make_unique<LinkedDataType>( FCurrentDim, static_cast<int>( DataSize * sizeof( double ) ) );
            int FIDim = FCurrentDim;// First invalid dimension
            TgdxValues Avals;
            TIndex AElements{};
            int AFDim, V;
            bool AddNew{}, AddError{}, BadError{};
            while( DoRead( Avals.data(), AFDim ) )
            {
               if( FIDim < AFDim ) AFDim = FIDim;
               FIDim = FCurrentDim;
               int D;
               for( D = AFDim - 1; D < FCurrentDim && !AddError; D++ )
               {
                  const auto &obj = DomainList[D];
                  if( LastElem[D] < 0 )
                  {
                     ReportError( ERR_BADELEMENTINDEX );
                     BadError = true;
                     break;
                  }
                  switch( obj.DAction )
                  {
                     case TgdxDAction::dm_unmapped:
                        // NOTE: Not covered by unit tests yet.
                        AElements[D] = LastElem[D];
                        break;
                     case TgdxDAction::dm_filter:
                        // NOTE: Not covered by unit tests yet.
                        V = UELTable->GetUserMap( LastElem[D] );
                        if( obj.DFilter->InFilter( V ) )
                           AElements[D] = V;
                        else
                        {
                           AddError = true;
                           FIDim = D;
                        }
                        break;
                     case TgdxDAction::dm_strict:
                        V = UELTable->GetUserMap( LastElem[D] );
                        if( V >= 0 ) AElements[D] = V;
                        else
                        {
                           AddError = true;
                           FIDim = D;
                        }
                        break;
                     case TgdxDAction::dm_expand:
                     {
                        int EN = LastElem[D];
                        V = ExpndList.GetMapping( EN );
                        if( V >= 0 ) AElements[D] = V;
                        else
                        {
                           V = UELTable->GetUserMap( EN );
                           if( V >= 0 )
                           {
                              ExpndList.SetMapping( EN, V );
                              AElements[D] = V;
                           }
                           else
                           {
                              AElements[D] = -EN;// so we recognize and assign the same nr
                              AddNew = true;
                           }
                        }
                     }
                     break;
                  }
               }
               if( BadError )
               {
                  AllocOk = false;// That is used as a signal below for something bad happend, i.e., an exception was thrown
                  break;
               }
               else if( AddError )
               {
                  // NOTE: Not covered by unit tests yet.
                  // Ensure that dimensions to the right have no bad UELs
                  for( int D2{ D + 1 }; D2 <= FCurrentDim; D2++ )
                  {
                     if( LastElem[D2] < 0 )
                     {
                        ReportError( ERR_BADELEMENTINDEX );
                        AllocOk = false;// That is used as a signal below for something bad happend, i.e., an exception was thrown
                        break;
                     }
                  }
                  LastElem[FIDim] = -LastElem[FIDim];
                  AddToErrorListDomErrs( LastElem, Avals.data() );// unmapped
                  LastElem[FIDim] = -LastElem[FIDim];
                  AddError = false;
               }
               else
               {
                  if( AddNew )
                  {
                     for( D = 0; D < FCurrentDim; D++ )
                     {
                        int EN = AElements[D];
                        if( EN < 0 )
                        {
                           V = UELTable->NewUsrUel( -EN );
                           AElements[D] = V;
                           ExpndList.SetMapping( -EN, V );
                           NrMappedAdded++;
                           // look for same mapping to be issued
                           for( int D2{ D + 1 }; D2 < FCurrentDim; D2++ )
                           {
                              if( AElements[D2] == EN ) AElements[D2] = V;
                           }
                        }
                     }
                     AddNew = false;
                  }
                  SortList->AddItem( AElements.data(), Avals.data() );
               }
            }
            ReadPtr = SortList->StartRead( nullptr );
            res = static_cast<int>( SortList->Count() );
         }
         catch( std::exception &e )
         {
            std::cout << "Exception: " << e.what() << "\n";
            AllocOk = false;
         }
      }
   }

   if( AllocOk )
   {
      std::fill_n( LastElem.begin(), FCurrentDim, -1 );
      fmode = newmode;
      return res;
   }
   else
   {
      SetError( ERR_OUT_OF_MEMORY );
      SortList = nullptr;
      fmode = fr_init;
      return -1;
   }
}

void TGXFileObj::SetError( int N )
{
   if( N == 0 ) return;
   if( LastError == ERR_NOERROR ) LastError = N;
   ErrCnt++;
   ErrCntTotal++;
}

void TGXFileObj::InitDoWrite( int NrRecs )
{
   DataCount = 0;
   FFile->SetPosition( NextWritePosition );
   CurSyPtr->SPosition = NextWritePosition;
   FFile->WriteString( MARK_DATA );
   FFile->WriteByte( static_cast<uint8_t>( FCurrentDim ) );
   FFile->WriteInteger( NrRecs );// ignores dupes in count
   for( int D{}; D < FCurrentDim; D++ )
   {
      LastElem[D] = INDEX_INITIAL;
      ElemType[D] = GetIntegerSize( MaxElem[D] - MinElem[D] + 1 );
      FFile->WriteInteger( MinElem[D] );
      FFile->WriteInteger( MaxElem[D] );
   }
}

// we have to make these mask "constants" vars since we cannot
// have large constants on input
const int64_t
        signMask{ (int64_t) 0x80000000 << 32 },
        expoMask{ (int64_t) 0x7ff00000 << 32 },
        mantMask{ ~( signMask | expoMask ) };

enum TDblClass
{
   DBL_NAN,  // any sort of NaN
   DBL_NINF, // negative infinity
   DBL_PINF, // positive infinity
   DBL_FINITE// positive infinity
};

TDblClass dblInfo( double x, int64_t &i );

const std::array DblClassText{
        "NaN"s, "negative infinity"s, "positive infinity"s, "finite"s };

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
TDblClass dblInfo( double x, int64_t &i )
{
   TI64Rec i64Rec{};
   i64Rec.x = x;
   i = i64Rec.i64;
   int64_t exponent{ i & expoMask };
   if( exponent == expoMask )
   {
      int64_t mantissa{ i & mantMask };
      return mantissa ? DBL_NAN : ( ( i & signMask ) ? DBL_NINF : DBL_PINF );
   }
   return DBL_FINITE;
}

inline bool accessBitMap( const TSetBitMap &bmap, int index )
{
   return bmap.GetBit( index );
}

bool TGXFileObj::DoWrite( const int *AElements, const double *AVals )
{
   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
   {
      std::cout << "DoWrite index: "s;
      for( int D{}; D < FCurrentDim; D++ )
         std::cout << std::to_string( AElements[D] ) << ( D + 1 < FCurrentDim ? ","s : ""s );
      std::cout << '\n';
   }

   int FDim{ FCurrentDim + 1 }, delta{};
   for( int D{}; D < FCurrentDim; D++ )
   {
      if( WrBitMaps[D] && !accessBitMap( *WrBitMaps[D], AElements[D] ) )
      {
         ReportError( ERR_DOMAINVIOLATION );
         TgdxUELIndex ErrorUELs{};
         for( int DD{}; DD < D; DD++ )
            ErrorUELs[DD] = AElements[DD];
         ErrorUELs[D] = -AElements[D];
         // see if there are more domain violations
         for( int DD{ D + 1 }; DD < FCurrentDim; DD++ )
         {
            bool neg{ WrBitMaps[DD] && !accessBitMap( *WrBitMaps[DD], AElements[DD] ) };
            ErrorUELs[DD] = ( neg ? -1 : 1 ) * AElements[DD];
         }
         AddToErrorListDomErrs( ErrorUELs, AVals );
         return false;
      }
   }
   for( int D{}; D < FCurrentDim; D++ )
   {
      delta = AElements[D] - LastElem[D];
      if( delta )
      {
         FDim = D + 1;// plus one to match Delphi semantics
         break;
      }
   }
   if( FDim > FCurrentDim )
   {
      if( FCurrentDim > 0 && DataCount >= 1 )
      {
         ReportError( ERR_DATADUPLICATE );
         AddToErrorList( AElements, AVals );
         return false;
      }
      FFile->WriteByte( 1 );// keeps logic working for scalars
   }
   else
   {
      if( delta < 0 )
      {
         ReportError( ERR_RAWNOTSORTED );
         AddToErrorList( AElements, AVals );
         return false;
      }
      if( FDim == FCurrentDim && delta <= DeltaForWrite )
      {// small change in last dimension
         FFile->WriteByte( FCurrentDim + delta );
         LastElem[FCurrentDim - 1] = AElements[FCurrentDim - 1];
      }
      else
      {// general change
         FFile->WriteByte( FDim );
         for( int D{ FDim - 1 }; D < FCurrentDim; D++ )
         {
            int v{ AElements[D] - MinElem[D] };
            switch( ElemType[D] )
            {
               case TgdxElemSize::sz_integer:
                  FFile->WriteInteger( v );
                  break;
               case TgdxElemSize::sz_word:
                  FFile->WriteWord( v );
                  break;
               case TgdxElemSize::sz_byte:
                  FFile->WriteByte( v );
                  break;
            }
            LastElem[D] = AElements[D];
         }
      }
   }
   if( DataSize > 0 )
   {
      for( int DV{}; DV <= LastDataField; DV++ )
      {
         double X{ AVals[DV] };
         int64_t i64;
         TDblClass dClass{ dblInfo( X, i64 ) };
         int xv{ vm_valund };
         for( ; xv < vm_normal; xv++ )
            if( i64 == intlValueMapI64[xv] ) break;
         if( xv == vm_normal && dClass != DBL_FINITE )
         {
            switch( dClass )
            {
               case DBL_NINF:
                  xv = vm_valmin;
                  break;
               case DBL_PINF:
                  xv = vm_valpin;
                  break;
               case DBL_NAN:
                  xv = vm_valna;
                  break;
               default:
                  break;
            }
         }
         FFile->WriteByte( xv );
         if( xv == vm_normal )
         {
            FFile->WriteDouble( X );
            if( X >= Zvalacr )
               AcronymList->CheckEntry( static_cast<int>( std::round( X / Zvalacr ) ) );
         }
      }
      if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
         std::cout << "level="s << AVals[GMS_VAL_LEVEL] << '\n';
   }
   DataCount++;
   if( utils::in( CurSyPtr->SDataType, dt_set, dt_alias ) )
   {
      if( AVals[GMS_VAL_LEVEL] != 0.0 ) CurSyPtr->SSetText = true;
      if( FCurrentDim == 1 && CurSyPtr->SSetBitMap )
      {
         auto &ssbm = *CurSyPtr->SSetBitMap;
         ssbm.SetBit( LastElem.front(), true );
      }
   }
   return true;
}

bool TGXFileObj::DoRead( double *AVals, int &AFDim )
{
   const auto maybeRemap = [this]( double v ) {
      return v >= Zvalacr ? AcronymRemap( v ) : v;
   };

   if( ReadUniverse )
   {
      UniverseNr++;
      bool res{ UniverseNr <= UelCntOrig };
      if( res )
      {
         LastElem[0] = UniverseNr;
         AVals[GMS_VAL_LEVEL] = 0.0;
         AFDim = 1;
      }
      return res;
   }
   if( CurSyPtr->SScalarFrst )
   {
      CurSyPtr->SScalarFrst = false;
      GetDefaultRecord( AVals );
      AFDim = 0;
      return true;
   }

   uint8_t B;
   FFile->Read( &B, 1 );
   if( B > DeltaForRead )
   {// relative change in last dimension
      if( B == 255 ) return false;
      AFDim = FCurrentDim;
      if( FCurrentDim > 0 ) LastElem[FCurrentDim - 1] += B - DeltaForRead;
   }
   else
   {
      AFDim = B;
      for( int D{ AFDim - 1 }; D < FCurrentDim; D++ )
      {
         assert( D >= 0 );
         switch( ElemType[D] )
         {
            case TgdxElemSize::sz_integer:
               LastElem[D] = FFile->ReadInteger() + MinElem[D];
               break;
            case TgdxElemSize::sz_word:
               LastElem[D] = FFile->ReadWord() + MinElem[D];
               break;
            case TgdxElemSize::sz_byte:
               LastElem[D] = FFile->ReadByte() + MinElem[D];
               break;
         }
      }
   }
   if( DataSize > 0 )
   {
      for( int DV{ GMS_VAL_LEVEL }; DV <= LastDataField; DV++ )
      {
         uint8_t BSV;
         FFile->Read( &BSV, 1 );
         TgdxIntlValTyp SV{ static_cast<TgdxIntlValTyp>( BSV ) };
         AVals[DV] = SV != vm_normal ? readIntlValueMapDbl[SV] : maybeRemap( FFile->ReadDouble() );
      }
      if( MapSetText && AVals[GMS_VAL_LEVEL] != 0.0 && CurSyPtr->SDataType == dt_set )
      {// remap settext number
         // NOTE: Not covered by unit tests yet.
         double X{ AVals[GMS_VAL_LEVEL] };
         int D{ static_cast<int>( std::round( X ) ) };
         if( std::abs( X - D ) < 1e-12 && D >= 0 && D <= SetTextList->GetCapacity() )
            AVals[GMS_VAL_LEVEL] = MapSetText[D];
      }
      if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
         std::cout << "level="s << AVals[GMS_VAL_LEVEL] << '\n';
   }
   return true;
}

double TGXFileObj::AcronymRemap( double V )
{
   auto GetAsAcronym = [&]( double v ) {
      int orgIndx{ static_cast<int>( std::round( v / Zvalacr ) ) },
              N{ AcronymList->FindEntry( orgIndx ) },
              newIndx{};
      if( N < 0 )
      {// not found
         // NOTE: Not covered by unit tests yet.
         if( NextAutoAcronym <= 0 ) newIndx = orgIndx;
         else
         {
            newIndx = NextAutoAcronym++;
            N = AcronymList->AddEntry( "", "", orgIndx );
            ( *AcronymList )[N].AcrReadMap = newIndx;
            ( *AcronymList )[N].AcrAutoGen = true;
         }
      }
      else
      {// found
         newIndx = ( *AcronymList )[N].AcrReadMap;
         if( newIndx <= 0 )
         {
            if( NextAutoAcronym <= 0 ) newIndx = orgIndx;
            else
            {
               newIndx = NextAutoAcronym++;
               ( *AcronymList )[N].AcrReadMap = newIndx;
               ( *AcronymList )[N].AcrAutoGen = true;
            }
         }
      }
      return Zvalacr * newIndx;
   };

   if( V < Zvalacr )
      return V;
   else
   {
      if( V == 0.0 ) return 0.0;
      if( std::isnan( V ) ) return intlValueMapDbl[vm_valna];
      if( std::isinf( V ) ) return V < 0.0 ? intlValueMapDbl[vm_valmin] : intlValueMapDbl[vm_valpin];
      if( std::isnormal( V ) ) return V < 0.0 ? V : GetAsAcronym( V );
      return intlValueMapDbl[vm_valna];
   }
}

void TGXFileObj::AddToErrorListDomErrs( const std::array<int, GLOBAL_MAX_INDEX_DIM> &AElements, const double *AVals )
{
   if( !ErrorList ) ErrorList = std::make_unique<TTblGamsDataImpl<double>>( FCurrentDim, (int) ( DataSize * sizeof( double ) ) );
   else if( ErrorList->GetCount() >= 11 )
      return;

   static std::array<int, GLOBAL_MAX_INDEX_DIM> keys{};
   static std::array<double, GMS_VAL_MAX> vals{};

   for( int D{}; D < FCurrentDim; D++ )
   {
      int EN{ AElements[D] };
      if( EN < 0 )
      {
         bool Found{};
         for( int i{}; i < ErrorList->GetCount(); i++ )
         {
            // NOTE: Not covered by unit tests yet.
            ErrorList->GetRecord( i, keys.data(), vals.data() );
            if( keys[D] == EN )
            {
               Found = true;
               break;
            }
         }
         if( !Found )
         {
            ErrorList->AddRecord( AElements.data(), AVals );
            return;
         }
      }
   }
}

void TGXFileObj::AddToErrorList( const int *AElements, const double *AVals )
{
   if( !ErrorList )
      ErrorList = std::make_unique<TTblGamsDataImpl<double>>( FCurrentDim, (int) ( DataSize * sizeof( double ) ) );
   else if( ErrorList->GetCount() >= 11 )// avoid storing too many errors
      return;
   ErrorList->AddRecord( AElements, AVals );
}

bool TGXFileObj::ResultWillBeSorted( const int *ADomainNrs )
{
   for( int D{}; D < FCurrentDim; D++ )
   {
      switch( ADomainNrs[D] )
      {
         case DOMC_UNMAPPED:
            continue;
         case DOMC_EXPAND:
            if( UELTable->GetMapToUserStatus() == TUELUserMapStatus::map_unsorted ) return false;
            if( !D )
            {
               if( UELTable->GetMapToUserStatus() >= TUELUserMapStatus::map_sortgrow ) continue;
               else
                  return false;
            }
            else if( UELTable->GetMapToUserStatus() == TUELUserMapStatus::map_sortfull )
               continue;
            else
               return false;
            break;
         case DOMC_STRICT:
            if( UELTable->GetMapToUserStatus() == TUELUserMapStatus::map_unsorted ) return false;
            break;
         default:
            if( UELTable->GetMapToUserStatus() >= TUELUserMapStatus::map_sorted ) continue;
            const TDFilter *PFilter = FilterList->FindFilter( ADomainNrs[D] );
            if( !PFilter->FiltSorted ) return false;
            break;
      }
   }
   return true;
}

void TGXFileObj::GetDefaultRecord( double *Avals ) const
{
   int ui{};
   switch( CurSyPtr->SDataType )
   {
      case dt_set:
      case dt_alias:
      case dt_par:
         Avals[GMS_VAL_LEVEL] = 0.0;
         break;
      case dt_var:
         ui = CurSyPtr->SUserInfo;
         std::memcpy( Avals, ui >= GMS_VARTYPE_UNKNOWN && ui <= GMS_VARTYPE_SEMIINT ? gmsDefRecVar[ui] : gmsDefRecVar[GMS_VARTYPE_UNKNOWN], sizeof( double ) * 5 );
         break;
      case dt_equ:
         ui = CurSyPtr->SUserInfo;
         std::memcpy( Avals, ui >= GMS_EQUTYPE_E && ui <= GMS_EQUTYPE_E + ( GMS_EQUTYPE_B + 1 ) ? gmsDefRecEqu[ui] : gmsDefRecEqu[GMS_EQUTYPE_E], sizeof( double ) * 5 );
         break;
      default:
         assert( false && "GetDefaultRecord-2" );
         break;
   }
}

const std::map<int, std::string> errorCodeToStr{
        { ERR_NOFILE, "File name is empty"s },
        { ERR_FILEERROR, "File I/O error"s },
        { ERR_NOERROR, "No error"s },
        { ERR_BADMODE, "Bad mode"s },
        { ERR_BADDIMENSION, "Bad dimension"s },
        { ERR_BAD_ALIAS_DIM, "Bad dimension for aliased set"s },
        { ERR_BADELEMENTINDEX, "Bad UEL Nr"s },
        { ERR_BADSYMBOLINDEX, "Bad symbol number"s },
        { ERR_ELEMENTSEQUENCE, "Element out of sequence"s },
        { ERR_DUPLICATESYMBOL, "Duplicate symbol"s },
        { ERR_DATANOTSORTED, "Data is not sorted"s },
        { ERR_DATADUPLICATE, "Duplicate keys"s },
        { ERR_UNKNOWNFILTER, "Unknown filter"s },
        { ERR_BADSTRINGFORMAT, "Bad quotes"s },
        { ERR_BADIDENTFORMAT, "Illegal identifier"s },
        { ERR_UELCONFLICT, "UEL string with different index"s },
        { ERR_DUPLICATESPECVAL, "Duplicate special value"s },
        { ERR_BADERRORRECORD, "Bad Error record number"s },
        { ERR_DUPLICATEUEL, "Duplicate UEL"s },
        { ERR_BADUELSTR, "Bad UEL string"s },
        { ERR_UNDEFUEL, "Unknown UEL"s },
        { ERR_UELSECONDWRITE, "gdx file has UEL table already"s },
        { ERR_UELNOTEMPTY, "UEL table is not empty"s },
        { ERR_BAD_FILTER_NR, "Bad filter number"s },
        { ERR_BAD_FILTER_INDX, "Bad index in filter"s },
        { ERR_FILTER_UNMAPPED, "Unmapped index in filter"s },
        { ERR_OBSOLETE_FUNCTION, "Use of obsolete function"s },
        { ERR_RAWNOTSORTED, "Data not sorted when writing raw"s },
        { ERR_BADACROINDEX, "Bad index for acronym"s },
        { ERR_BADACRONUMBER, "Bad acronym record number"s },
        { ERR_BADACRONAME, "Bad acronym name for update"s },
        { ERR_ACRODUPEMAP, "Bad acronym index for update"s },
        { ERR_ACROBADADDITION, "Bad addition to acronym table"s },
        { ERR_UNKNOWNDOMAIN, "Unknown domain"s },
        { ERR_BADDOMAIN, "Domain not set with dim=1"s },
        { ERR_NODOMAINDATA, "Set has no data"s },
        { ERR_ALIASSETEXPECTED, "Set expected for domain"s },
        { ERR_BADDATATYPE, "Bad data type"s },
        { ERR_NOSYMBOLFORCOMMENT, "No symbol to add comment to"s },
        { ERR_DOMAINVIOLATION, "Domain violation"s },
        { ERR_FILEALREADYOPEN, "File is already open"s },
        { ERR_FILETOOLDFORAPPEND, "File version to old for append"s },
        { ERR_OPEN_DOMSMARKER1, "Expected data marker (DOMS_1) not found in GDX file"s },
        { ERR_OPEN_DOMSMARKER2, "Expected data marker (DOMS_2) not found in GDX file"s },
        { ERR_OPEN_DOMSMARKER3, "Expected data marker (DOMS_3) not found in GDX file"s },
        { ERR_BADDATAMARKER_DATA, "Expected data marker (DATA) not found in GDX file"s },
        { ERR_BADDATAMARKER_DIM, "Expected data marker (DIM) not found in GDX file"s },
        { ERR_OPEN_BOI, "Expected data marker (BOI) not found in GDX file"s },
        { ERR_OPEN_FILEHEADER, "Expected data marker (FILEHEADER) not found in GDX file"s },
        { ERR_OPEN_FILEMARKER, "Expected data marker (FILEMARKER) not found in GDX file"s },
        { ERR_OPEN_SYMBOLMARKER1, "Expected data marker (SYMBOL_1) not found in GDX file"s },
        { ERR_OPEN_SYMBOLMARKER2, "Expected data marker (SYMBOL_2) not found in GDX file"s },
        { ERR_OPEN_UELMARKER1, "Expected data marker (UEL_1) not found in GDX file"s },
        { ERR_OPEN_UELMARKER2, "Expected data marker (UEL_2) not found in GDX file"s },
        { ERR_OPEN_TEXTMARKER1, "Expected data marker (TEXT_1) not found in GDX file"s },
        { ERR_OPEN_TEXTMARKER2, "Expected data marker (TEXT_2) not found in GDX file"s },
        { ERR_OPEN_ACROMARKER1, "Expected data marker (ACRO_1) not found in GDX file"s },
        { ERR_OPEN_ACROMARKER2, "Expected data marker (ACRO_2) not found in GDX file"s },
        { ERR_OPEN_FILEVERSION, "GDX file version not supported"s },
        { ERR_BADDATAFORMAT, "File not recognized as a GDX file"s },
        { ERR_OUT_OF_MEMORY, "Out of memory"s },
        { ERR_ZLIB_NOT_FOUND, "Compression library not found"s },
        { ERR_GDXCOPY, "GDXCOPY: Unknown error"s },
        { ERR_PARAMETER, "GDXCOPY: Parameter error"s },
        { ERR_DLL_NOT_FOUND, "GDXCOPY: DLL not found"s },
        { ERR_CREATE_DIR, "GDXCOPY: Cannot create directory"s },
        { ERR_FILE_OPEN, "GDXCOPY: File open failed"s },
        { ERR_FILE_WRITE, "GDXCOPY: Cannot open file for write"s },
        { ERR_UEL_LENGTH, "GDXCOPY: UEL length exceeds maximum"s },
        { ERR_UEL_REGISTER, "GDXCOPY: Cannot register UELs"s },
        { ERR_EXPL_TEXT, "GDXCOPY: Cannot save explanatory text"s },
        { ERR_DIMENSION, "GDXCOPY: Dimension exceeds maximum"s },
        { ERR_WRITE_SYMBOL, "GDXCOPY: Error writing symbol"s },
        { ERR_CLOSE_FILE, "GDXCOPY: Error closing file"s },
        { ERR_CANNOT_DELETE, "GDXCOPY: Cannot delete file"s },
        { ERR_CANNOT_RENAME, "GDXCOPY: Cannot rename file"s } };

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
int TGXFileObj::gdxErrorStr( int ErrNr, char *ErrMsg ) const
{
   return gdxErrorStrStatic( ErrNr, ErrMsg );
}

int TGXFileObj::gdxErrorStrStatic( int ErrNr, char *ErrMsg )
{
   const auto it = errorCodeToStr.find( ErrNr );
   utils::assignStrToBuf( it == errorCodeToStr.end() ? SysErrorMessage( ErrNr ) : it->second, ErrMsg, GMS_SSSIZE );
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
int TGXFileObj::gdxOpenRead( const char *FileName, int &ErrNr )
{
   return gdxOpenReadXX( FileName, fmOpenRead, 0, ErrNr );
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
int TGXFileObj::gdxFileVersion( char *FileStr, char *ProduceStr ) const
{
   utils::assignStrToBuf( FileSystemID, FileStr, GMS_SSSIZE );
   utils::assignStrToBuf( FProducer, ProduceStr, GMS_SSSIZE );
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
int TGXFileObj::gdxFindSymbol( const char *SyId, int &SyNr )
{
   if( SyId == "*"s )
   {
      SyNr = 0;
      return true;
   }
   if( NameList )
   {
      SyNr = NameList->IndexOf( SyId );
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
int TGXFileObj::gdxDataReadStr( char **KeyStr, double *Values, int &DimFrst )
{
   if( ( TraceLevel >= TraceLevels::trl_all || fmode != fr_str_data ) && !CheckMode( "DataReadStr"s, fr_str_data ) )
      return false;
   if( !DoRead( Values, DimFrst ) )
   {
      gdxDataReadDone();
      return false;
   }
   else
   {
      for( int D{}; D < FCurrentDim; D++ )
      {
         int LED{ LastElem[D] };
         if( LED >= 1 && LED <= ( UELTable ? UELTable->size() : 0 ) )
         {
#if defined( _WIN32 )
            utils::assignPCharToBuf( ( *UELTable )[LED], KeyStr[D], GMS_UEL_IDENT_SIZE );
#else
            std::strcpy( KeyStr[D], ( *UELTable )[LED] );
#endif
         }
         else
            std::snprintf( KeyStr[D], GMS_UEL_IDENT_SIZE, "%s%d", BADUEL_PREFIX.c_str(), LED );
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
int TGXFileObj::gdxDataReadDone()
{
   static const TgxModeSet AllowedMode{ fr_init, fr_raw_data, fr_map_data, fr_mapr_data, fr_str_data, fr_slice };
   SortList = nullptr;
   CurSyPtr = nullptr;
   if( !MajorCheckMode( "DataReadDone"s, AllowedMode ) )
   {
      fmode = fr_init;
      return false;
   }
   if( fmode == fr_slice )
   {
      for( int D{}; D < GLOBAL_MAX_INDEX_DIM; D++ )
      {
         SliceIndxs[D] = std::nullopt;
         SliceRevMap[D] = std::nullopt;
      }
   }
   if( NrMappedAdded )
   {
      int HighestIndex = UELTable->UsrUel2Ent->GetHighestIndex();
      for( int N{ HighestIndex }; N >= HighestIndex - NrMappedAdded + 1; N-- )
      {
         assert( N >= 1 && "Wrong entry number" );
         int EN{ UELTable->UsrUel2Ent->GetMapping( N ) };// nr in ueltable
#ifndef NDEBUG
         int d{ UELTable->GetUserMap( EN ) };
         assert( ( d == -1 || d == N ) && "Mapped already" );
#endif
         UELTable->SetUserMap( EN, N );// map to user entry
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
int TGXFileObj::gdxSymbolInfo( int SyNr, char *SyId, int &Dim, int &Typ )
{
   if( !SyNr )
   {
      SyId[0] = '*';
      SyId[1] = '\0';
      Dim = 1;
      Typ = dt_set;
      return true;
   }

   if( NameList && !NameList->empty() && SyNr > 0 && SyNr <= NameList->size() )
   {
      const auto *sym = *NameList->GetObject( SyNr );
      utils::assignPCharToBuf( NameList->GetString( SyNr ), SyId, GMS_SSSIZE );
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
int TGXFileObj::gdxDataReadStrStart( int SyNr, int &NrRecs )
{
   auto XDomains{ utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>( DOMC_UNMAPPED ) };
   NrRecs = PrepareSymbolRead( "DataReadStrStart"s, SyNr, XDomains.data(), fr_str_data );
   return NrRecs >= 0;
}

// same as std::string::substr but silent when offset > input size
static inline std::string_view substr( const std::string_view s, int offset, int len )
{
   return ( s.empty() || offset > (int) s.size() - 1 ) ? std::string_view{} : s.substr( offset, len );
}

int TGXFileObj::gdxOpenReadXX( const char *Afn, int filemode, int ReadMode, int &ErrNr )
{
   if( fmode != f_not_open )
   {
      ErrNr = ERR_FILEALREADYOPEN;
      return false;
   }
   MajContext = "OpenRead"s;
   TraceLevel = defaultTraceLevel;
   fmode = f_not_open;
   ReadPtr = std::nullopt;
   InitErrors();

   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
   {
      std::cout << "gdxOpenRead("s << Afn << ")\n"s;
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

   if( Afn[0] == '\0' )
   {
      ErrNr = ERR_NOFILE;
      return FileNoGood();
   }
   FFile = std::make_unique<TMiBufferedStreamDelphi>( Afn, filemode );
   ErrNr = FFile->GetLastIOResult();
   if( ErrNr ) return FileNoGood();
   if( FFile->GoodByteOrder() )
   {
      ErrNr = ERR_BADDATAFORMAT;
      return FileNoGood();
   }
   if( ErrorCondition( FFile->ReadByte() == gdxHeaderNr, ERR_OPEN_FILEHEADER ) ||
       ErrorCondition( utils::sameText( FFile->ReadString(), gdxHeaderId ), ERR_OPEN_FILEMARKER ) ) return FileErrorNr();
   VersionRead = FFile->ReadInteger();
   if( ErrorCondition( VersionRead <= VERSION, ERR_OPEN_FILEVERSION ) ) return FileErrorNr();

   int Compr{ VersionRead <= 5 ? 0 : FFile->ReadInteger() };
   DoUncompress = Compr > 0;
   if( DoUncompress && !FFile->GetCanCompress() )
   {
      ErrNr = ERR_ZLIB_NOT_FOUND;
      return FileNoGood();
   }

   fComprLev = Compr;
   FileSystemID = FFile->ReadString();
   FProducer = FFile->ReadString();
   FProducer2.clear();

   MajorIndexPosition = FFile->GetPosition();
   if( ErrorCondition( FFile->ReadInteger() == MARK_BOI, ERR_OPEN_BOI ) ) return FileErrorNr();
   int64_t AcronymPos{}, DomStrPos{}, SymbPos{}, UELPos{}, SetTextPos{};

   if( VersionRead <= 5 )
   {
      // NOTE: Not covered by unit tests yet.
      SymbPos = FFile->ReadInteger();
      UELPos = FFile->ReadInteger();
      SetTextPos = FFile->ReadInteger();
      NextWritePosition = FFile->ReadInteger();
   }
   else
   {
      SymbPos = FFile->ReadInt64();
      UELPos = FFile->ReadInt64();
      SetTextPos = FFile->ReadInt64();
      if( VersionRead >= 7 )
      {
         AcronymPos = FFile->ReadInt64();
         NextWritePosition = FFile->ReadInt64();
         DomStrPos = FFile->ReadInt64();
      }
   }
   // reading symbol table
   FFile->SetCompression( DoUncompress );
   FFile->SetPosition( SymbPos );
   if( ErrorCondition( FFile->ReadString() == MARK_SYMB, ERR_OPEN_SYMBOLMARKER1 ) ) return FileErrorNr();
   int NrElem{ FFile->ReadInteger() };
   NameList = std::make_unique<TNameList>();
   NameList->OneBased = true;
   AcronymList = std::make_unique<TAcronymList>();
   FilterList = std::make_unique<TFilterList>();
   const int NrElemsOfSym = NrElem;
   for( int N{ 1 }; N <= NrElemsOfSym; N++ )
   {
      std::string S{ FFile->ReadString() };
      CurSyPtr = new TgdxSymbRecord{};
      CurSyPtr->SPosition = VersionRead <= 5 ? FFile->ReadInteger() : FFile->ReadInt64();
      CurSyPtr->SDim = FFile->ReadInteger();
      uint8_t B{ FFile->ReadByte() };
      CurSyPtr->SDataType = static_cast<gdxSyType>( B );
      CurSyPtr->SUserInfo = FFile->ReadInteger();
      CurSyPtr->SDataCount = FFile->ReadInteger();
      CurSyPtr->SErrors = FFile->ReadInteger();
      B = FFile->ReadByte();
      CurSyPtr->SSetText = B;
      utils::assignStrToBuf( FFile->ReadString(), CurSyPtr->SExplTxt.data(), GMS_SSSIZE );
      CurSyPtr->SIsCompressed = VersionRead > 5 && FFile->ReadByte();
      CurSyPtr->SDomSymbols = nullptr;
      CurSyPtr->SCommentsList = std::nullopt;
      if( VersionRead >= 7 )
      {
         if( FFile->ReadByte() )
         {
            CurSyPtr->SDomSymbols = std::unique_ptr<int[]>{ new int [CurSyPtr->SDim] };
            for( int D{}; D < CurSyPtr->SDim; D++ )
               CurSyPtr->SDomSymbols[D] = FFile->ReadInteger();
         }
         NrElem = FFile->ReadInteger();
         if( NrElem > 0 )
         {
            CurSyPtr->SCommentsList = std::make_optional<TCommentsList>();
            for( ; NrElem > 0; NrElem-- )
            {
               const auto s{ FFile->ReadString() };
               CurSyPtr->SCommentsList->Add( s.c_str(), s.length() );
            }
         }
      }
      CurSyPtr->SSetBitMap = nullptr;
      CurSyPtr->SDomStrings = nullptr;
      CurSyPtr->SSyNr = NameList->StoreObject( S.c_str(), S.length(), CurSyPtr );
   }
   if( ErrorCondition( FFile->ReadString() == MARK_SYMB, ERR_OPEN_SYMBOLMARKER2 ) ) return FileErrorNr();

   // reading UEL table
   FFile->SetCompression( DoUncompress );
   FFile->SetPosition( UELPos );
   UELTable = std::make_unique<UELTableImplChoice>();

   if( ErrorCondition( FFile->ReadString() == MARK_UEL, ERR_OPEN_UELMARKER1 ) )
      return FileErrorNr();

   NrElem = FFile->ReadInteger();
   //bug for pre 2002
   if( substr( FileSystemID, 15, 4 ) == "2001"s ) NrElem--;

   while( UELTable->size() < NrElem )
   {
      auto s{ FFile->ReadString() };
      UELTable->StoreObject( s.c_str(), s.length(), -1 );
   }
   UelCntOrig = UELTable->size();// needed when reading universe

   if( ErrorCondition( FFile->ReadString() == MARK_UEL, ERR_OPEN_UELMARKER2 ) ) return FileErrorNr();
   if( ReadMode % 2 == 0 )
   {// reading text table
      FFile->SetCompression( DoUncompress );
      FFile->SetPosition( SetTextPos );
      SetTextList = std::make_unique<TSetTextList>();
      SetTextList->OneBased = false;
      if( ErrorCondition( FFile->ReadString() == MARK_SETT, ERR_OPEN_TEXTMARKER1 ) ) return FileErrorNr();
      NrElem = FFile->ReadInteger();
      SetTextList->SetCapacity( NrElem );
      for( int N{}; N < NrElem; N++ )
      {
         auto s{ FFile->ReadString() };
         int TextNum{ SetTextList->Add( s.c_str(), s.length() ) };
         if( TextNum != N )
         {// duplicates stored in GDX file, e.g. empty string
            // NOTE: Not covered by unit tests yet.
            if( !MapSetText )
            {
               MapSetText = std::unique_ptr<int[]>{ new int[NrElem] };
               for( int D{}; D < N; D++ )
                  MapSetText[D] = D;
            }
            MapSetText[N] = TextNum;
         }
      }
      if( ErrorCondition( FFile->ReadString() == MARK_SETT, ERR_OPEN_TEXTMARKER2 ) ) return FileErrorNr();
   }
   if( VersionRead >= 7 )
   {
      FFile->SetCompression( DoUncompress );
      FFile->SetPosition( AcronymPos );
      if( ErrorCondition( FFile->ReadString() == MARK_ACRO, ERR_OPEN_ACROMARKER1 ) ) return FileErrorNr();
      AcronymList->LoadFromStream( *FFile );
      if( ErrorCondition( FFile->ReadString() == MARK_ACRO, ERR_OPEN_ACROMARKER2 ) ) return FileErrorNr();
   }

   DomainStrList = std::make_unique<TDomainStrList>();
   DomainStrList->OneBased = true;
   if( VersionRead >= 7 && DomStrPos )
   {
      FFile->SetCompression( DoUncompress );
      FFile->SetPosition( DomStrPos );
      if( ErrorCondition( FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER1 ) ) return FileErrorNr();
      DomainStrList->LoadFromStream<TXStreamDelphi>( *FFile );
      if( ErrorCondition( FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER2 ) ) return FileErrorNr();
      while( true )
      {
         int SyNr = FFile->ReadInteger();
         if( SyNr <= 0 ) break;
         const auto sym = *NameList->GetObject( SyNr );
         sym->SDomStrings = std::unique_ptr<int[]>{ new int[sym->SDim] };
         for( int D{}; D < sym->SDim; D++ )
            sym->SDomStrings[D] = FFile->ReadInteger();
      }
      if( ErrorCondition( FFile->ReadString() == MARK_DOMS, ERR_OPEN_DOMSMARKER3 ) ) return FileErrorNr();
   }

   LastError = ERR_NOERROR;
   gdxResetSpecialValues();
   fmode = fr_init;
   fstatus = stat_read;
   FFile->SetCompression( false );
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
int TGXFileObj::gdxAddAlias( const char *Id1, const char *Id2 )
{
   if( !MajorCheckMode( "AddAlias"s, AnyWriteMode ) ) return false;
   int SyNr1{ !strcmp( Id1, "*" ) ? std::numeric_limits<int>::max() : NameList->IndexOf( Id1 ) };
   int SyNr2{ !strcmp( Id2, "*" ) ? std::numeric_limits<int>::max() : NameList->IndexOf( Id2 ) };
   if( ErrorCondition( ( SyNr1 >= 0 ) != ( SyNr2 >= 0 ), ERR_ALIASSETEXPECTED ) ) return false;
   int SyNr;
   const char *AName;
   if( SyNr1 > 0 )
   {
      SyNr = SyNr1;
      AName = Id2;
   }
   else
   {
      SyNr = SyNr2;
      AName = Id1;
   }
   if( SyNr == std::numeric_limits<int>::max() ) SyNr = 0;
   else if( ErrorCondition( utils::in( ( *NameList->GetObject( SyNr ) )->SDataType, dt_set, dt_alias ), ERR_ALIASSETEXPECTED ) )
      return false;
   if( !IsGoodNewSymbol( AName ) ) return false;
   auto SyPtr = new TgdxSymbRecord{};
   // NOTE: SSyNr not set correctly for alias! (was also like this in original P3 implementation of GDX)
   SyPtr->SDataType = dt_alias;
   SyPtr->SUserInfo = SyNr;
   if( !SyNr )
   {
      SyPtr->SDim = 1;
      utils::assignStrToBuf( "Aliased with *"s, SyPtr->SExplTxt.data() );
   }
   else
   {
      SyPtr->SDim = ( *NameList->GetObject( SyNr ) )->SDim;
      utils::assignStrToBuf( "Aliased with "s + NameList->GetString( SyNr ), SyPtr->SExplTxt.data() );
   }
   NameList->AddObject( AName, std::strlen( AName ), SyPtr );
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
int TGXFileObj::gdxAddSetText( const char *Txt, int &TxtNr )
{
   if( !SetTextList || ( TraceLevel >= TraceLevels::trl_all && !CheckMode( "AddSetText" ) ) )
   {
      TxtNr = 0;
      return false;
   }
   static std::array<char, GMS_SSSIZE> s;
   utils::assignPCharToBuf( Txt, s.data(), GMS_SSSIZE );
   const int slen{ MakeGoodExplText( s.data() ) };
   TxtNr = SetTextList->Add( s.data(), slen );
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
int TGXFileObj::gdxDataErrorCount() const
{
   return !ErrorList ? 0 : ErrorList->GetCount();
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
int TGXFileObj::gdxDataErrorRecord( int RecNr, int *KeyInt, double *Values )
{
   int res{ gdxDataErrorRecordX( RecNr, KeyInt, Values ) };
   if( res )
   {
      for( int D{}; D < ErrorList->GetDimension(); D++ )
      {
         if( KeyInt[D] < 0 ) KeyInt[D] = -KeyInt[D];
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
int TGXFileObj::gdxDataErrorRecordX( int RecNr, int *KeyInt, double *Values )
{
   static const TgxModeSet AllowedModes{ fr_init, fw_init, fr_map_data, fr_mapr_data, fw_raw_data, fw_map_data, fw_str_data };
   if( ( TraceLevel >= TraceLevels::trl_all || !utils::in( fmode, AllowedModes ) ) && !CheckMode( "DataErrorRecord", AllowedModes ) )
      return false;

   if( ErrorList )
   {
      if( RecNr < 1 || RecNr > ErrorList->GetCount() )
      {
         ReportError( ERR_BADERRORRECORD );
      }
      else
      {
         ErrorList->GetRecord( RecNr - 1, KeyInt, Values );
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
int TGXFileObj::gdxDataReadRaw( int *KeyInt, double *Values, int &DimFrst )
{
   if( ( TraceLevel >= TraceLevels::trl_all || fmode != fr_raw_data ) && !CheckMode( "DataReadRaw"s, fr_raw_data ) ) return false;
   if( !DoRead( Values, DimFrst ) ) gdxDataReadDone();
   else
   {
      std::memcpy( KeyInt, LastElem.data(), FCurrentDim * sizeof( int ) );
      if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
      {
         std::cout << "DataReadRaw index: "s;
         for( int D{}; D < FCurrentDim; D++ )
            std::cout << std::to_string( KeyInt[D] ) << ( D + 1 < FCurrentDim ? ","s : ""s );
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
int TGXFileObj::gdxDataReadRawStart( int SyNr, int &NrRecs )
{
   auto XDomains{ utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>( DOMC_UNMAPPED ) };
   NrRecs = PrepareSymbolRead( "DataReadRawStart"s, SyNr, XDomains.data(), fr_raw_data );
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
int TGXFileObj::gdxDataWriteRaw( const int *KeyInt, const double *Values )
{
   if( fmode == fw_dom_raw ) fmode = fw_raw_data;
   if( ( TraceLevel >= TraceLevels::trl_some || fmode != fw_raw_data ) && !CheckMode( "DataWriteRaw"s, fw_raw_data ) ) return false;
   if( DoWrite( KeyInt, Values ) ) return true;
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
int TGXFileObj::gdxDataWriteRawStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ,
                                      int UserInfo )
{
   if( !PrepareSymbolWrite( "DataWriteRawStart"s, SyId, ExplTxt, Dimen, Typ, UserInfo ) ) return false;
   // we overwrite the initialization
   std::fill_n( MinElem.begin(), FCurrentDim, 0 );// no assumptions about the range for a uel
   std::fill_n( MaxElem.begin(), FCurrentDim, std::numeric_limits<int>::max() );
   InitDoWrite( -1 );
   fmode = fw_dom_raw;
   return true;
}

// Brief:
//  Returns the number of errors
// Returns:
//  Total number of errors encountered
// See Also:
//   gdxGetLastError
int TGXFileObj::gdxErrorCount() const
{
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
int TGXFileObj::gdxGetElemText( int TxtNr, char *Txt, int &Node )
{
   Node = 0;
   if( !SetTextList )
   {
      Txt[0] = '\0';
      return false;
   }
   if( TraceLevel >= TraceLevels::trl_all && !CheckMode( "GetElemText" ) )
      return false;
   if( TxtNr < 0 || TxtNr >= SetTextList->size() )
   {
      utils::assignStrToBuf( BADStr_PREFIX + std::to_string( TxtNr ), Txt, GMS_SSSIZE );
      return false;
   }
   else
   {
      utils::assignPCharToBuf( SetTextList->GetName( TxtNr ), Txt, GMS_SSSIZE );
      auto obj = SetTextList->GetObject( TxtNr );
#ifdef TXSPOOL_LEGACY
      Node = obj ? (int) reinterpret_cast<long long>( obj ) : 0;
#else
      Node = obj ? (int) *obj : 0;
#endif
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
int TGXFileObj::gdxGetLastError()
{
   if( !FFile )
   {
      int le{ LastError };
      LastError = ERR_NOERROR;
      return le;
   }
   else
   {
      int res{ FFile->GetLastIOResult() };
      if( res == ERR_NOERROR )
      {
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
int TGXFileObj::gdxGetSpecialValues( double *AVals )
{
   AVals[sv_valund] = intlValueMapDbl[vm_valund];
   AVals[sv_valna] = intlValueMapDbl[vm_valna];
   AVals[sv_valpin] = intlValueMapDbl[vm_valpin];
   AVals[sv_valmin] = intlValueMapDbl[vm_valmin];
   AVals[sv_valeps] = intlValueMapDbl[vm_valeps];
   AVals[sv_acronym] = Zvalacr;

   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
   {
      std::array svNames{ "undef"s, "na"s, "posinf"s, "min"s, "eps"s };
      std::array svIndices{ sv_valund, sv_valna, sv_valpin, sv_valmin, sv_valeps };
      for( int i = 0; i < (int) svNames.size(); i++ )
         std::cout << svNames[i] << "="s << AVals[svIndices[i]] << '\n';
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
int TGXFileObj::gdxSetSpecialValues( const double *AVals )
{
   TIntlValueMapDbl tmpDbl{ intlValueMapDbl };

   tmpDbl[vm_valund] = AVals[sv_valund];
   tmpDbl[vm_valna] = AVals[sv_valna];
   tmpDbl[vm_valpin] = AVals[sv_valpin];
   tmpDbl[vm_valmin] = AVals[sv_valmin];
   tmpDbl[vm_valeps] = AVals[sv_valeps];

   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
   {
      std::array svNames{ "undef"s, "na"s, "posinf"s, "min"s, "eps"s };
      std::array svIndices{ sv_valund, sv_valna, sv_valpin, sv_valmin, sv_valeps };
      for( int i = 0; i < (int) svNames.size(); i++ )
         std::cout << svNames[i] << "="s << AVals[svIndices[i]] << '\n';
   }

   TIntlValueMapI64 tmpI64;
   copyIntlMapDblToI64( tmpDbl, tmpI64 );

   // check for duplicates using the int64 version of the map
   const TgdxIntlValTyp stopper = vm_valeps;
   for( int iv1{ vm_valund }; iv1 <= stopper; iv1++ )
   {
      for( int iv2{ iv1 + 1 }; iv2 <= stopper; iv2++ )
      {
         if( tmpI64[iv1] == tmpI64[iv2] )
         {
            // NOTE: Not covered by unit tests yet.
            ReportError( ERR_DUPLICATESPECVAL );
            return false;
         }
      }
   }

   intlValueMapDbl = tmpDbl;
   readIntlValueMapDbl = intlValueMapDbl;

   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
   {
      std::cout << "Read dump, readIntlValueMapDbl\n";
      std::array svNames{ "undef"s, "na"s, "posinf"s, "min"s, "eps"s };
      std::array svIndices{ sv_valund, sv_valna, sv_valpin, sv_valmin, sv_valeps };
      for( int i = 0; i < (int) svNames.size(); i++ )
         std::cout << svNames[i] << "="s << readIntlValueMapDbl[svIndices[i]] << '\n';
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
int TGXFileObj::gdxSymbolGetDomain( int SyNr, int *DomainSyNrs )
{
   if( ErrorCondition( SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX ) ) return false;
   const TgdxSymbRecord *SyPtr{ ( *NameList->GetObject( SyNr ) ) };
   for( int D{}; D < SyPtr->SDim; D++ )
      DomainSyNrs[D] = !SyPtr->SDomSymbols ? 0 : SyPtr->SDomSymbols[D];
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
int TGXFileObj::gdxSymbolGetDomainX( int SyNr, char **DomainIDs )
{
   if( ErrorCondition( !NameList->empty() && SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX ) ) return 0;
   const TgdxSymbRecord *SyPtr{ ( *NameList->GetObject( SyNr ) ) };

   for( int D{}; D < SyPtr->SDim; D++ )
   {
      DomainIDs[D][0] = '*';
      DomainIDs[D][1] = '\0';
   }

   int res{};

   if( SyPtr->SDomStrings )
   {
      for( int D{}; D < SyPtr->SDim; D++ )
         if( SyPtr->SDomStrings[D] )
            utils::assignPCharToBuf( DomainStrList->GetString( SyPtr->SDomStrings[D] ), DomainIDs[D], GMS_SSSIZE );
      res = 2;
   }
   else if( !SyPtr->SDomSymbols )
      res = 1;
   else
   {
      for( int D{}; D < SyPtr->SDim; D++ )
         if( SyPtr->SDomSymbols[D] )
            utils::assignPCharToBuf( NameList->GetString( SyPtr->SDomSymbols[D] ), DomainIDs[D], GMS_SSSIZE );
      res = 3;
   }

   if( verboseTrace && TraceLevel == TraceLevels::trl_all && utils::in( res, 2, 3 ) )
   {
      std::cout << "GetDomain SyNr="s << SyNr << '\n';
      for( int D{}; D < SyPtr->SDim; D++ )
      {
         if( res == 2 )
            std::cout << "SDomStrings["s << D << "]="s << SyPtr->SDomStrings[D] << '\n';
         else if( res == 3 )
            std::cout << "SDomSymbols["s << D << "]="s << SyPtr->SDomSymbols[D] << '\n';
         std::cout << "DomainIDs["s << D << "]="s << DomainIDs[D] << '\n';
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
int TGXFileObj::gdxSymbolDim( int SyNr )
{
   if( !SyNr ) return 1;
   return !NameList || NameList->empty() || SyNr < 1 || SyNr > NameList->size() ? -1 : ( *NameList->GetObject( SyNr ) )->SDim;
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
int TGXFileObj::gdxSymbolInfoX( int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt )
{
   if( !SyNr )
   {
      RecCnt = UelCntOrig;
      UserInfo = 0;
      utils::assignPCharToBuf( "Universe", ExplTxt, GMS_SSSIZE );
      return true;
   }
   else if( !NameList || NameList->empty() || SyNr < 1 || SyNr > NameList->size() )
   {
      RecCnt = UserInfo = 0;
      ExplTxt[0] = '\0';
      return false;
   }
   else
   {
      const auto *obj = ( *NameList->GetObject( SyNr ) );
      RecCnt = !obj->SDim ? 1 : obj->SDataCount;// scalar trick
      UserInfo = obj->SUserInfo;
      utils::assignPCharToBuf( obj->SExplTxt.data(), ExplTxt, GMS_SSSIZE );
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
int TGXFileObj::gdxSymbolSetDomain( const char **DomainIDs )
{
   static const TgxModeSet AllowedModes{ fw_dom_raw, fw_dom_map, fw_dom_str };
   if( !MajorCheckMode( "SymbolSetDomain"s, AllowedModes ) || !CurSyPtr ) return false;

   if( verboseTrace && TraceLevel == TraceLevels::trl_all )
   {
      std::cout << "SetDomain\n"s;
      for( int D{}; D < CurSyPtr->SDim; D++ )
         std::cout << "DomainID["s << D << "]="s << DomainIDs[D] << '\n';
   }

   int res{ true };
   assert( !CurSyPtr->SDomSymbols && "SymbolSetDomain" );
   CurSyPtr->SDomSymbols = std::unique_ptr<int[]>{ new int[CurSyPtr->SDim] };
   for( int D{}; D < CurSyPtr->SDim; D++ )
   {
      bool domap{ true };
      int DomSy;
      if( !std::strcmp( DomainIDs[D], "*" ) ) DomSy = 0;
      else
      {
         // NOTE: SSyNr of alias symbol objects is 0!
         DomSy = NameList->IndexOf( DomainIDs[D] );
         if( DomSy <= -1 )
         {
            ReportError( ERR_UNKNOWNDOMAIN );
            DomSy = -1;
            res = false;
         }
      }
      int SyNr;
      if( DomSy > 0 )
      {
         SyNr = DomSy;
         do {
            const auto *obj = ( *NameList->GetObject( SyNr ) );
            if( obj->SDataType == dt_set ) break;
            if( obj->SDataType == dt_alias )
            {
               SyNr = obj->SUserInfo;
               if( SyNr > 0 ) continue;
               domap = false;
               break;
            }
            ReportError( ERR_ALIASSETEXPECTED );
            res = false;
            DomSy = 0;
            break;
         } while( true );
      }
      CurSyPtr->SDomSymbols[D] = DomSy;
      if( domap && DomSy > 0 )
      {
         // this is the case for set i(i)
         if( CurSyPtr->SDim != 1 || CurSyPtr != *NameList->GetObject( DomSy ) )
         {
            const auto *thesym = *NameList->GetObject( SyNr );
            WrBitMaps[D] = thesym->SSetBitMap ? thesym->SSetBitMap.get() : nullptr;
         }
      }
   }
   switch( fmode )
   {
      case fw_dom_raw:
         fmode = fw_raw_data;
         break;
      case fw_dom_map:
         // NOTE: Not covered by unit tests yet.
         fmode = fw_map_data;
         break;
      case fw_dom_str:
         fmode = fw_str_data;
         break;
      default:
         break;
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
int TGXFileObj::gdxSymbolSetDomainX( int SyNr, const char **DomainIDs )
{
   // check for write or append only
   if( ErrorCondition( SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX ) ) return false;
   PgdxSymbRecord SyPtr = ( *NameList->GetObject( SyNr ) );

   if( verboseTrace && TraceLevel == TraceLevels::trl_all )
   {
      std::cout << "SetDomainX SyNr="s << SyNr << '\n';
      for( int D{}; D < SyPtr->SDim; D++ )
         std::cout << "DomainID["s << D << "]="s << DomainIDs[D] << '\n';
   }

   if( SyPtr->SDim > 0 )
   {
      if( !SyPtr->SDomStrings )
         SyPtr->SDomStrings = std::unique_ptr<int[]>{ new int[SyPtr->SDim] };
      for( int D{}; D < SyPtr->SDim; D++ )
      {
         const char *S{ DomainIDs[D] };
         if( S[0] == '\0' || !strcmp( S, "*" ) || !IsGoodIdent( S ) ) SyPtr->SDomStrings[D] = 0;
         else
         {
            SyPtr->SDomStrings[D] = DomainStrList->IndexOf( S );// one based
            if( SyPtr->SDomStrings[D] <= 0 )
            {
               DomainStrList->Add( S, std::strlen( S ) );
               SyPtr->SDomStrings[D] = (int) DomainStrList->size();
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
int TGXFileObj::gdxSystemInfo( int &SyCnt, int &UelCnt ) const
{
   UelCnt = UELTable ? (int) UELTable->size() : 0;
   SyCnt = NameList ? (int) NameList->size() : 0;
   return true;
}

// Brief:
//   Finish registration of unique elements
// Returns:
//   Non-zero if the operation is possible, zero otherwise
// See Also:
//   gdxUELRegisterRawStart, gdxUELRegisterMapStart, gdxUELRegisterStrStart
int TGXFileObj::gdxUELRegisterDone()
{
   static const TgxModeSet AllowedModes{ f_raw_elem, f_map_elem, f_str_elem };
   if( !MajorCheckMode( "UELRegisterDone"s, AllowedModes ) ) return false;
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
int TGXFileObj::gdxUELRegisterRaw( const char *Uel )
{
   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
      std::cout << "Uel=" << Uel << '\n';

   if( ( TraceLevel >= TraceLevels::trl_all || fmode != f_raw_elem ) && !CheckMode( "UELRegisterRaw"s, f_raw_elem ) )
      return false;

   static std::array<char, GMS_SSSIZE> svStorage;
   int svLen;
   const char *SV{ utils::trimRight( Uel, svStorage.data(), svLen ) };
   if( ErrorCondition( GoodUELString( SV, svLen ), ERR_BADUELSTR ) ) return false;
   UELTable->AddObject( SV, svLen, -1 );// should about existing mapping?
   return true;
}

// Brief:
//   Start registering unique elements in raw mode
// Returns:
//   Non-zero if the operation is possible, zero otherwise
// See Also:
//   gdxUELRegisterRaw, gdxUELRegisterDone
int TGXFileObj::gdxUELRegisterRawStart()
{
   if( !MajorCheckMode( "UELRegisterRawStart"s, fw_init ) ) return false;
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
int TGXFileObj::gdxUELRegisterStr( const char *Uel, int &UelNr )
{
   if( ( TraceLevel >= TraceLevels::trl_all || fmode != f_str_elem ) && !CheckMode( "UELRegisterStr"s, f_str_elem ) )
      return false;
   static std::array<char, GMS_SSSIZE> SVstorage;
   int svlen;
   const char *SV{ utils::trimRight( Uel, SVstorage.data(), svlen ) };
   if( ErrorCondition( GoodUELString( SV, svlen ), ERR_BADUELSTR ) ) return false;
   UelNr = UELTable->AddUsrNew( SV, svlen );
   return true;
}

// Brief:
//   Start registering unique elements in string mode
// Returns:
//   Non-zero if the operation is possible, zero otherwise
// See Also:
//   gdxUELRegisterStr, gdxUELRegisterDone
// Description:
int TGXFileObj::gdxUELRegisterStrStart()
{
   static const TgxModeSet AllowedModes{ fr_init, fw_init };
   if( !MajorCheckMode( "UELRegisterStrStart"s, AllowedModes ) ) return false;
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
int TGXFileObj::gdxUMUelGet( int UelNr, char *Uel, int &UelMap )
{
   if( UELTable && UelNr >= 1 && UelNr <= UELTable->size() )
   {
      utils::assignPCharToBuf( ( *UELTable )[UelNr], Uel );
      UelMap = UELTable->GetUserMap( UelNr );
      return true;
   }
   else
   {
      utils::assignStrToBuf( BADUEL_PREFIX + std::to_string( UelNr ), Uel );
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
int TGXFileObj::gdxUMUelInfo( int &UelCnt, int &HighMap ) const
{
   if( !FFile )
   {// AS: Use FFile != nullptr as proxy for checking open has been called before
      UelCnt = HighMap = 0;
      return false;
   }
   else
   {
      UelCnt = UELTable ? UELTable->size() : 0;
      HighMap = UELTable->UsrUel2Ent->GetHighestIndex();// highest index
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
int TGXFileObj::gdxCurrentDim() const
{
   return FCurrentDim;//need to do more checks here
}

// Brief:
//   Rename UEL OldName to NewName
// Arguments:
//   OldName: Name of an existing UEL
//   NewName: New name for the UEL
// Returns:
//   Zero if the renaming was possible; non-zero is an error indicator
int TGXFileObj::gdxRenameUEL( const char *OldName, const char *NewName )
{
   if( !UELTable ) return -1;

   int slen;
   std::array<char, GMS_SSSIZE> Sstorage;
   const char *S{ utils::trimRight( NewName, Sstorage.data(), slen ) };

   if( !GoodUELString( S, slen ) )
      return ERR_BADUELSTR;

   int oldNameLen;
   std::array<char, GMS_SSSIZE> oldNameStorage;
   int N{ UELTable->IndexOf( utils::trimRight( OldName, oldNameStorage.data(), oldNameLen ) ) };
   if( N < 0 )
      return 2;
   else if( UELTable->IndexOf( S ) >= 0 )
      return 3;
   UELTable->RenameEntry( N, S );
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
int TGXFileObj::gdxOpenReadEx( const char *FileName, int ReadMode, int &ErrNr )
{
   return gdxOpenReadXX( FileName, FileAccessMode::fmOpenRead, ReadMode, ErrNr );
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
int TGXFileObj::gdxGetUEL( int uelNr, char *Uel ) const
{
   if( !UELTable )
   {
      Uel[0] = '\0';
      return false;
   }
   int EN = UELTable->UsrUel2Ent->GetMapping( uelNr );
   if( EN >= 1 ) utils::assignPCharToBuf( ( *UELTable )[EN], Uel );
   else
      utils::assignStrToBuf( BADUEL_PREFIX + std::to_string( uelNr ), Uel );
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
int TGXFileObj::gdxDataWriteMapStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo )
{
   if( !PrepareSymbolWrite( "DataWriteMapStart"s, SyId, ExplTxt, Dimen, Typ, UserInfo ) ) return false;
   SortList = std::make_unique<LinkedDataType>( FCurrentDim, static_cast<int>( DataSize * sizeof( double ) ) );
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
int TGXFileObj::gdxDataWriteMap( const int *KeyInt, const double *Values )
{
   TIndex Keys;
   if( fmode == fw_dom_map )
      fmode = fw_map_data;
   if( TraceLevel >= TraceLevels::trl_all || fmode != fw_map_data )
   {
      if( !CheckMode( "DataWriteMap"s, fw_map_data ) ) return false;
      std::cout << "   Index =";
      for( int D{}; D < FCurrentDim; D++ )
      {
         std::cout << " " << std::to_string( KeyInt[D] );
         if( D + 1 < FCurrentDim ) std::cout << ",";
      }
   }
   for( int D{}; D < FCurrentDim; D++ )
   {
      int KD = UELTable->UsrUel2Ent->GetMapping( KeyInt[D] );
      if( KD < 0 )
      {
         ReportError( ERR_BADELEMENTINDEX );
         return false;
      }
      Keys[D] = KD;
      if( KD < MinElem[D] ) MinElem[D] = KD;
      if( KD > MaxElem[D] ) MaxElem[D] = KD;
   }
   SortList->AddItem( Keys.data(), Values );
   return true;
}

// Brief:
//   Start registering unique elements in mapped mode
// Returns:
//   Non-zero if the operation is possible, zero otherwise
// See Also:
//   gdxUELRegisterMap, gdxUELRegisterDone
// Description:
int TGXFileObj::gdxUELRegisterMapStart()
{
   static const TgxModeSet AllowedModes{ fr_init, fw_init };
   if( !MajorCheckMode( "UELRegisterMapStart"s, AllowedModes ) ) return false;
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
int TGXFileObj::gdxUELRegisterMap( int UMap, const char *Uel )
{
   int svLen;
   static std::array<char, GMS_SSSIZE> svStorage;
   const char *SV{ utils::trimRight( Uel, svStorage.data(), svLen ) };
   if( TraceLevel >= TraceLevels::trl_all || fmode != f_map_elem )
   {
      if( !CheckMode( "UELRegisterMap"s, f_map_elem ) ) return false;
      std::cout << "   Enter UEL: " << SV << " with number " << UMap << "\n";
   }
   if( ErrorCondition( GoodUELString( SV, svLen ), ERR_BADUELSTR ) ||
       ErrorCondition( UELTable->AddUsrIndxNew( SV, svLen, UMap ) >= 0, ERR_UELCONFLICT ) ) return false;
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
int TGXFileObj::gdxDataReadMapStart( int SyNr, int &NrRecs )
{
   auto XDomains = utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>( DOMC_STRICT );
   NrRecs = PrepareSymbolRead( "DataReadMapStart"s, SyNr, XDomains.data(), fr_map_data );
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
int TGXFileObj::gdxDataReadMap( [[maybe_unused]] int RecNr, int *KeyInt, double *Values, int &DimFrst )
{
   static const TgxModeSet AllowedModes{ fr_map_data, fr_mapr_data };
   if( ( TraceLevel >= TraceLevels::trl_all || !utils::in( fmode, AllowedModes ) ) && !CheckMode( "DataReadMap", AllowedModes ) ) return false;
   if( CurSyPtr && CurSyPtr->SScalarFrst )
   {
      CurSyPtr->SScalarFrst = false;
      GetDefaultRecord( Values );
      DimFrst = 0;
      return true;
   }
   if( fmode == fr_map_data )
   {
      DimFrst = 0;
      if( !ReadPtr || !SortList->GetNextRecord( &*ReadPtr, KeyInt, Values ) ) return false;
      // checking mapped values
      for( int D{}; D < FCurrentDim; D++ )
      {
         if( KeyInt[D] != PrevElem[D] )
         {
            PrevElem[D] = KeyInt[D];
            if( !DimFrst ) DimFrst = D + 1;
         }
      }
      return true;
   }

   assert( fmode == fr_mapr_data && "fr_mapr_data expected" );

   bool AddNew{}, AddError{}, BadError{};
   int FIDim{ FCurrentDim };

again:
   if( !DoRead( Values, DimFrst ) ) return false;
   if( FIDim < DimFrst ) DimFrst = FIDim;
   FIDim = FCurrentDim;
   if( DimFrst > 0 )
   {
      bool loopDone{};
      for( int D{ DimFrst - 1 }; D < FCurrentDim && !loopDone; D++ )
      {
         const auto &obj = DomainList[D];
         if( LastElem[D] < 0 )
         {
            // NOTE: Not covered by unit tests yet.
            ReportError( ERR_BADELEMENTINDEX );
            BadError = true;
            break;
         }
         int V;
         switch( obj.DAction )
         {
            case TgdxDAction::dm_unmapped:
               KeyInt[D] = LastElem[D];
               break;
            case TgdxDAction::dm_filter:
               V = UELTable->GetUserMap( LastElem[D] );
               if( obj.DFilter->InFilter( V ) ) KeyInt[D] = V;
               else
               {
                  AddError = true;
                  FIDim = D + 1;
                  loopDone = true;
               }
               break;
            case TgdxDAction::dm_strict:
               V = UELTable->GetUserMap( LastElem[D] );
               if( V >= 0 ) KeyInt[D] = V;
               else
               {
                  AddError = true;// NOTE: Not covered by unit tests yet.
                  FIDim = D + 1;
                  loopDone = true;
               }
               break;
            case TgdxDAction::dm_expand:// no filter, allow growth of domain
               // NOTE: Not covered by unit tests yet.
               {
                  int EN = LastElem[D];
                  V = UELTable->GetUserMap( EN );
                  if( V >= 0 ) KeyInt[D] = V;
                  else
                  {
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

   if( BadError ) return false;
   else if( AddError )
   {
      for( int D{}; D < FCurrentDim; D++ )
      {
         if( LastElem[D] < 0 )
         {
            // NOTE: Not covered by unit tests yet.
            ReportError( ERR_BADELEMENTINDEX );
            return false;
         }
         int V;
         switch( DomainList[D].DAction )
         {
            case TgdxDAction::dm_filter:
               V = UELTable->GetUserMap( LastElem[D] );
               if( !DomainList[D].DFilter->InFilter( V ) )
                  LastElem[D] = -LastElem[D];
               break;
            case TgdxDAction::dm_strict:
               V = UELTable->GetUserMap( LastElem[D] );
               if( V < 0 )
                  LastElem[D] = -LastElem[D];
               break;
            default:
               break;
         }
      }
      AddToErrorListDomErrs( LastElem, Values );// unmapped
      for( int D{}; D < FCurrentDim; D++ )
         if( LastElem[D] < 0 ) LastElem[D] = -LastElem[D];
      AddError = false;
      goto again;
   }

   if( AddNew )
   {
      // NOTE: Not covered by unit tests yet.
      for( int D{}; D < FCurrentDim; D++ )
      {
         int EN = KeyInt[D];
         if( EN < 0 )
         {
            int V = UELTable->NewUsrUel( -EN );
            KeyInt[D] = V;
            NrMappedAdded++;
            // look for same mapping to be issued
            for( int D2{ D + 1 }; D2 < FCurrentDim; D2++ )
               if( KeyInt[D2] == EN ) KeyInt[D2] = V;
         }
      }
   }

   // with all filtered we lost track of AFDIM
   DimFrst = 0;
   for( int D{}; D < FCurrentDim; D++ )
   {
      if( PrevElem[D] != KeyInt[D] )
      {
         PrevElem[D] = KeyInt[D];
         if( !DimFrst ) DimFrst = D + 1;
      }
   }

   return true;
}

void TGXFileObj::SetTraceLevel( TGXFileObj::TraceLevels tl )
{
   TraceLevel = tl;
}

// Summary:
//   Number of entries in the acronym table
// Arguments:
// Returns:
//   The number of entries in the acronym table
// See Also:
//    gdxAcronymSetInfo, gdxAcronymSetInfo
int TGXFileObj::gdxAcronymCount() const
{
   return !AcronymList ? 0 : static_cast<int>( AcronymList->size() );
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
int TGXFileObj::gdxAcronymGetInfo( int N, char *AName, char *Txt, int &AIndx ) const
{
   if( N < 1 || N > (int) AcronymList->size() )
   {
      AName[0] = Txt[0] = '\0';
      AIndx = 0;
      return false;
   }
   const auto &acr = ( *AcronymList )[N - 1];
   utils::assignStrToBuf( acr.AcrName, AName, GMS_SSSIZE );
   utils::assignStrToBuf( acr.AcrText, Txt, GMS_SSSIZE );
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
int TGXFileObj::gdxAcronymSetInfo( int N, const char *AName, const char *Txt, int AIndx )
{
   auto MapIsUnique = [this]( int Indx ) {
      for( int i{}; i < AcronymList->size(); i++ )
         if( ( *AcronymList )[i].AcrReadMap == Indx )
            return false;
      return true;
   };

   if( TraceLevel >= TraceLevels::trl_some )
      WriteTrace( "AcronymSetInfo: "s + AName + " index = " + std::to_string( AIndx ) );

   if( ErrorCondition( N >= 1 || N <= (int) AcronymList->size(), ERR_BADACRONUMBER ) ) return false;
   auto &obj = ( *AcronymList )[N - 1];
   if( utils::in( fmode, AnyWriteMode ) || obj.AcrAutoGen )
   {
      if( ErrorCondition( IsGoodNewSymbol( AName ), ERR_BADACRONAME ) ) return false;
      if( obj.AcrAutoGen )
      {
         assert( obj.AcrReadMap == AIndx && "gdxAcronymSetInfo" );
         obj.AcrAutoGen = false;
      }
      else if( ErrorCondition( AIndx == obj.AcrMap, ERR_BADACROINDEX ) )
         return false;

      obj.SetNameAndText( AName, Txt );
   }
   else if( obj.AcrReadMap != AIndx )
   {
      if( ErrorCondition( utils::sameTextPChar( AName, obj.AcrName.c_str() ), ERR_BADACRONAME ) ||
          ErrorCondition( MapIsUnique( AIndx ), ERR_ACRODUPEMAP ) ) return false;
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
int TGXFileObj::gdxAcronymNextNr( int nv )
{
   int res{ NextAutoAcronym };
   if( nv >= 0 ) NextAutoAcronym = nv;
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
int TGXFileObj::gdxAcronymGetMapping( int N, int &orgIndx, int &newIndx, int &autoIndex )
{
   if( TraceLevel >= TraceLevels::trl_some )
      WriteTrace( "AcronymGetMapping: N = "s + std::to_string( N ) );
   if( ErrorCondition( N >= 1 || N <= (int) AcronymList->size(), ERR_BADACRONUMBER ) ) return false;
   const auto &obj = ( *AcronymList )[N - 1];
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
int TGXFileObj::gdxFilterExists( int FilterNr )
{
   if( !MajorCheckMode( "FilterExists"s, AnyReadMode ) ) return false;
   return FilterList->FindFilter( FilterNr ) != nullptr;
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
int TGXFileObj::gdxFilterRegisterStart( int FilterNr )
{
   if( !MajorCheckMode( "FilterRegisterStart"s, fr_init ) ||
       ErrorCondition( FilterNr >= 1, ERR_BAD_FILTER_NR ) ) return false;
   CurFilter = new TDFilter{ FilterNr, UELTable->UsrUel2Ent->GetHighestIndex() };
   FilterList->AddFilter( CurFilter );
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
int TGXFileObj::gdxFilterRegister( int UelMap )
{
   if( ( TraceLevel >= TraceLevels::trl_all || fmode != fr_filter ) &&
       !CheckMode( "FilterRegister"s, fr_filter ) ) return false;
   auto &obj = *CurFilter;
   if( ErrorCondition( UelMap >= 1 && UelMap <= obj.FiltMaxUel, ERR_BAD_FILTER_INDX ) ) return false;
   int EN{ UELTable->UsrUel2Ent->GetMapping( UelMap ) };
   if( EN >= 1 ) obj.SetFilter( UelMap, true );
   else
   {
      // NOTE: Not covered by unit tests yet.
      ReportError( ERR_FILTER_UNMAPPED );
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
int TGXFileObj::gdxFilterRegisterDone()
{
   if( !MajorCheckMode( "FilterRegisterDone"s, fr_filter ) ) return false;
   fmode = fr_init;
   CurFilter->FiltSorted = true;
   if( UELTable && UELTable->GetMapToUserStatus() == TUELUserMapStatus::map_unsorted )
   {
      int LV{ -1 };
      for( int N{ 1 }; N <= UELTable->size(); N++ )
      {
         int V{ UELTable->GetUserMap( N ) };
         if( !CurFilter->InFilter( V ) ) continue;
         if( V <= LV )
         {
            // NOTE: Not covered by unit tests yet.
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
int TGXFileObj::gdxDataReadFilteredStart( int SyNr, const int *FilterAction, int &NrRecs )
{
   NrRecs = PrepareSymbolRead( "DataReadStartFiltered"s, SyNr, FilterAction, fr_map_data );
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
int TGXFileObj::gdxSetTextNodeNr( int TxtNr, int Node )
{
   if( !SetTextList || ( TraceLevel >= TraceLevels::trl_all && !CheckMode( "SetTextNodeNr" ) ) ) return false;
   auto &obj = *SetTextList;
   if( TxtNr >= 0 && TxtNr < obj.size() )
   {
#ifdef TXSPOOL_LEGACY
      if( !obj.GetObject( TxtNr ) )
      {
         obj.PutObject( TxtNr, reinterpret_cast<uint8_t *>( (long long) Node ) );
         return true;
      }
#else
      if( !*obj.GetObject( TxtNr ) )
      {
         *obj.GetObject( TxtNr ) = static_cast<uint8_t>( Node );
         return true;
      }
#endif
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
int TGXFileObj::gdxGetDomainElements( int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int &NrElem, void *UPtr )
{
   gdxGetDomainElements_DP = DP;
   if( ErrorCondition( SyNr >= 1 && SyNr <= NameList->size(), ERR_BADSYMBOLINDEX ) ) return false;
   int Dim{ ( *NameList->GetObject( SyNr ) )->SDim };
   if( !Dim || ErrorCondition( DimPos >= 1 && DimPos <= Dim, ERR_BADDIMENSION ) ) return false;
   const TDFilter *DFilter = FilterNr == DOMC_EXPAND ? nullptr : FilterList->FindFilter( FilterNr );
   if( FilterNr != DOMC_EXPAND && !DFilter )
   {
      // NOTE: Not covered by unit tests yet.
      ReportError( ERR_UNKNOWNFILTER );
      return false;
   }

   TIntegerMapping DomainIndxs;

   //-- Note: PrepareSymbolRead checks for the correct status
   TIndex XDomains = utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>( DOMC_UNMAPPED );

   // Following call also clears ErrorList
   PrepareSymbolRead( "gdxGetDomain"s, SyNr, XDomains.data(), fr_raw_data );
   int AFDim;
   std::array<double, GMS_VAL_SCALE + 1> AVals{};
   while( DoRead( AVals.data(), AFDim ) )
   {
      int RawNr{ LastElem[DimPos - 1] };
      if( DFilter )
      {
         int MapNr{ UELTable->GetUserMap( RawNr ) };
         if( !DFilter->InFilter( MapNr ) )
         {
            // NOTE: Not covered by unit tests yet.
            //Register this record as a domain error (negative value indicates domain violation)
            LastElem[DimPos - 1] = -LastElem[DimPos - 1];
            AddToErrorListDomErrs( LastElem, AVals.data() );//unmapped
            LastElem[DimPos - 1] = -LastElem[DimPos - 1];
            //do not mark this element
            continue;
         }
      }
      DomainIndxs.SetMapping( RawNr, 1 );
   }
   gdxDataReadDone();
   NrElem = 0;
   TIndex Index{};
   if( !DP )
   {// we only count
      for( int N{ 1 }; N <= DomainIndxs.GetHighestIndex(); N++ )
         if( DomainIndxs.GetMapping( N ) == 1 )
            NrElem++;
   }
   else
   {//should we have an option to return indices in Raw order or in Mapped order?
      TTblGamsDataImpl<int> SortL{ 1, sizeof( int ) };
      for( int N{ 1 }; N <= DomainIndxs.GetHighestIndex(); N++ )
      {
         if( DomainIndxs.GetMapping( N ) == 1 )
         {
            NrElem++;
            Index.front() = UELTable->NewUsrUel( N );
            SortL.AddRecord( Index.data(), &N );
         }
      }
      SortL.Sort();
      int RawNr;
      for( int N{}; N < SortL.GetCount(); N++ )
      {
         SortL.GetRecord( N, Index.data(), &RawNr );
         gdxGetDomainElements_DP_FC( RawNr, Index.front(), UPtr );
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
int TGXFileObj::gdxSetTraceLevel( int N, const char *s )
{
   if( N <= 0 ) TraceLevel = TraceLevels::trl_none;
   else
   {
      switch( N )
      {
         case 1:
            TraceLevel = TraceLevels::trl_errors;
            break;
         case 2:
            TraceLevel = TraceLevels::trl_some;
            break;
         default:
            TraceLevel = TraceLevels::trl_all;
            break;
      }
      TraceStr = s;
   }
   //!! GetStdHandle(STD_OUTPUT_HANDLE) <> INVALID_HANDLE_VALUE;
   if( TraceLevel > TraceLevels::trl_errors )
   {
      std::cout << std::endl;
      WriteTrace( "Tracing at level "s + std::to_string( (int) TraceLevel ) );
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
int TGXFileObj::gdxAcronymAdd( const char *AName, const char *Txt, int AIndx )
{
   for( int N{}; N < (int) AcronymList->size(); N++ )
   {
      const auto &obj = ( *AcronymList )[N];
      if( utils::sameTextPChar( obj.AcrName.c_str(), AName ) )
      {
         if( ErrorCondition( obj.AcrMap == AIndx, ERR_ACROBADADDITION ) ) return -1;
         return N;
      }
      if( ErrorCondition( obj.AcrMap != AIndx, ERR_ACROBADADDITION ) ) return -1;
   }
   int res{ AcronymList->AddEntry( AName, Txt, AIndx ) };
   ( *AcronymList )[res].AcrReadMap = AIndx;
   res++;// one based for the user
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
int TGXFileObj::gdxAcronymIndex( double V ) const
{
   return V < Zvalacr ? 0 : static_cast<int>( std::round( V / Zvalacr ) );
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
int TGXFileObj::gdxAcronymName( double V, char *AName )
{
   int Indx{ gdxAcronymIndex( V ) };
   //not an acronym
   if( Indx <= 0 ) AName[0] = '\0';
   else
   {
      int N{ AcronymList->FindEntry( Indx ) };
      utils::assignStrToBuf( N < 0 ? "UnknownAcronym"s + std::to_string( Indx ) : ( *AcronymList )[N].AcrName, AName, GMS_SSSIZE );
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
double TGXFileObj::gdxAcronymValue( int AIndx ) const
{
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
int TGXFileObj::gdxAutoConvert( int nv )
{
   int res{ AutoConvert };
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
int TGXFileObj::gdxGetDLLVersion( char *V ) const
{
   utils::assignStrToBuf( auditLine, V, GMS_SSSIZE );
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
int TGXFileObj::gdxFileInfo( int &FileVer, int &ComprLev ) const
{
   switch( fstatus )
   {
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
int TGXFileObj::gdxDataReadSliceStart( int SyNr, int *ElemCounts )
{
   //-- Note: PrepareSymbolRead checks for the correct status
   TgdxUELIndex XDomains = utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>( DOMC_UNMAPPED );
   SliceSyNr = SyNr;
   PrepareSymbolRead( "DataReadSliceStart"s, SliceSyNr, XDomains.data(), fr_raw_data );

   memset( ElemCounts, 0, sizeof( int ) * GLOBAL_MAX_INDEX_DIM );

   TgdxValues Values;
   int FDim;
   for( int D{}; D < FCurrentDim; D++ )
   {
      SliceIndxs[D] = std::make_optional<TIntegerMapping>();
      SliceRevMap[D] = std::make_optional<TIntegerMapping>();
   }
   while( DoRead( Values.data(), FDim ) )
      for( int D{}; D < FCurrentDim; D++ )
         SliceIndxs[D]->SetMapping( LastElem[D], 1 );

   gdxDataReadDone();

   for( int D{}; D < FCurrentDim; D++ )
   {
      auto &obj = *SliceIndxs[D];
      int Cnt{};
      for( int N{}; N <= obj.GetHighestIndex(); N++ )
      {
         if( obj.GetMapping( N ) >= 0 )
         {
            obj.SetMapping( N, Cnt );// we keep it zero based
            SliceRevMap[D]->SetMapping( Cnt, N );
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
int TGXFileObj::gdxDataReadSlice( const char **UelFilterStr, int &Dimen, TDataStoreProc_t DP )
{
   if( !MajorCheckMode( "DataReadSlice"s, fr_slice ) )
      return false;
   bool GoodIndx{ true };
   Dimen = 0;
   TgdxUELIndex ElemNrs;
   for( int D{}; D < FCurrentDim; D++ )
   {
      SliceElems[D] = UelFilterStr[D];
      if( !std::strlen( UelFilterStr[D] ) )
      {
         ElemNrs[D] = -1;
         Dimen++;
      }
      else
      {
         // NOTE: Not covered by unit tests yet.
         ElemNrs[D] = UELTable->IndexOf( UelFilterStr[D] );
         if( ElemNrs[D] < 0 ) GoodIndx = false;
      }
   }
   fmode = fr_init;
   if( !GoodIndx ) return false;
   TgdxUELIndex XDomains = utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>( DOMC_UNMAPPED );
   PrepareSymbolRead( "DataReadSlice"s, SliceSyNr, XDomains.data(), fr_slice );
   TgdxValues Values;
   TgdxUELIndex HisIndx;
   int FDim;
   while( DoRead( Values.data(), FDim ) )
   {
      GoodIndx = true;
      int HisDim{};
      for( int D{}; D < FCurrentDim; D++ )
      {
         if( ElemNrs[D] == -1 )
            HisIndx[HisDim++] = SliceIndxs[D]->GetMapping( LastElem[D] );
         else if( ElemNrs[D] != LastElem[D] )
            GoodIndx = false;// NOTE: Not covered by unit tests yet.
      }
      if( GoodIndx ) DP( HisIndx.data(), Values.data() );
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
int TGXFileObj::gdxDataSliceUELS( const int *SliceKeyInt, char **KeyStr )
{
   if( !MajorCheckMode( "DataSliceUELS"s, fr_slice ) ) return false;
   int HisDim{};
   for( int D{}; D < FCurrentDim; D++ )
   {
      if( !SliceElems[D].empty() )// NOTE: Not covered by unit tests yet.
         utils::assignStrToBuf( SliceElems[D], KeyStr[D] );
      else
      {
         int N = SliceRevMap[D]->GetMapping( SliceKeyInt[HisDim++] );
         if( N < 0 )
         {
            // NOTE: Not covered by unit tests yet.
            KeyStr[D][0] = '?';
            KeyStr[D][1] = '\0';
         }
         else
            utils::assignPCharToBuf( ( *UELTable )[N], KeyStr[D] );
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
int64_t TGXFileObj::gdxGetMemoryUsed()
{
   int64_t res{};
   if( UELTable ) res += UELTable->MemoryUsed();
   if( SetTextList ) res += static_cast<int64_t>( SetTextList->MemoryUsed() );
   if( NameList ) res += NameList->MemoryUsed();
   if( DomainStrList ) res += DomainStrList->MemoryUsed();
   if( SortList ) res += SortList->MemoryUsed();
   if( ErrorList ) res += ErrorList->MemoryUsed();
   if( FilterList ) res += (int64_t) FilterList->MemoryUsed();
   return res;
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
int TGXFileObj::gdxMapValue( double D, int &sv )
{
   int64_t i64 = dblToI64( D );
   if( i64 == intlValueMapI64[vm_valund] ) sv = sv_valund;
   else if( i64 == intlValueMapI64[vm_valna] )
      sv = sv_valna;
   else if( i64 == intlValueMapI64[vm_valpin] )
      sv = sv_valpin;
   else if( i64 == intlValueMapI64[vm_valmin] )
      sv = sv_valmin;
   else if( i64 == intlValueMapI64[vm_valeps] )
      sv = sv_valeps;
   else
   {
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
int TGXFileObj::gdxOpenAppend( const char *FileName, const char *Producer, int &ErrNr )
{
   FProducer2 = Producer;
   AppendActive = true;
   int res{ gdxOpenReadXX( FileName, fmOpenReadWrite, 0, ErrNr ) };
   if( !res || ErrNr != 0 ) return res;
   if( VersionRead < 7 )
   {
      // NOTE: Not covered by unit tests yet.
      ReportError( ERR_FILETOOLDFORAPPEND );
      gdxClose();
      return res;
   }
   fmode = fw_init;
   fstatus = stat_write;
   FFile->SetPosition( NextWritePosition );
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
int TGXFileObj::gdxSetHasText( int SyNr )
{
   return NameList && !NameList->empty() && SyNr >= 1 && SyNr <= NameList->size() && ( *NameList->GetObject( SyNr ) )->SSetText;
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
int TGXFileObj::gdxSetReadSpecialValues( const double *AVals )
{
   readIntlValueMapDbl[vm_valund] = AVals[sv_valund];
   readIntlValueMapDbl[vm_valna] = AVals[sv_valna];
   readIntlValueMapDbl[vm_valpin] = AVals[sv_valpin];
   readIntlValueMapDbl[vm_valmin] = AVals[sv_valmin];
   readIntlValueMapDbl[vm_valeps] = AVals[sv_valeps];

   if( verboseTrace && TraceLevel >= TraceLevels::trl_all )
   {
      std::cout << "gdxSetReadSpecialValues, dump of readIntlValueMapDbl\n";
      static const std::array<std::pair<std::string, int>, 5> svNameIndexPairs{ { { "undef"s, sv_valund },
                                                                                  { "na"s, sv_valna },
                                                                                  { "posinf"s, sv_valpin },
                                                                                  { "min"s, sv_valmin },
                                                                                  { "eps"s, sv_valeps } } };
      for( const auto &[svName, svIndex]: svNameIndexPairs )
         std::cout << svName << "="s << readIntlValueMapDbl[svIndex] << '\n';
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
int TGXFileObj::gdxSymbIndxMaxLength( int SyNr, int *LengthInfo )
{
   memset( LengthInfo, 0, GLOBAL_MAX_INDEX_DIM * sizeof( int ) );

   int NrRecs;
   if( ( ( TraceLevel >= TraceLevels::trl_some || fmode != fr_init ) && !CheckMode( "SymbIndxMaxLength"s, fr_init ) ) || ( SyNr < 0 || SyNr > NameList->size() ) || !gdxDataReadRawStart( SyNr, NrRecs ) )
      return 0;

   int res{};
   if( FCurrentDim > 0 )
   {
      int UELTableCount = UELTable ? UELTable->size() : 0;// local copy for speed
      TgdxValues AVals;
      int AFDim;
      while( DoRead( AVals.data(), AFDim ) )
      {
         for( int D{ AFDim - 1 }; D < FCurrentDim; D++ )
         {
            int UEL = LastElem[D];
            if( UEL >= 1 && UEL <= UELTableCount )
            {
               auto L = static_cast<int>( std::strlen( ( *UELTable )[UEL] ) );
               if( L > LengthInfo[D] ) LengthInfo[D] = L;
            }
         }
      }
      for( int D{}; D < FCurrentDim; D++ )
         if( LengthInfo[D] > res )
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
int TGXFileObj::gdxSymbMaxLength() const
{
   int acc{};
   for( int N{ 1 }; N <= NameList->Count(); N++ )
      acc = std::max<int>( acc, (int) std::strlen( NameList->GetString( N ) ) );
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
int TGXFileObj::gdxSymbolAddComment( int SyNr, const char *Txt )
{
   if( !MajorCheckMode( "SymbolAddComment"s, AnyWriteMode ) ) return false;
   PgdxSymbRecord SyPtr;
   if( SyNr <= 0 ) SyPtr = CurSyPtr;
   else
      SyPtr = NameList && !NameList->empty() && SyNr >= 1 && SyNr <= NameList->size() ? *NameList->GetObject( SyNr ) : nullptr;
   if( !SyPtr )
   {
      ReportError( ERR_NOSYMBOLFORCOMMENT );
      return false;
   }
   if( !SyPtr->SCommentsList )
      SyPtr->SCommentsList = std::make_optional<TCommentsList>();
   SyPtr->SCommentsList->Add( Txt, std::strlen( Txt ) );
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
int TGXFileObj::gdxSymbolGetComment( int SyNr, int N, char *Txt )
{
   if( NameList && !NameList->empty() && SyNr >= 1 && SyNr <= NameList->size() )
   {
      const auto obj = *NameList->GetObject( SyNr );
      if( obj->SCommentsList && !obj->SCommentsList->empty() && N >= 1 && N <= (int) obj->SCommentsList->size() )
      {
         utils::assignPCharToBuf( ( *obj->SCommentsList )[N - 1], Txt, GMS_SSSIZE );
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
int TGXFileObj::gdxUELMaxLength() const
{
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
int TGXFileObj::gdxUMFindUEL( const char *Uel, int &UelNr, int &UelMap )
{
   UelMap = -1;
   if( !UELTable )
   {
      UelNr = -1;
      return false;
   }
   static std::array<char, GMS_SSSIZE> trimmedUelStorage;
   int trimmedUelLen;
   UelNr = UELTable->IndexOf( utils::trimRight( Uel, trimmedUelStorage.data(), trimmedUelLen ) );
   if( UelNr < 0 ) return false;
   UelMap = UELTable->GetUserMap( UelNr );
   return true;
}

int TGXFileObj::gdxStoreDomainSets() const
{
   return StoreDomainSets;
}

void TGXFileObj::gdxStoreDomainSetsSet( int x )
{
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
int TGXFileObj::gdxDataReadRawFastFilt( int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP )
{
   gdxDataReadRawFastFilt_DP = DP;
   bool res{};
   auto XDomains{ utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>( DOMC_UNMAPPED ) };
   // -- Note: PrepareSymbolRead checks for the correct status
   int NrRecs{ PrepareSymbolRead( "gdxDataReadRawFastFilt"s, SyNr, XDomains.data(), fr_raw_data ) };
   if( NrRecs >= 0 )
   {
      bool GoodIndx{ true };
      int FiltDim{};
      TgdxUELIndex ElemDim, ElemNrs;
      for( int D{}; D < FCurrentDim; D++ )
      {
         if( std::strlen( UelFilterStr[D] ) )
         {
            ElemDim[FiltDim] = D;
            ElemNrs[FiltDim] = UELTable->IndexOf( UelFilterStr[D] );
            if( ElemNrs[FiltDim] < 0 ) GoodIndx = false;
            FiltDim++;
         }
      }
      if( GoodIndx )
      {
         TgdxValues Values;
         int AFDim;
         while( DoRead( Values.data(), AFDim ) )
         {
            GoodIndx = true;
            for( int D{}; D < FiltDim; D++ )
            {
               if( LastElem[ElemDim[D]] != ElemNrs[D] )
               {
                  GoodIndx = false;
                  break;
               }
            }
            if( GoodIndx && !gdxDataReadRawFastFilt_DP_FC( LastElem.data(), Values.data(), this ) )
            {
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
int TGXFileObj::gdxDataReadRawFast( int SyNr, TDataStoreProc_t DP, int &NrRecs )
{
   auto XDomains{ utils::arrayWithValue<int, GLOBAL_MAX_INDEX_DIM>( DOMC_UNMAPPED ) };
   NrRecs = PrepareSymbolRead( "gdxDataReadRawFast"s, SyNr, XDomains.data(), fr_raw_data );
   std::array<double, GMS_VAL_SCALE + 1> AVals{};
   int AFDim;
   while( DoRead( AVals.data(), AFDim ) )
      DP( LastElem.data(), AVals.data() );
   gdxDataReadDone();
   return NrRecs >= 0;
}

// Brief:
//   Read a symbol in Raw mode using a callback procedure
// Arguments:
//   SyNr: The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
//   DP: Procedure that will be called for each data record
//   NrRecs: The number of records available for reading
//   Uptr: pointer to user memory that will be passed back with the callback
// Returns:
//   Non-zero if the operation is possible, zero otherwise
// See Also:
//   gdxDataReadRawFast
// Description:
//   Use a callback function to read a symbol in raw mode. Using a callback procedure
//   to read the data is faster because we no longer have to check the context for each
//   call to read a record.
int TGXFileObj::gdxDataReadRawFastEx( int SyNr, TDataStoreExProc_t DP, int &NrRecs, void *Uptr )
{
   //-- Note: PrepareSymbolRead checks for the correct status
   TgdxUELIndex XDomains;
   //do not know dimension yet
   std::fill( XDomains.begin(), XDomains.end(), DOMC_UNMAPPED );
   NrRecs = PrepareSymbolRead( "gdxDataReadRawFastEx"s, SyNr, XDomains.data(), fr_raw_data );
   std::array<double, valscale + 1> AVals{};
   int AFDim;
   if( gdxDataReadRawFastEx_DP_CallByRef )
   {
      // NOTE: Not covered by unit tests yet.
      TDataStoreExProc_F local_DP{ (TDataStoreExProc_F) DP };
      uInt64 local_Uptr{};
      local_Uptr.i = 0;
      local_Uptr.p = Uptr;
      while( DoRead( AVals.data(), AFDim ) )
         local_DP( LastElem.data(), AVals.data(), AFDim, local_Uptr.i );
   }
   else
   {
      while( DoRead( AVals.data(), AFDim ) )
         DP( LastElem.data(), AVals.data(), AFDim, Uptr );
   }
   gdxDataReadDone();
   return NrRecs >= 0;
}

void TGXFileObj::gdxGetDomainElements_DP_FC( int RawIndex, int MappedIndex, void *Uptr )
{
   // NOTE: Not covered by unit tests yet.
   if( gdxGetDomainElements_DP_CallByRef )
   {
      TDomainIndexProc_F local_gdxGetDomainElements_DP{ (TDomainIndexProc_F) gdxGetDomainElements_DP };
      uInt64 local_Uptr;
      local_Uptr.i = 0;
      local_Uptr.p = Uptr;
      return local_gdxGetDomainElements_DP( RawIndex, MappedIndex, local_Uptr.i );
   }
   return gdxGetDomainElements_DP( RawIndex, MappedIndex, Uptr );
}

// NOTE: Not covered by unit tests yet.
int TGXFileObj::gdxDataReadRawFastFilt_DP_FC( const int *Indx, const double *Vals, void *Uptr )
{
   if( gdxDataReadRawFastFilt_DP_CallByRef )
   {
      TDataStoreFiltProc_F local_gdxDataReadRawFastFilt_DP{ (TDataStoreFiltProc_F) gdxDataReadRawFastFilt_DP };
      uInt64 local_Uptr;
      local_Uptr.i = 0;
      local_Uptr.p = Uptr;
      return local_gdxDataReadRawFastFilt_DP( Indx, Vals, local_Uptr.i );
   }
   return gdxDataReadRawFastFilt_DP( Indx, Vals, Uptr );
}

int TUELTable::size() const
{
   return FCount;
}

bool TUELTable::empty() const
{
   return !FCount;
}

int TUELTable::GetUserMap( int i )
{
   return *GetObject( i );
}

void TUELTable::SetUserMap( int EN, int N )
{
   *GetObject( EN ) = N;
}

int TUELTable::NewUsrUel( int EN )
{
   int res = *GetObject( EN );
   if( res < 0 )
   {
      res = UsrUel2Ent->GetHighestIndex() + 1;
      *GetObject( EN ) = res;
      UsrUel2Ent->SetMapping( res, EN );
   }
   ResetMapToUserStatus();
   return res;
}

int TUELTable::AddUsrNew( const char *s, size_t slen )
{
   int EN{ AddObject( s, slen, -1 ) };
   int res{ *GetObject( EN ) };
   if( res < 0 )
   {
      res = UsrUel2Ent->GetHighestIndex() + 1;
      *GetObject( EN ) = res;
      UsrUel2Ent->SetMapping( res, EN );
   }
   ResetMapToUserStatus();
   return res;
}

int TUELTable::AddUsrIndxNew( const char *s, size_t slen, int UelNr )
{
   int EN{ AddObject( s, slen, -1 ) };
   int res{ *GetObject( EN ) };
   if( res < 0 )
   {
      res = UelNr;
      *GetObject( EN ) = res;
      UsrUel2Ent->SetMapping( res, EN );
   }
   else if( res != UelNr )
   {
      res = -1;// NOTE: Not covered by unit tests yet.
   }
   ResetMapToUserStatus();
   return res;
}

int TUELTable::GetMaxUELLength() const
{
   int maxLen{};
   for( int i{}; i < (int) Buckets.size(); i++ )
      maxLen = std::max<int>( static_cast<int>( strlen( Buckets[i]->StrP ) ), maxLen );
   return maxLen;
}

TUELTable::TUELTable()
{
   OneBased = true;
   UsrUel2Ent = std::make_unique<TIntegerMapping>();
   ResetMapToUserStatus();
}

int TUELTable::IndexOf( const char *s )
{
   return TXStrHashListImpl<int>::IndexOf( s );
}

int TUELTable::AddObject( const char *id, size_t idlen, int mapping )
{
   return TXStrHashListImpl<int>::AddObject( id, idlen, mapping );
}

int TUELTable::StoreObject( const char *id, size_t idlen, int mapping )
{
   return TXStrHashListImpl<int>::StoreObject( id, idlen, mapping );
}

const char *TUELTable::operator[]( int index ) const
{
   return GetString( index );
}

void TUELTable::RenameEntry( int N, const char *s )
{
   TXStrHashListImpl<int>::RenameEntry( N, s );
}

int TUELTable::MemoryUsed() const
{
   return (int) TXStrHashListImpl<int>::MemoryUsed() + UsrUel2Ent->MemoryUsed();
}

void TUELTable::SaveToStream( TXStreamDelphi &S )
{
   TXStrHashListImpl<int>::SaveToStream( S );
}

void TUELTable::LoadFromStream( TXStreamDelphi &S )
{
   TXStrHashListImpl<int>::LoadFromStream( S );
   if( UsrUel2Ent ) UsrUel2Ent = std::make_unique<TIntegerMapping>();
   for( int N{ 1 }; N <= FCount; N++ )
      *GetObject( N ) = -1;
   ResetMapToUserStatus();
}

TUELUserMapStatus TUELTable::GetMapToUserStatus()
{
   if( FMapToUserStatus == TUELUserMapStatus::map_unknown )
   {
      FMapToUserStatus = TUELUserMapStatus::map_sortgrow;
      bool C{ true };
      for( int N{ 1 }, LV{ -1 }; N <= size(); N++ )
      {
         int V = GetUserMap( N );
         if( V < 0 ) C = false;
         else if( V > LV )
         {
            LV = V;
            // saw some un-init/neg values
            if( !C ) FMapToUserStatus = TUELUserMapStatus::map_sorted;
         }
         else
         {
            FMapToUserStatus = TUELUserMapStatus::map_unsorted;
            break;
         }
      }
      if( FMapToUserStatus == TUELUserMapStatus::map_sortgrow && C )
         FMapToUserStatus = TUELUserMapStatus::map_sortfull;// fully strictly growing
   }
   return FMapToUserStatus;
}

void TUELTable::ResetMapToUserStatus()
{
   FMapToUserStatus = TUELUserMapStatus::map_unknown;
}

TgxModeSet::TgxModeSet( const std::initializer_list<TgxFileMode> &modes )
{
   for( const auto mode: modes )
   {
      modeActive[mode] = true;
      count++;
   }
}

inline bool TgxModeSet::contains( const TgxFileMode &mode ) const
{
   return modeActive[mode];
}

inline bool TgxModeSet::empty() const
{
   return !count;
}

TAcronymList::~TAcronymList()
{
   for( int N{}; N < FList.GetCount(); N++ )
      delete FList[N];
}

int TAcronymList::FindEntry( int Map )
{
   for( int N{}; N < FList.GetCount(); N++ )
      if( FList[N]->AcrMap == Map )
         return N;
   return -1;
}

int TAcronymList::FindName( const char *Name )
{
   for( int N{}; N < FList.GetCount(); N++ )
      if( utils::sameTextPChar( FList[N]->AcrName.c_str(), Name ) )
         return N;
   return -1;
}

int TAcronymList::AddEntry( const char *Name, const char *Text, int Map )
{
   return FList.Add( new TAcronym{ Name, Text, Map } );
}

void TAcronymList::CheckEntry( int Map )
{
   if( FindEntry( Map ) < 0 )
      AddEntry( "", "", Map );
}

void TAcronymList::SaveToStream( gmsstrm::TXStreamDelphi &S )
{
   S.WriteInteger( FList.GetCount() );
   for( int N{}; N < FList.GetCount(); N++ )
      FList[N]->SaveToStream( S );
}

void TAcronymList::LoadFromStream( gmsstrm::TXStreamDelphi &S )
{
   int Cnt{ S.ReadInteger() };
   FList.Clear();
   FList.SetCapacity( Cnt );
   while( FList.GetCount() < Cnt )
      FList.Add( new TAcronym{ S } );
}

int TAcronymList::MemoryUsed()
{
   int res{ (int) FList.MemoryUsed() + FList.GetCount() * (int) sizeof( TAcronym * ) };
   for( int N{}; N < FList.GetCount(); N++ )
      res += FList[N]->MemoryUsed();
   return res;
}

inline int TAcronymList::size() const
{
   return FList.GetCount();
}

TAcronym &TAcronymList::operator[]( int Index )
{
   return *FList[Index];
}

void TFilterList::AddFilter( TDFilter *F )
{
   for( int N{}; N < FList.size(); N++ )
   {
      if( FList[N]->FiltNumber == F->FiltNumber )
      {
         DeleteFilter( N );
         break;
      }
   }
   FList.Add( F );
}

TFilterList::~TFilterList()
{
   while( !FList.empty() )
      DeleteFilter( FList.size() - 1 );
   FList.Clear();
}

void TFilterList::DeleteFilter( int ix )
{
   delete FList[ix];
   FList.Delete( ix );
}

TDFilter *TFilterList::FindFilter( int Nr )
{
   for( int N{}; N < FList.size(); N++ )
      if( FList[N]->FiltNumber == Nr )
         return FList[N];
   return nullptr;
}

size_t TFilterList::MemoryUsed() const
{
   size_t res{ FList.MemoryUsed() + FList.size() * sizeof( TDFilter ) };
   for( int N{}; N < FList.size(); N++ )
      res += FList.GetConst( N )->MemoryUsed();
   return res;
}

void TIntegerMapping::growMapping( int F )
{
   assert( FCapacity < FMAXCAPACITY && "Already at maximum capacity: cannot grow TIntegerMapping" );
   int64_t currCap{ FCapacity }, prevCap{ currCap };
   while( F >= currCap )
   {
      int64_t delta;
      if( currCap >= 1024 * 1024 ) delta = currCap / 2;
      else if( currCap <= 0 )
         delta = 1024;
      else
         delta = currCap;
      currCap += delta;
      if( currCap > FMAXCAPACITY ) currCap = FMAXCAPACITY;
   }
   FCapacity = currCap;
   FMapBytes = (int64_t) ( FCapacity * sizeof( int ) );
   if( !PMap ) PMap = (int *) std::malloc( FMapBytes );
   else
   {
      void *p = std::realloc( PMap, FMapBytes );
      if( p ) PMap = (int *) p;
   }
   assert( PMap && "Problem allocating memory for integer mapping!" );
   if( PMap )
      std::memset( &PMap[prevCap], -1, sizeof( int ) * ( FCapacity - prevCap ) );
}

TIntegerMapping::~TIntegerMapping()
{
   std::free( PMap );
}

int TIntegerMapping::MemoryUsed() const
{
   return (int) FMapBytes;
}

int TIntegerMapping::GetHighestIndex() const
{
   return FHighestIndex;
}

int TIntegerMapping::GetMapping( int F ) const
{
   return F >= 0 && F < FCapacity ? PMap[F] : -1;
}

void TIntegerMapping::SetMapping( int F, int T )
{
   if( F >= FCapacity ) growMapping( F );
   PMap[F] = T;
   if( F > FHighestIndex ) FHighestIndex = F;
}

int TIntegerMapping::size() const
{
   return (int) FCapacity;
}

bool TIntegerMapping::empty() const
{
   return !FCapacity;
}

TAcronym::TAcronym( const char *Name, const char *Text, int Map ) : AcrName{ Name },
                                                                    AcrText{ Text },
                                                                    AcrMap{ Map }
{
   MakeGoodExplText( AcrText.data() );
}

TAcronym::TAcronym( TXStreamDelphi &S ) : AcrName{ S.ReadString() },
                                          AcrText{ S.ReadString() },
                                          AcrMap{ S.ReadInteger() }
{
}

int TAcronym::MemoryUsed() const
{
   return 2 + (int) AcrName.length() + (int) AcrText.length();
}

void TAcronym::SaveToStream( TXStreamDelphi &S ) const
{
   S.WriteString( AcrName.empty() ? "UnknownACRO"s + std::to_string( AcrMap ) : AcrName );
   S.WriteString( AcrText );
   S.WriteInteger( AcrMap );
}

void TAcronym::SetNameAndText( const char *Name, const char *Text )
{
   AcrName.assign( Name );
   AcrText.assign( Text );
   MakeGoodExplText( AcrText.data() );
}
}// namespace gdx