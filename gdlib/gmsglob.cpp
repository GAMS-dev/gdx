//
// Created by andre on 8/21/21.
//

#include "gmsglob.h"
//#include "../ctv.h"
#include "../global/unit.h"

using namespace global::gmsspecs;

namespace gdlib::gmsglob {
    std::array<tvarreca, stypsemiint+1> defrecvar;
    std::array<tvarreca, ssyequb+1> defrecequ;

    void InitDefaultRecords()
    {
        for (int v = tvarstyp::stypunknwn; v <= tvarstyp::stypsemiint; v++) {
            for (int f = vallevel; f <= valupper; f++)
                defrecvar[v][f] = 0.0;
            defrecvar[v][valscale] = 1.0;
        }

        defrecvar[stypbin][valupper]    = 1.0;
        defrecvar[stypint][valupper]    = valiup;
        defrecvar[styppos][valupper]    = valpin;
        defrecvar[stypneg][vallower]    = valmin;
        defrecvar[stypfre][vallower]    = valmin;
        defrecvar[stypfre][valupper]    = valpin;
        defrecvar[stypsos1][valupper]    = valpin;
        defrecvar[stypsos2][valupper]    = valpin;
        defrecvar[stypsemi][vallower]    = 1.0;
        defrecvar[stypsemi][valupper]    = valpin;
        defrecvar[stypsemiint][vallower] = 1.0;
        defrecvar[stypsemiint][valupper] = valiup;

        /*equations*/
        for (int e = ssyeque; e <= ssyequb; e++) {
            for (int f = vallevel; f <= valupper; f++)
                defrecequ[e][f] = 0.0;
            defrecequ[e][valscale] = 1.0;
        }

        defrecequ[ssyequg][valupper] = valpin;
        defrecequ[ssyequl][vallower] = valmin;
        defrecequ[ssyequn][vallower] = valmin;
        defrecequ[ssyequn][valupper] = valpin;
        defrecequ[ssyequc][valupper] = valpin;
    }

    void initialization() {
        for (int e = ssyeque; e <= ssyequb; e++)
            equstypInfo[e - ssyeque] = e;
        InitDefaultRecords();
    }

    void finalization() {

    }

    UNIT_INIT_FINI();
}