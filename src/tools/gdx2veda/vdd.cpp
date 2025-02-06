#include <iostream>

#include "vdd.hpp"

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

library::ShortString_t VEDAFlavor { "MyVEDA" };

// library::Container_t<std::string> NameDimension { MaxDimension };
// library::Container_t<std::string> DummyDimension { MaxDimension };

library::Container_t<std::string> AtrName { MaxDataEntry };
library::Container_t<library::ShortString_t> GamsName { MaxDataEntry };
library::Container_t<int> GamsSuff { MaxDataEntry };
library::Container_t<int> GamsDim { MaxDataEntry };

int NumText {};

library::Container_t<library::ShortString_t> GamsText { MaxText };
library::Container_t<int> DummyText { MaxText };
library::Container_t<bool_t> ExpandMap { MaxText };

int NumSubset {};

library::Container_t<int> DimSubset { MaxDimension };
library::Container_t<library::ShortString_t> Subset { MaxSubset };
library::Container_t<std::string> GamsSubset { MaxSubset };
library::Container_t<int> NextSubset { MaxSubset };
library::Container_t<int> NextSet { MaxSubset };

bool SubsetList_t::AddSubset( const int i, const int k )
{
   if( !assigned )
   {
      // list.clear();
      dimension = i;
      assigned = true;
   }
   else if( i != dimension )
      return false;
   list.Add( k );
   return true;
}

int SubsetList_t::GetCount()
{
   return list.GetCount();
}

int SubsetList_t::GetSubset( const int i )
{
   int *result {};
   list.GetItem( i - 1, &result );
   return *result;
}

int DimensionStore_t::AddName( const library::ShortString_t &s, const int dim, const int tuple )
{
   // TODO: Implement function
   return {};
}

bool DimensionStore_t::NameIsUsed( const library::ShortString_t &s )
{
   // TODO: Implement function
   return {};
}

bool DimensionStore_t::AddNewTab( const library::ShortString_t &s, const int dimension )
{
   // TODO: Implement function
   return {};
}

bool DimensionStore_t::AddNewTupleIndex( const library::ShortString_t &s, const int dimension )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::GetDimensionS( const library::ShortString_t &s )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::GetDimensionI( const int h )
{
   // TODO: Implement function
   return {};
}

library::ShortString_t DimensionStore_t::GetTabName( const int i )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::GetNumTuple( const int i )
{
   // TODO: Implement function
   return {};
}

library::ShortString_t DimensionStore_t::GetTuple( const int i, const int j )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::GetTupleI( const int i, const int j )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::GetHash( const library::ShortString_t &s )
{
   // TODO: Implement function
   return {};
}

library::ShortString_t DimensionStore_t::GetName( const int i )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::EntryList( const int e, const int k )
{
   // TODO: Implement function
   return {};
}

bool DimensionStore_t::AddEntryList( const int e, const int k, const int j )
{
   // TODO: Implement function
   return {};
}

bool DimensionStore_t::AddTextList( const int t, const int k, const int j )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::TextListLength( const int t )
{
   // TODO: Implement function
   return {};
}

library::ShortString_t DimensionStore_t::TextList( const int t, const int k )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::DimensionListLength( const int t )
{
   // TODO: Implement function
   return {};
}

library::ShortString_t DimensionStore_t::DimensionList( const int i, const int j )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::GetTupleIndex( const int h )
{
   // TODO: Implement function
   return {};
}

bool DimensionStore_t::AddSubset( const int subset, const int i, const library::ShortString_t &s )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::GetSubsetTuples( const int subset )
{
   // TODO: Implement function
   return {};
}

int DimensionStore_t::GetSubset( const int subset, const int i )
{
   // TODO: Implement function
   return {};
}

DimensionStore_t DimensionStore {};

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

library::Container_t<Subset_t> ParentOfSubset { MaxSubset };

int Parent { -1 };
library::Container_t<int> Children { MaxDimension };
int NumChildren {};

library::Container_t<ParentChildSet_t> ParentDimensionText { MaxDimension };
int NumParentDimensionTextSets {};

Options_t Options {
        ".",
        false,
        false,
        1,
        false,
        library::Container_t<bool_t> { MaxDimension },
        "",
        Format_t::FormatVeda };

std::ofstream f {};

library::Container_t<library::ShortString_t> LiteralPool { MaxLiteral };
library::Container_t<int> LiteralUel { MaxLiteral };
int NumLiteral {};

gdlib::strhash::TXStrHashList<std::nullptr_t> SuppressZero {};

