#pragma once

#include "gdx.hpp"

namespace gdlib::scrgdx {

constexpr int  TStart {11}, // mipnod
               TEnd{22}; // robj

enum class CreateType
{
   forRead,
   forWrite
};

class TScratchGdx {
   std::unique_ptr<gdx::TGXFileObj> gdx {};
   gdx::TgdxUELIndex Keys {};
   gdx::TgdxValues Vals {};
   int irow {}, lrow {}, jcol {};
   int knz {}, nzcnt {}, jnzmax {};
   int knl {}, nlcnt {}, inlmax {};
   int cpcnt {}, seq {}, Fcpoolsize {};
   bool FhaveNLNZ {}, FrowsDone {}, htrecread {};
   std::array<double, TEnd> htrec {};
   std::string lasterrmsg {};

   void WriteGDXRecord(int idx, double val, double def);
   bool GDXError(const std::string &procname, std::string &msg ) const;

   void CreateForWriting( const std::string &fn, std::string &msg );

public:
   TScratchGdx(CreateType ct, const std::string &s, std::string &msg)
   {
   }

   int WriteColumn(int ivartype, int vnzcnt, double lower, double leve, double upper, int varsosset, double prior, int imarginal, double marginal, double scale)
   {
      STUBWARN();
      return 0;
   }

   int WriteRow(int itype, double rhs, double slack, int imarginal, double marginal, double scale, int match) {
      STUBWARN();
      return 0;
   }

   int WriteJac(int rownum, double jacval, int nlflag)
   {
      STUBWARN();
      return 0;
   }

   int ReadDone()
   {
      // ..
      STUBWARN();
      return 0;
   }

   int WriteDone()
   {
      // ...
      STUBWARN();
      return 0;
   }

   std::string GetLastErrMsg()
   {
      STUBWARN();
      return {};
   }

   int PutHTRec( int inum, double value );

   int GetHTRec( int inum, double &value );

   [[nodiscard]] bool GetRowsDone() const { return FrowsDone; }

   int ReadColumnSol( int &cstat, int &stat, double &level, double &marginal, double &margScale );
   int ReadRowSol(int &cstat, int &stat, double &level, double &marginal, double &margScale );
};

}
