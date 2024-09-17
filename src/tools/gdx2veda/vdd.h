#ifndef GDX_VDD_H
#define GDX_VDD_H

#include <string>

#include "container.h"
#include "../library/short_string.h"

namespace gdx2veda
{

// Cube data definitions

// Errors during loading
int NumErr {};
// Veda Data base name
library::short_string VEDAFlavor;

// Max gams dimension
constexpr int MaxGamsDim { 10 };
// Max gams suffixes
constexpr int MaxSuff { 5 };

// .LM is new: write both level and marginal
library::one_indexed_container<std::string>
        NameSuff { { "l", "m", "lo", "up", "lm" } };

// Max cube dimensions
constexpr int MaxDimension { 50 };
// Number of dimensions
int NumDimension {};

// Position of the runname
// int RunPos {};
// Position of the attributes
int AtrPos {};

// const std::string RunDimension { "Scenario" };
const std::string AtrDimension { "Attribute" };

// EK: The following arrays are now inside the TDimensionStore class
// library::one_indexed_container<std::string> NameDimension { MaxDimension };
// library::one_indexed_container<std::string> DummyDimension { MaxDimension };

// Max number of tuples
constexpr int MaxTuple { 20 };
// Max unique attributes
constexpr int MaxDataEntry { 500 };
// Max sets to extract text
constexpr int MaxText { 200 };
// Max subsets to extract text
constexpr int MaxSubset { 200 };

void ReportError( const std::string &msg );

}// namespace gdx2veda

#endif// GDX_VDD_H
