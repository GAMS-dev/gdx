#include <iostream>

#include "vdd.h"

namespace gdx2veda
{

std::string Line;
std::string ExpandedLine;

int NumErr {};
int NumLine {};

// int RunPos {};
int AtrPos {};

int NumDimension {};
int NumDataEntry {};

library::short_string VEDAFlavor { "MyVEDA" };

// library::one_indexed_container<std::string> NameDimension { MaxDimension };
// library::one_indexed_container<std::string> DummyDimension { MaxDimension };

library::one_indexed_container<std::string> AtrName { MaxDataEntry };
library::one_indexed_container<int> GamsName { MaxDataEntry };
library::one_indexed_container<int> GamsDim { MaxDataEntry };

int NumText {};

library::one_indexed_container<std::string> GamsText { MaxText };
library::one_indexed_container<int> DummyText { MaxText };
library::one_indexed_container<bool> ExpandMap { MaxText };

int NumSubSet {};

library::one_indexed_container<int> DimSubset { MaxDimension };
library::one_indexed_container<library::short_string> Subset { MaxSubset };
library::one_indexed_container<std::string> GamsSubset { MaxSubset };
library::one_indexed_container<int> NextSubset { MaxSubset };
library::one_indexed_container<int> NextSet { MaxSubset };

// TODO: DimensionStore = TDimensionStore.Create;

// Defaults:
std::map<gdxSpecValue, std::string> SpecialValueMapping {
        { sv_valund, "8888888888" },
        { sv_valna, "9999999999" },
        { sv_valpin, "+1e30" },
        { sv_valmin, "-1e30" },
        { sv_valeps, "0" },
        { sv_normal, "" } };

std::map<gdxSpecValue, bool> SpecialValueIsString {
        { sv_valund, false },
        { sv_valna, false },
        { sv_valpin, false },
        { sv_valmin, false },
        { sv_valeps, false },
        { sv_normal, false } };

std::map<gdxSpecValue, bool> SpecialValueIsZero {
        { sv_valund, false },
        { sv_valna, false },
        { sv_valpin, false },
        { sv_valmin, false },
        { sv_valeps, true },
        { sv_normal, false } };

library::one_indexed_container<Subset_t> ParentOfSubset { MaxSubset };

int Parent { -1 };
library::one_indexed_container<int> Children { MaxDimension };
int NumChildren {};

library::one_indexed_container<ParentChildSet_t> ParentDimensionText { MaxDimension };
int NumParentDimensionTextSets {};

Options_t Options {
        .TupleSeparator = ".",
        .ShowAllSeparators = false,
        .RelaxDimensionAll = false,
        .ValueDim = 1,
        .SetsAllowedFlag = false,
        .ScenarioSet = "",
        .Format = Format_t::FormatVeda };

std::ofstream f {};

library::one_indexed_container<library::short_string> LiteralPool { MaxLiteral };
library::one_indexed_container<int> LiteralUel { MaxLiteral };
int NumLiteral {};

// TODO: SuppressZero = TXStrHashList.Create;

void ReportError( const std::string &msg )
{
   NumErr++;
   std::cout << "*** " << msg << std::endl;
}

}// namespace gdx2veda
