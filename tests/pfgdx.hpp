#include "../expertapi/gclgms.h"
#include "../gdxinterface.h"

#include <chrono>

namespace pfgdx {

    class PerfGDX {
        gdxStrIndexPtrs_t Indx;
        gdxStrIndex_t IndxXXX;
        gdxUelIndex_t Uels;
        gdxValues_t Values;
        gdxinterface::GDXInterface *PGX {}, *wPGX {};

        int symCountLimit {-1}, recCountLimit {-1};

        void enforceSymCountLimit(int &symCount);

        void enforceRecCountLimit(int &recCount);

    public:
        void setLimits(int nsyms, int nrecs);

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

    struct TimeTriple {
        std::chrono::time_point<std::chrono::high_resolution_clock> item_start{std::chrono::high_resolution_clock::now()};
        double total_t{}, subtotal_t{}, item_t{};
    };

    TimeTriple runWithTiming(const std::string &gdxfn, bool useLegacy = false, bool quiet = false, int symCountLimit=-1, int recCountLimit=-1);
}