void VddHelp()
{
   std::cout << "VDD file Summary\n"
             << "--------------------------------------------------------------------\n"
             << "[DataBaseName]\n"
             << " myveda\n"
             << '\n'
             << "[Dimensions] cube dimensions\n"
             << " long_name tuple_element1 tuple_element2 ...\n"
             << '\n'
             << "[DataEntries] data for the cube\n"
             << " long_name gams_name tuple_element1 tuple_element2 ...\n"
             << '\n'
             << "[DimensionText] for generating .vde file (only for data in [DataEntries])\n"
             << " gams_set tuple_element1 tuple_element2 ...\n"
             << '\n'
             << "[DimensionTextAll] for generating .vde file (also for data not in [DataEntries])\n"
             << " gams_set tuple_element1 tuple_element2 ...\n"
             << '\n'
             << "[SubSets] for generating .vds file\n"
             << " sub_name gams_name tuple_element1 tuple_element2 ...\n"
             << '\n'
             << "[ParentDimension] defines parent-child structure\n"
             << " parent_tab  child_tab1 child_tab2 ...\n"
             << '\n'
             << "[ParentDimensionTextAll] .vde file definitions with parent-child structure\n"
             << "2d_gams_set  parent_tab child_tab\n"
             << "2d_gams_set  child_tab parent_tab\n"
             << '\n'
             << "[ParentSubSets] .vds file definitions with parent-child structure\n"
             << "sub_name 2d_gams_set parent_tab child_tab\n"
             << "sub_name 2d_gams_set child_tab parent_tab\n"
             << '\n'
             << "[Options]\n"
             << "TupleSeparator \"string\"  use a different separator symbol between tuple elements\n"
             << "ShowAllSeparators        don''t squeeze unnecessary separators\n"
             << "RelaxDimensionAll        relax strict dimensionality checks in DimensionText(All) sections.\n"
             << "ValueDim n               if n=2 write PV/DV value pairs for VEDA\n"
             << "SetsAllowed dim1 dim2 .. write SetsAllowed specification line to VEDA .vd file\n"
             << "Scenario scenarioSet     specify the scenario set; a record with expl text goes to .vde\n"
             << "Format veda/csv          specify the format of the data files\n"
             << "Not-0 attribute ...      don''t write records with zero values for these attributes\n"
             << '\n'
             << "[SpecialValues]\n"
             << "EPS \"string\"             value to be used for EPS\n"
             << "INF \"string\"             value to be used for +INF\n"
             << "MINF \"string\"            value to be used for -INF\n"
             << "NA \"string\"              value to be used for NA\n"
             << "UNDEF \"string\"           value to be used for UNDEF\n"
             << "--------------------------------------------------------------------\n"
             << '\n'
             << "<myveda> is usually the application name which will be displayed\n"
             << "         on the top of the VEDA splash screen. When a new VEDA database is\n"
             << "         created, a new folder with this name will appear:\n"
             << "         ...veda\\database\\mayveda_date_time.\n"
             << "         Where data and time are the creation time stamp.\n"
             << "<tab_name> corresponds to the tabs of your VEDA screen\n"
             << '\n'
             << "Lines starting with * and empty lines are ignored.\n"
             << "Blanks, commas and tabs are delimiters, blanks before and after\n"
             << "delimiters are ignored. Quotes around data items are optional.\n"
             << "The input data is NOT case sensitive.\n"
             << '\n'
             << '\n'
             << "Example of a Veda Data Definition file:\n"
             << '\n'
             << "* Transport model\n"
             << '\n'
             << "[DataBaseName]\n"
             << "myveda\n"
             << '\n'
             << "[Dimensions]\n"
             << "* tab-name indices\n"
             << "Plants     i\n"
             << "Warehouses j\n"
             << "Links      ii jj\n"
             << '\n'
             << "[DataEntries]\n"
             << "* veda_attribute gams_name tab1 tab2 ... for gams index 1, 2, ...\n"
             << "\"x( i, j ) duals\" x.m      i  Warehouses\n"
             << "Shipments      x.l      i  j\n"
             << "SupplyPrice    supply.m i\n"
             << "DemandPrice    demand.m j\n"
             << "TransportCost  c        i  j\n"
             << "Distance       d        ii jj\n"
             << "Supply         a        i\n"
             << "Demand         b        j\n"
             << "TotalCost      z.l\n"
             << "SupplyNodes    i        i\n"
             << "DemandNodes    j        j\n"
             << "Rate           f\n"
             << '\n'
             << "[DimensionText]\n"
             << "* gams_set tab\n"
             << "i  i\n"
             << '\n'
             << "[DimensionTextAll]\n"
             << "* gams_set tab\n"
             << "j  j\n"
             << '\n'
             << "[SubSets]\n"
             << "* sub_name gams_name tab\n"
             << "i1 ic  Plants  \n"
             << "i1 id  i       \n"
             << '\n'
             << "Notes:\n"
             << "The long name from the [Dimensions] section can be used as a macro that\n"
             << "  expands to the tuples it defines. E.g. \"Links\" is identical to \"ii jj\".\n"
             << "In the [DataEntries] section a literal tuple element can be defined as\n"
             << "  /element/.\n"
             << "When ValueDim=2, the [DataEntries] section can contain X.LM entries,\n"
             << "  indicating both .L and .M needs to written as a pair." << std::endl;
}

void LoadVdd( const std::string &fn )
{
   // TODO: Implement function
}

void DumpVdd( const std::ofstream &f )
{
   // TODO: Implement function
}

void ReportError( const std::string &msg )
{
   NumErr++;
   std::cout << "*** " << msg << std::endl;
}

}// namespace gdx2veda
