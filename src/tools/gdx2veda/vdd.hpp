#ifndef GDX_VDD_H
#define GDX_VDD_H

#include <string>
#include <map>
#include <fstream>
#include <cstdint>

#include "container.hpp"
#include "../library/short_string.hpp"
#include "../../gdlib/strhash.hpp"
#include "../../gdlib/gmsdata.hpp"

namespace gdx2veda
{

using bool_t = uint8_t;

// Cube data definitions

// Errors during loading
extern int NumErr;
// Veda Data base name
extern library::ShortString_t VEDAFlavor;

// Max gams dimension
constexpr int MaxGamsDim { 10 };
// Max gams suffixes
constexpr int MaxSuff { 5 };

// .LM is new: write both level and marginal
const library::Container_t<std::string>
        NameSuff { { "l", "m", "lo", "up", "lm" } };

// Max cube dimensions
constexpr int MaxDimension { 50 };
// Number of dimensions
extern int NumDimension;

// Position of the runname
// extern int RunPos;
// Position of the attributes
extern int AtrPos;

// const std::string RunDimension { "Scenario" };
const std::string AtrDimension { "Attribute" };

// EK: The following arrays are now inside the DimensionStore_t class
// extern library::Container_t<std::string> NameDimension;
// extern library::Container_t<std::string> DummyDimension;

// Max number of tuples
constexpr int MaxTuple { 20 };
// Max unique attributes
constexpr int MaxDataEntry { 500 };
// Max sets to extract text
constexpr int MaxText { 200 };
// Max subsets to extract text
constexpr int MaxSubset { 200 };

extern int NumDataEntry;

extern library::Container_t<std::string> AtrName;
extern library::Container_t<library::ShortString_t> GamsName;
extern library::Container_t<int> GamsSuff;
extern library::Container_t<int> GamsDim;

// List of attributes whose records have to be skipped if zero
extern gdlib::strhash::TXStrHashList<std::nullptr_t> SuppressZero;

extern int NumText;

extern library::Container_t<library::ShortString_t> GamsText;
extern library::Container_t<int> DummyText;
extern library::Container_t<bool_t> ExpandMap;

// Number of subset definitions (0..MaxSubset)
extern int NumSubset;

extern library::Container_t<int> DimSubset;
extern library::Container_t<library::ShortString_t> Subset;
extern library::Container_t<std::string> GamsSubset;
extern library::Container_t<int> NextSubset;
// Next set in chain
extern library::Container_t<int> NextSet;

enum class Subset_t : uint8_t
{
   NoParent,
   ParentIsIndex1,
   ParentIsIndex2
};

extern library::Container_t<Subset_t> ParentOfSubset;

// -1 or 1..NumDimension
extern int Parent;
extern library::Container_t<int> Children;
extern int NumChildren;

struct ParentChildSet_t {
   library::ShortString_t GamsSetName;
   // Dimension numbers
   int Index1 {}, Index2 {}, child {};
};

extern library::Container_t<ParentChildSet_t> ParentDimensionText;
extern int NumParentDimensionTextSets;

constexpr int MaxLiteral { 100 };

extern library::Container_t<library::ShortString_t> LiteralPool;
extern library::Container_t<int> LiteralUel;
extern int NumLiteral;

class SubsetList_t
{
   int dimension {};
   gdlib::gmsdata::TXIntList list {};
   bool assigned {};

public:
   bool AddSubset( int i, int k );
   int GetCount();
   int GetSubset( int i );
};

class DimensionStore_t
{
   gdlib::strhash::TXStrHashList<std::nullptr_t> StringStore {};
   library::Container_t<int> NameDimension { MaxDimension };
   library::Container_t<gdlib::gmsdata::TXIntList> DummyDimension { MaxDimension };
   library::Container_t<gdlib::gmsdata::TXIntList> EntryMap { MaxDataEntry };
   library::Container_t<gdlib::gmsdata::TXIntList> TextMap { MaxText };
   library::Container_t<SubsetList_t> SubsetList { MaxSubset };

   // Add new name to the stringstore. Also store its associated
   // dimension number (1..MaxDimension) and the tuple member (1..MaxTuple)
   int AddName( const library::ShortString_t &s, int dim, int tuple );

public:
   // Returns true if name is used already
   // global comparison in hash object (case insensitive)
   bool NameIsUsed( const library::ShortString_t &s );

   // Returns false if name is used already
   // else a new dimension name is added
   bool AddNewTab( const library::ShortString_t &s, int dimension );

   // Returns false if name is used already
   // else a new tuple member name is added to the dimension
   bool AddNewTupleIndex( const library::ShortString_t &s, int dimension );

   // 0 if not found
   int GetDimensionS( const library::ShortString_t &s );

   // Id but for number
   int GetDimensionI( int h );

   // i=1..NumDimension
   library::ShortString_t GetTabName( int i );

   // Size of tuple in dimension i
   // 1 : normal one dimensional thing
   int GetNumTuple( int i );

   // i=1..NumDimension
   // j=1..getNumTuple(i)
   library::ShortString_t GetTuple( int i, int j );

   // i=1..NumDimension
   // j=1..getNumTuple(i)
   int GetTupleI( int i, int j );

   // Get hash for string s
   int GetHash( const library::ShortString_t &s );

   // i = hash
   library::ShortString_t GetName( int i );

   // Get k-th hash for entry e
   int EntryList( int e, int k );

   // Add k-th hash for entry e
   // true : success
   bool AddEntryList( int e, int k, int j );

   // Add k-th hash for text t
   // true: success
   bool AddTextList( int t, int k, int j );

   // Returns the number of tuple members for DimensionText list
   int TextListLength( int t );

   // Return a tuple member of a DimensionText list
   library::ShortString_t TextList( int t, int k );

   // Return number of tuple members for dimension i
   int DimensionListLength( int t );

   // Return j-th tuple member for dimension i
   library::ShortString_t DimensionList( int i, int j );

   // Return tuple member index for string stored in position h
   int GetTupleIndex( int h );

   // Add tuplemember to a subset in dimension i
   bool AddSubset( int subset, int i, const library::ShortString_t &s );

   // Get number of tuple members for subset
   int GetSubsetTuples( int subset );

   int GetSubset( int subset, int i );
};

extern DimensionStore_t DimensionStore;

extern std::map<gdxSpecValue, std::string> SpecialValueMapping;
extern std::map<gdxSpecValue, bool> SpecialValueIsString;
extern std::map<gdxSpecValue, bool> SpecialValueIsZero;

enum class Format_t : uint8_t
{
   FormatVeda,
   FormatCSV
};

struct Options_t {
   std::string TupleSeparator;
   bool ShowAllSeparators {}, RelaxDimensionAll {};
   int ValueDim {};
   bool SetsAllowedFlag {};
   library::Container_t<bool_t> SetsAllowed;
   std::string ScenarioSet;
   Format_t Format;
};

extern Options_t Options;

extern std::ofstream f;
extern int NumLine, NumToken, rc;
extern std::string Token, OrigLine, Line, ExpandedLine;
extern char CharTab;

void VddHelp();
void LoadVdd( const std::string &fn );
void DumpVdd( const std::ofstream &f );
void ReportError( const std::string &msg );

}// namespace gdx2veda

#endif// GDX_VDD_H
