#ifndef GDX_CMDPAR_H
#define GDX_CMDPAR_H

#include <string>
#include <vector>
#include <map>

#include "../library/short_string.h"

// Description:
//   Object that handles command line parameters

namespace library::cmdpar
{

// Parameter record
struct TParamRec {
   int Key {};      // key number
   std::string KeyS;// key value
};
using PParamRec = TParamRec *;

// Class to handle command line parameters
class TCmdParams
{
   std::vector<std::pair<std::string, int>> FKeyList;
   std::map<int, TParamRec> FParList;

   void ClearParams();
   void AddVS( int v, const std::string &s );
   int FindKeyV( int V );

public:
   TCmdParams();
   ~TCmdParams();

   bool CrackCommandLine( int ParamCount, const char *ParamStr[] );
   bool AddParameters( int AInsP, const std::string &CmdLine, int ParamCount, const char *ParamStr[] );
   void AddKeyWord( int v, const std::string &s );
   void AddParam( int v, const std::string &s );
   bool HasParam( int v, library::short_string &s );
   bool HasKey( int v );

   TParamRec GetParams( int n );
   [[nodiscard]] int GetParamCount() const;
   [[nodiscard]] std::string GetParamText( int key ) const;
};

enum class CmdParamStatus
{
   kp_input,
   ke_empty = -1,
   ke_unknown = -2,
   ke_noparam = -3,
   ke_badfile = -4,
   kk_big = 10000
};

}// namespace library::cmdpar

#endif//GDX_CMDPAR_H
