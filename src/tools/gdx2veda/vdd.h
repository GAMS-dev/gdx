#ifndef GDX_VDD_H
#define GDX_VDD_H

#include <string>
#include <map>
#include <fstream>
#include <cstdint>

#include "container.h"
#include "../library/short_string.h"

namespace gdx2veda
{

using bool_t = uint8_t;

// Cube data definitions

// Errors during loading
extern int NumErr;
// Veda Data base name
extern library::short_string VEDAFlavor;

// Max gams dimension
constexpr int MaxGamsDim { 10 };
// Max gams suffixes
constexpr int MaxSuff { 5 };

// .LM is new: write both level and marginal
const library::one_indexed_container<std::string>
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

// EK: The following arrays are now inside the TDimensionStore class
// extern library::one_indexed_container<std::string> NameDimension;
// extern library::one_indexed_container<std::string> DummyDimension;

// Max number of tuples
constexpr int MaxTuple { 20 };
// Max unique attributes
constexpr int MaxDataEntry { 500 };
// Max sets to extract text
constexpr int MaxText { 200 };
// Max subsets to extract text
constexpr int MaxSubset { 200 };

extern int NumDataEntry;

extern library::one_indexed_container<std::string> AtrName;
extern library::one_indexed_container<library::short_string> GamsName;
extern library::one_indexed_container<int> GamsSuff;
extern library::one_indexed_container<int> GamsDim;

// List of attributes whose records have to be skipped if zero
// TODO: TXStrHashList SuppressZero;

extern int NumText;

extern library::one_indexed_container<library::short_string> GamsText;
extern library::one_indexed_container<int> DummyText;
extern library::one_indexed_container<bool_t> ExpandMap;

// Number of subset definitions (0..MaxSubset)
extern int NumSubset;

extern library::one_indexed_container<int> DimSubset;
extern library::one_indexed_container<library::short_string> Subset;
extern library::one_indexed_container<std::string> GamsSubset;
extern library::one_indexed_container<int> NextSubset;
// Next set in chain
extern library::one_indexed_container<int> NextSet;

enum class Subset_t
{
   NoParent,
   ParentIsIndex1,
   ParentIsIndex2
};

extern library::one_indexed_container<Subset_t> ParentOfSubset;

// -1 or 1..NumDimension
extern int Parent;
extern library::one_indexed_container<int> Children;
extern int NumChildren;

struct ParentChildSet_t {
   library::short_string GamsSetName;
   // Dimension numbers
   int Index1 {}, Index2 {}, child {};
};

extern library::one_indexed_container<ParentChildSet_t> ParentDimensionText;
extern int NumParentDimensionTextSets;

constexpr int MaxLiteral { 100 };

extern library::one_indexed_container<library::short_string> LiteralPool;
extern library::one_indexed_container<int> LiteralUel;
extern int NumLiteral;

class SubsetList_t
{
private:
   int dimension {};
   std::vector<int> list;
   bool assigned {};

public:
   bool AddSubset( int i, int k );
   int GetCount();
   int GetSubset( int i );
};

class DimensionStore_t
{
private:
   // TODO: Check types of map
   std::map<int, library::short_string> StringStore;
   library::one_indexed_container<int> NameDimension { MaxDimension };
   library::one_indexed_container<std::vector<int>> DummyDimension { MaxDimension };
   library::one_indexed_container<std::vector<int>> EntryMap { MaxDataEntry };
   library::one_indexed_container<std::vector<int>> TextMap { MaxText };
   library::one_indexed_container<SubsetList_t> SubsetList { MaxSubset };

   // Add new name to the stringstore. Also store its associated
   // dimension number (1..MaxDimension) and the tuple member (1..MaxTuple)
   int AddName( const library::short_string &s, int dim, int tuple );

public:
   // Returns true if name is used already
   // global comparison in hash object (case insensitive)
   bool NameIsUsed( const library::short_string &s );

   // Returns false if name is used already
   // else a new dimension name is added
   bool AddNewTab( const library::short_string &s, int dimension );

   // Returns false if name is used already
   // else a new tuple member name is added to the dimension
   bool AddNewTupleIndex( const library::short_string &s, int dimension );

   // 0 if not found
   int GetDimensionS( const library::short_string &s );

   // Id but for number
   int GetDimensionI( int h );

   // i=1..NumDimension
   library::short_string GetTabName( int i );

   // Size of tuple in dimension i
   // 1 : normal one dimensional thing
   int GetNumTuple( int i );

   // i=1..NumDimension
   // j=1..getNumTuple(i)
   library::short_string GetTuple( int i, int j );

   // i=1..NumDimension
   // j=1..getNumTuple(i)
   int GetTupleI( int i, int j );

   // Get hash for string s
   int GetHash( const library::short_string &s );

   // i = hash
   library::short_string GetName( int i );

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
   library::short_string TextList( int t, int k );

   // Return number of tuple members for dimension i
   int DimensionListLength( int t );

   // Return j-th tuple member for dimension i
   library::short_string DimensionList( int i, int j );

   // Return tuple member index for string stored in position h
   int GetTupleIndex( int h );

   // Add tuplemember to a subset in dimension i
   bool AddSubset( int subset, int i, const library::short_string &s );

   // Get number of tuple members for subset
   int GetSubsetTuples( int subset );

   int GetSubset( int subset, int i );
};

extern DimensionStore_t DimensionStore;

extern std::map<gdxSpecValue, std::string> SpecialValueMapping;
extern std::map<gdxSpecValue, bool> SpecialValueIsString;
extern std::map<gdxSpecValue, bool> SpecialValueIsZero;

enum class Format_t
{
   FormatVeda,
   FormatCSV
};

struct Options_t {
   std::string TupleSeparator;
   bool ShowAllSeparators {}, RelaxDimensionAll {};
   int ValueDim {};
   bool SetsAllowedFlag {};
   library::one_indexed_container<bool_t> SetsAllowed;
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
