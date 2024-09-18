#include <iostream>

#include "vdd.h"

namespace gdx2veda
{

int NumErr {};

void ReportError( const std::string &msg )
{
   NumErr++;
   std::cout << "*** " << msg << std::endl;
}

}// namespace gdx2veda
