/*
 * GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2025 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2025 GAMS Development Corp. <support@gams.com>
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

#pragma once

// Description:
//  This unit defines the GDX Object as a C++ object.

#include "datastorage.hpp"   // for TLinkedData
#include <gclgms.h>        // for GLOBAL_MAX_INDEX_DIM, GMS_MAX_INDEX_DIM
#include "gmsdata.hpp"       // for TTblGamsData
#include "gmsobj.hpp"        // for TBooleanBitArray, TXList, TXStrings
#include "strhash.hpp"       // for TXCSStrHashList, TXStrHashList
#include "utils.hpp"
#include <array>           // for array
#include <cstdint>         // for int64_t, uint8_t
#include <cstring>         // for size_t
#include <initializer_list>// for initializer_list
#include <limits>          // for numeric_limits
#include <memory>          // for unique_ptr, allocator
#include <optional>        // for optional
#include <string>          // for string

namespace gdlib::gmsstrm
{
class TXStream;
}// namespace gdx::gmsstrm

//======================================================================================================================

namespace gdx
{

using TgdxUELIndex = std::array<int, GMS_MAX_INDEX_DIM>;
using TgdxValues = std::array<double, GMS_VAL_SCALE + 1>;

using TDomainIndexProc_t = void ( * )( int RawIndex, int MappedIndex, void *Uptr );
using TDataStoreProc_t = void ( * )( const int *Indx, const double *Vals );
using TDataStoreFiltProc_t = int ( * )( const int *Indx, const double *Vals, void *Uptr );
using TDataStoreExProc_t = int ( * )( const int *Indx, const double *Vals, const int afdim, void *Uptr );

using TDataStoreExProc_F = int ( * )( const int *Indx, const double *Vals, const int afdim, int64_t Uptr );
using TDataStoreFiltProc_F = int ( * )( const int *Indx, const double *Vals, int64_t Uptr );
using TDomainIndexProc_F = void ( * )( int RawIndex, int MappedIndex, int64_t Uptr );

const std::array<int, GMS_DT_ALIAS + 1> DataTypSize { 1, 1, 5, 5, 0 };

constexpr int DOMC_UNMAPPED = -2,// indicator for unmapped index pos
        DOMC_EXPAND = -1,        // indicator growing index pos
        DOMC_STRICT = 0;         // indicator mapped index pos

const std::string BADUEL_PREFIX = "?L__",
                  BADStr_PREFIX = "?Str__",
                  strGDXCOMPRESS = "GDXCOMPRESS",
                  strGDXCONVERT = "GDXCONVERT";

struct TDFilter {
   int FiltNumber {}, FiltMaxUel {};
   gdlib::gmsobj::TBooleanBitArray FiltMap {};
   bool FiltSorted {};

   TDFilter( int Nr, int UserHigh ) : FiltNumber { Nr },
                                      FiltMaxUel { UserHigh }
   {
   }

   ~TDFilter() = default;

   [[nodiscard]] int MemoryUsed() const
   {
      return FiltMap.MemoryUsed();
   }

   [[nodiscard]] bool InFilter( int V ) const
   {
      return V >= 0 && V <= FiltMaxUel && FiltMap.GetBit( V );
   }

   void SetFilter( int ix, bool v )
   {
      FiltMap.SetBit( ix, v );
   }
};

using TSetBitMap = gdlib::gmsobj::TBooleanBitArray;

enum class TgdxDAction : uint8_t
{
   dm_unmapped,
   dm_strict,
   dm_filter,
   dm_expand
};

struct TDomain {
   TDFilter *DFilter;
   TgdxDAction DAction;
};

using TDomainList = std::array<TDomain, GLOBAL_MAX_INDEX_DIM>;

using TCommentsList = gdlib::gmsobj::TXStrings;

struct TgdxSymbRecord {
   int SSyNr;
   int64_t SPosition;
   int SDim, SDataCount, SErrors;
   gdxSyType SDataType;
   int SUserInfo;
   bool SSetText;
   std::array<char, GMS_SSSIZE> SExplTxt;
   bool SIsCompressed;
   bool SScalarFrst;                  // not stored
   std::unique_ptr<int[]> SDomSymbols,// real domain info
                          SDomStrings;// relaxed domain info
   std::optional<TCommentsList> SCommentsList;
   std::unique_ptr<TSetBitMap> SSetBitMap;// for 1-dim sets only
};
using PgdxSymbRecord = TgdxSymbRecord *;

enum TgdxIntlValTyp : uint8_t
{// values stored internally via the indicator byte
   vm_valund,
   vm_valna,
   vm_valpin,
   vm_valmin,
   vm_valeps,
   vm_zero,
   vm_one,
   vm_mone,
   vm_half,
   vm_two,
   vm_normal,
   vm_count
};

enum TgxFileMode : uint8_t
{
   f_not_open,
   fr_init,
   fw_init,
   fw_dom_raw,
   fw_dom_map,
   fw_dom_str,
   fw_raw_data,
   fw_map_data,
   fw_str_data,
   f_raw_elem,
   f_map_elem,
   f_str_elem,
   fr_raw_data,
   fr_map_data,
   fr_mapr_data,
   fr_str_data,
   fr_filter,
   fr_slice,
   tgxfilemode_count
};

class TgxModeSet : public utils::IContainsPredicate<TgxFileMode>
{
   std::array<bool, tgxfilemode_count> modeActive {};
   uint8_t count {};

public:
   TgxModeSet( const std::initializer_list<TgxFileMode> &modes );
   ~TgxModeSet() override = default;
   [[nodiscard]] bool contains( const TgxFileMode &mode ) const override;
   [[nodiscard]] bool empty() const;
};

const TgxModeSet AnyWriteMode { fw_init, fw_dom_raw, fw_dom_map, fw_dom_str, fw_raw_data, fw_map_data, fw_str_data },
        AnyReadMode { fr_init, fr_raw_data, fr_map_data, fr_mapr_data, fr_str_data };

enum class TgdxElemSize : uint8_t
{
   sz_byte,
   sz_word,
   sz_integer
};

// N.B.: we store integers in [0..high(integer)] in TIntegerMapping, so
// FMAXCAPACITY = high(integer) + 1 is all we will ever need, and we will
// never get a request to grow any larger.  The checks and code
// in growMapping reflect this
class TIntegerMapping
{
   int64_t FCapacity {}, FMapBytes {};
   int64_t FMAXCAPACITY { std::numeric_limits<int>::max() + static_cast<int64_t>( 1 ) };
   int FHighestIndex {};
   int *PMap {};

   void growMapping( int F );

public:
   TIntegerMapping() = default;
   ~TIntegerMapping();
   [[nodiscard]] int MemoryUsed() const;
   [[nodiscard]] int GetHighestIndex() const;
   [[nodiscard]] int GetMapping( int F ) const;
   void SetMapping( int F, int T );
   [[nodiscard]] int size() const;
   [[nodiscard]] bool empty() const;
   void reset();
};

enum class TUELUserMapStatus : uint8_t
{
   map_unknown,
   map_unsorted,
   map_sorted,
   map_sortgrow,
   map_sortfull
};

template<typename T>
using TXStrHashListImpl = gdlib::strhash::TXStrHashList<T>;

template<typename T>
using TXCSStrHashListImpl = gdlib::strhash::TXCSStrHashList<T>;

class TUELTable final : public TXStrHashListImpl<int>
{
   TUELUserMapStatus FMapToUserStatus { TUELUserMapStatus::map_unknown };

public:
   std::unique_ptr<TIntegerMapping> UsrUel2Ent {};// from user uelnr to table entry
   TUELTable();
   ~TUELTable() override = default;
   [[nodiscard]] int size() const;
   [[nodiscard]] bool empty() const;
   [[nodiscard]] int GetUserMap( int i ) const;
   void SetUserMap( int EN, int N );
   int NewUsrUel( int EN );
   int AddUsrNew( const char *s, size_t slen );
   int AddUsrIndxNew( const char *s, size_t slen, int UelNr );
   [[nodiscard]] int GetMaxUELLength() const;
   int IndexOf( const char *s );
   int AddObject( const char *id, size_t idlen, int mapping );
   int StoreObject( const char *id, size_t idlen, int mapping );
   const char *operator[]( int index ) const;
   void RenameEntry( int N, const char *s );
   [[nodiscard]] int MemoryUsed() const;
   void SaveToStream( gdlib::gmsstrm::TXStream &S );
   void LoadFromStream( gdlib::gmsstrm::TXStream &S );
   TUELUserMapStatus GetMapToUserStatus();
   void ResetMapToUserStatus();
};

int MakeGoodExplText( char *s );

struct TAcronym {
   std::string AcrName, AcrText;
   int AcrMap {}, AcrReadMap { -1 };
   bool AcrAutoGen {};

   TAcronym( const char *Name, const char *Text, int Map );
   explicit TAcronym( gdlib::gmsstrm::TXStream &S );
   TAcronym() = default;
   virtual ~TAcronym() = default;
   [[nodiscard]] int MemoryUsed() const;
   void SaveToStream( gdlib::gmsstrm::TXStream &S ) const;
   void SetNameAndText( const char *Name, const char *Text );
};

class TAcronymList
{
   gdlib::gmsobj::TXList<TAcronym> FList;

public:
   TAcronymList() = default;
   ~TAcronymList();
   int FindEntry( int Map );
   int FindName( const char *Name );
   int AddEntry( const char *Name, const char *Text, int Map );
   void CheckEntry( int Map );
   void SaveToStream( gdlib::gmsstrm::TXStream &S );
   void LoadFromStream( gdlib::gmsstrm::TXStream &S );
   int MemoryUsed();
   [[nodiscard]] int size() const;
   TAcronym &operator[]( int Index );
};

class TFilterList
{
   gdlib::gmsobj::TXList<TDFilter> FList;

public:
   TFilterList() = default;
   ~TFilterList();
   void AddFilter( TDFilter *F );
   void DeleteFilter( int ix );
   TDFilter *FindFilter( int Nr );
   [[nodiscard]] size_t MemoryUsed() const;
};

using TIntlValueMapDbl = std::array<double, vm_count>;
using TIntlValueMapI64 = std::array<int64_t, vm_count>;

using LinkedDataType = gdlib::datastorage::TLinkedData<int, double>;
using LinkedDataIteratorType = gdlib::datastorage::TLinkedDataRec<int, double> *;

using TSetTextList = TXCSStrHashListImpl<int>;

using TNameList = TXStrHashListImpl<PgdxSymbRecord>;

template<typename T>
using TTblGamsDataImpl = gdlib::gmsdata::TTblGamsData<T>;

using TDomainStrList = TXStrHashListImpl<uint8_t>;

enum tvarvaltype : uint8_t
{
   vallevel,   // 1
   valmarginal,// 2
   vallower,   // 3
   valupper,   // 4
   valscale    // 5
};

extern std::string DLLLoadPath;// can be set by loader, so the "dll" knows where it is loaded from

union uInt64
{
   int64_t i;
   void *p;
};

bool IsGoodIdent( const char *S );
bool CanBeQuoted( const char *s, size_t slen );
bool GoodUELString( const char *s, size_t slen );

int ConvertGDXFile( const std::string &fn, const std::string &MyComp );

}// namespace gdx
