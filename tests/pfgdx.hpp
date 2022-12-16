#include "../expertapi/gclgms.h"
#include "../gdxinterface.h"

namespace pfgdx {

    class PerfGDX {
        gdxStrIndexPtrs_t Indx;
        gdxStrIndex_t IndxXXX;
        gdxUelIndex_t Uels;
        gdxValues_t Values;
        gdxinterface::GDXInterface *PGX {}, *wPGX {};

    public:
        int pfinit(const char *sysdir, bool useLegacy = false);
        int pffini();
        int pfopenread(const char *fname);
        int pfclose();
        int pfreadraw(const int dowrite);
        int pfregister_uels(const int reverse, const int dowrite);
        int pfreadstr(const int reverse, const int dowrite);
        int pfreadmap();
        int pfwritemap(const int reverse);
    };

    void runWithTiming(const std::string &gdxfn, bool quiet = false);
}