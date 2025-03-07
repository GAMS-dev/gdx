#include "scrgdx.hpp"
#include "global/gmsspecs.hpp"
#include "rtl/sysutils_p3.hpp"

using namespace global::gmsspecs;
using namespace rtl::sysutils_p3;
using namespace std::literals::string_literals;

namespace gdlib::scrgdx
{

enum ScrGdxFlags : uint16_t
{
   SCRGDX_OPEN_FOR_WRITE    =   5,
   SCRGDX_WRITING_ROW       =  10,
   SCRGDX_WRITING_ROWSOL    =  12,
   SCRGDX_WRITING_COLUMN    =  20,
   SCRGDX_WRITING_COLUMNSOL =  22,
   SCRGDX_WRITING_JAC       =  25,
   SCRGDX_WRITING_NLINSTR   =  30,
   SCRGDX_WRITING_CONSTPOOL =  35,
   SCRGDX_WRITING_DICT      =  40,
   SCRGDX_WRITING_DONE      =  45,
   SCRGDX_OPEN_FOR_READ     = 105,
   SCRGDX_READING_ROW       = 110,
   SCRGDX_READING_ROWSOL    = 112,
   SCRGDX_READING_COLUMN    = 120,
   SCRGDX_READING_COLUMNSOL = 122,
   SCRGDX_READING_JAC       = 125,
   SCRGDX_READING_NLINSTR   = 130,
   SCRGDX_READING_CONSTPOOL = 135,
   SCRGDX_READING_DICT      = 140,
   SCRGDX_READING_DONE      = 145,
   SCRGDX_ERROR             = 999
};

constexpr int JACOFFSET {32};

double EpsValue( const double val )
{
   return val == 0.0 ? valeps : val;
}

void TScratchGdx::WriteGDXRecord( int idx, double val, double def )
{
   if(val != def)
   {
      Keys[2] = idx;
      Vals[vallevel] = val;
      gdx->gdxDataWriteRaw( Keys.data(), Vals.data() );
   }
}

bool TScratchGdx::GDXError( const std::string &procname, std::string &msg ) const
{
   msg.clear();
   if( gdx->gdxErrorCount() )
   {
      if( const int ErrNr { gdx->gdxGetLastError() } )
      {
         std::array<char, GMS_SSSIZE> msgBuf {};
         gdx::TGXFileObj::gdxErrorStr( ErrNr, msgBuf.data() );
         msg.assign( msgBuf.data() );
      }
      else
         msg = "GdxErrorCount <> 0: No message available"s;
      msg = procname + ": "s + msg;
      return true;
   }
   return false;
}

void TScratchGdx::CreateForWriting( const std::string &fn, std::string &msg )
{
   bool brc{};
   gdx = std::make_unique<gdx::TGXFileObj>(msg);
   // ...
   STUBWARN();
}

int TScratchGdx::PutHTRec( const int inum, double value )
{
   if (inum < 1 || inum > TEnd)
   {
      lasterrmsg = "HT Record number "s + IntToStr( inum ) + " out of 1.."s + IntToStr( TEnd );
      return 1;
   }
   htrec[inum] = value;
   return 0;
}

int TScratchGdx::GetHTRec( int inum, double &value )
{
   STUBWARN();
   return 0;
}

int TScratchGdx::ReadColumnSol( int &cstat, int &stat, double &level, double &marginal, double &margScale )
{
   constexpr auto procname { "ReadColumnSol" };
   int afdim;
   if(!lasterrmsg.empty()) return 1;
   if(seq == SCRGDX_OPEN_FOR_READ || seq == SCRGDX_READING_ROWSOL)
   {
      int symid, nrecs;
      if(seq == SCRGDX_READING_ROWSOL)
         gdx->gdxDataReadDone();
      gdx->gdxFindSymbol( "variables", symid );
      gdx->gdxDataReadRawStart( symid, nrecs );
      gdx->gdxDataReadRaw( Keys.data(), Vals.data(), afdim );
      if(!nrecs || Keys[1] != 1)
      {
         lasterrmsg = "Column Solution data does not start with row 1 but "s + IntToStr( Keys[1] );
         return 1;
      }
      irow = 0;
      seq = SCRGDX_READING_COLUMNSOL;
   }
   if(seq != SCRGDX_READING_COLUMNSOL)
   {
      lasterrmsg = procname + ": Calling out of context"s;
      return 1;
   }

   jcol++;
   cstat = utils::round<int>(Vals[vallevel]);
   stat = 0;
   level = 0.0;
   marginal = 0.0;
   margScale = 1.0;
   while(gdx->gdxDataReadRaw( Keys.data(), Vals.data(), afdim ))
   {
      if(afdim == 1)
         break;
      switch(Keys[2])
      {
         case 2:
            stat = utils::round<int>(Vals[vallevel]);
            break;
         case 3:
            level = Vals[vallevel];
            break;
         case 4:
            marginal = Vals[vallevel];
            break;
         case 5:
            margScale = Vals[vallevel];
            break;
         default:
            lasterrmsg = procname + "Unknown GDX column solution record "s + IntToStr( Keys[2] );
            break;
      }
   }
   return GDXError( procname, lasterrmsg ) ? 1 : 0;
}

int TScratchGdx::ReadRowSol( int &cstat, int &stat, double &level, double &marginal, double &margScale )
{
   constexpr auto procname { "ReadRowSol" };
   if(!lasterrmsg.empty()) return 1;
   int afdim;
   if(seq == SCRGDX_OPEN_FOR_READ)
   {
      int SymID;
      gdx->gdxFindSymbol( "equations", SymID );
      int nrecs;
      gdx->gdxDataReadRawStart( SymID, nrecs );
      gdx->gdxDataReadRaw( Keys.data(), Vals.data(), afdim );
      if(!nrecs || Keys[1] != 1)
      {
         lasterrmsg = "Row Solution data does not start with row 1 but "s + IntToStr( Keys[1] );
         return 1;
      }
      irow = 0;
      seq = SCRGDX_READING_ROWSOL;
   }
   if(seq != SCRGDX_READING_ROWSOL)
   {
      lasterrmsg = procname + ": Calling out of context"s;
      return 1;
   }

   irow++;
   cstat = utils::round<int>(Vals[vallevel]);
   stat = 0;
   level = 0.0;
   marginal = 0.0;
   margScale = 1.0;
   FrowsDone = true;
   while(gdx->gdxDataReadRaw( Keys.data(), Vals.data(), afdim ))
   {
      if(afdim == 1)
      {
         FrowsDone = false;
         break;
      }
      switch(Keys[2])
      {
         case 2:
            stat = utils::round<int>(Vals[vallevel]);
            break;
         case 3:
            level = Vals[vallevel];
            break;
         case 4:
            marginal = Vals[vallevel];
            break;
         case 5:
            margScale = Vals[vallevel];
            break;
         default:
            lasterrmsg = "Unknown GDX row solution record "s + IntToStr( Keys[2] );
            return 1;
      }
   }

   return GDXError( procname, lasterrmsg ) ? 1 : 0;
}

}// namespace scrgdx