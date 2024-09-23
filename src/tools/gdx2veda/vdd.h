#ifndef GDX_VDD_H
#define GDX_VDD_H

#include <string>
#include <map>
#include <fstream>

#include "container.h"
#include "../library/short_string.h"

namespace gdx2veda
{

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
extern library::one_indexed_container<bool> ExpandMap;

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

// TODO: TDimensionStore DimensionStore;

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
   library::one_indexed_container<bool> SetsAllowed;
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
