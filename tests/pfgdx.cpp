#include "pfgdx.hpp"
#include "../gxfile.h"
#include "../xpwrap.h"

#include <cassert>
#include <filesystem>

using namespace std::literals::string_literals;

namespace pfgdx {

    int PerfGDX::pfinit(const char *sysdir, bool useLegacy) {
        std::string Msg;
        if(useLegacy) {
            PGX = new xpwrap::GDXFile{Msg};
            if (!Msg.empty()) return 1;
            wPGX = new xpwrap::GDXFile{Msg};
            if (!Msg.empty()) return 1;
        }
        else {
            PGX = new gxfile::TGXFileObj{Msg};
            if (!Msg.empty()) return 1;
            wPGX = new gxfile::TGXFileObj{Msg};
            if (!Msg.empty()) return 1;
        }
        GDXSTRINDEXPTRS_INIT(IndxXXX, Indx);
        return 0;
    }

    int PerfGDX::pffini() {
        delete PGX;
        delete wPGX;
        return 0;
    }

    int PerfGDX::pfopenread(const char *fname) {
        int ErrNr;
        return !PGX->gdxOpenRead(fname, ErrNr) ? 1 : 0;
    }

    int PerfGDX::pfclose() {
        if (PGX->gdxClose()) {
            char S[GMS_SSSIZE];
            printf("**** Fatal GDX Error\n");
            PGX->gdxErrorStr(PGX->gdxGetLastError(), S);
            printf("**** %s\n", S);
            return 1;
        }
        return 0;
    }

    int PerfGDX::pfreadraw(const int dowrite) {
        int ErrNr, i, k, symCount, uelCount, sydim, sytype, nrecs, afdim;
        char syid[GMS_SSSIZE];

        if (!PGX->gdxSystemInfo(symCount, uelCount))
            return 1;
        
        enforceSymCountLimit(symCount);

        if (dowrite) {
            if (!wPGX->gdxOpenWrite("out.gdx", "pfcc test raw", ErrNr))
                return 1;
        }
        for (i = 1; i <= symCount; i++) {
            if (!PGX->gdxSymbolInfo(i, syid, sydim, sytype))
                return 1;
            if (sytype == GMS_DT_ALIAS)
                continue;
            if (!PGX->gdxDataReadRawStart(i, nrecs))
                return 1;
            if (dowrite) {
                if (!wPGX->gdxDataWriteRawStart(syid, "", sydim, sytype, 0))
                    return 1;
            }
            enforceRecCountLimit(nrecs);
            for (k = 0; k < nrecs; k++) {
                if (!PGX->gdxDataReadRaw(Uels, Values, afdim))
                    return 1;
                if (dowrite) {
                    if (!wPGX->gdxDataWriteRaw(Uels, Values))
                        return 1;
                }
            }
            if (dowrite) {
                if (!wPGX->gdxDataWriteDone())
                    return 1;
            }
            if (!PGX->gdxDataReadDone())
                return 1;
        }
        if (dowrite) {
            if (wPGX->gdxClose())
                return 1;
        }
        return 0;
    }

    int PerfGDX::pfregister_uels(const int reverse, const int dowrite) {
        int i, j, maxuellen, symCount, uelCount;
        //char label[256];
        char* uels{};

        maxuellen = PGX->gdxUELMaxLength() + 1;

        if (!PGX->gdxSystemInfo(symCount, uelCount))
            return 1;

        enforceSymCountLimit(symCount);

        std::vector<char> uelsStorage(maxuellen * (uelCount + 1));
        uels = uelsStorage.data();
        if (!uels) return 1;
        for (i = 0; i < uelCount; i++) {
            if (!PGX->gdxUMUelGet(i + 1, uels + i * maxuellen, j))
                return 1;
        }

        auto gdx = dowrite ? wPGX : PGX;

        if (!gdx->gdxUELRegisterMapStart())
            return 1;

        for (i = 0; i < uelCount; i++) {
            if (reverse)
                j = uelCount - i;
            else
                j = i + 1;
            if (!gdx->gdxUELRegisterMap(j, (const char *) (uels + (j - 1) * maxuellen)))
                return 1;
        }
        if (!gdx->gdxUELRegisterDone())
            return 1;

        return 0;
    }

    int PerfGDX::pfreadstr(const int reverse, const int dowrite) {
        int ErrNr, i, k, symCount, uelCount, sydim, sytype, nrecs, afdim;
        char syid[GMS_SSSIZE];

        if (!PGX->gdxSystemInfo(symCount, uelCount))
            return 1;

        enforceSymCountLimit(symCount);

        if (dowrite) {
            if (!wPGX->gdxOpenWrite("out.gdx", "pfcc test str", ErrNr))
                return 1;
            if (pfregister_uels(reverse, 1))
                return 1;
        }
        for (i = 1; i <= symCount; i++) {
            if (!PGX->gdxSymbolInfo(i, syid, sydim, sytype))
                return 1;
            if (sytype == GMS_DT_ALIAS)
                continue;
            if (!PGX->gdxDataReadStrStart(i, nrecs))
                return 1;
            if (dowrite) {
                if (!wPGX->gdxDataWriteStrStart(syid, "", sydim, sytype, 0))
                    return 1;
            }
            enforceRecCountLimit(nrecs);
            for (k = 0; k < nrecs; k++) {
                if (!PGX->gdxDataReadStr(Indx, Values, afdim))
                    return 1;
                if (dowrite) {
                    if (!wPGX->gdxDataWriteStr((const char **) Indx, Values))
                        return 1;
                }
            }
            if (dowrite) {
                if (!wPGX->gdxDataWriteDone())
                    return 1;
            }
            if (!PGX->gdxDataReadDone())
                return 1;
        }
        if (dowrite) {
            if (wPGX->gdxClose())
                return 1;
        }
        return 0;
    }

    int PerfGDX::pfreadmap() {
        int i, k, symCount, uelCount, sydim, sytype, nrecs, afdim;
        char syid[GMS_SSSIZE];

        if (!PGX->gdxSystemInfo(symCount, uelCount))
            return 1;

        enforceSymCountLimit(symCount);

        for (i = 1; i <= symCount; i++) {
            if (!PGX->gdxSymbolInfo(i, syid, sydim, sytype))
                return 1;
            if (sytype == GMS_DT_ALIAS)
                continue;
            if (!PGX->gdxDataReadMapStart(i, nrecs))
                return 1;
            enforceRecCountLimit(nrecs);
            for (k = 0; k < nrecs; k++) {
                if (!PGX->gdxDataReadMap(k, Uels, Values, afdim))
                    return 1;
            }
            if (!PGX->gdxDataReadDone())
                return 1;
        }
        return 0;
    }

    int PerfGDX::pfwritemap(const int reverse) {
        int ErrNr, i, k, symCount, uelCount, sydim, sytype, nrecs, afdim;
        char syid[GMS_SSSIZE];

        if (!PGX->gdxSystemInfo(symCount, uelCount))
            return 1;

        enforceSymCountLimit(symCount);

        if (!wPGX->gdxOpenWrite("out.gdx", "pfcc test map", ErrNr))
            return 1;

        if (pfregister_uels(reverse, 1))
            return 1;

        for (i = 1; i <= symCount; i++) {
            if (!PGX->gdxSymbolInfo(i, syid, sydim, sytype))
                return 1;
            if (sytype == GMS_DT_ALIAS)
                continue;
            if (!PGX->gdxDataReadRawStart(i, nrecs))
                return 1;
            if (!wPGX->gdxDataWriteMapStart(syid, "", sydim, sytype, 0))
                return 1;

            enforceRecCountLimit(nrecs);
            for (k = 0; k < nrecs; k++) {
                if (!PGX->gdxDataReadRaw(Uels, Values, afdim))
                    return 1;
                if (!wPGX->gdxDataWriteMap(Uels, Values))
                    return 1;
            }
            if (!PGX->gdxDataReadDone())
                return 1;
            if (!wPGX->gdxDataWriteDone())
                return 1;
        }
        if (wPGX->gdxClose())
            return 1;
        return 0;
    }

    void PerfGDX::setLimits(int nsyms, int nrecs) {
        symCountLimit = nsyms;
        recCountLimit = nrecs;
    }

    inline void PerfGDX::enforceSymCountLimit(int &symCount) {
        if(symCountLimit != -1) symCount = std::min<int>(symCountLimit, symCount);
    }

    inline void PerfGDX::enforceRecCountLimit(int &recCount) {
        if(recCountLimit != -1) recCount = std::min<int>(recCountLimit, recCount);
    }

    static bool silencePrintT {};
    static int stepCnt {};

    static void printT(TimeTriple &t, const std::string &fn, const std::string &text) {
        if(silencePrintT) return;
        t.item_t = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t.item_start).count();
        t.total_t = t.total_t + t.item_t;
        t.subtotal_t = t.subtotal_t + t.item_t;
        std::cout   << utils::doubleToString(t.total_t, 9, 3) << " # "
                    << utils::doubleToString(t.subtotal_t, 9, 3) << " # "
                    << utils::doubleToString(t.item_t, 9, 3) << " # "
                    << utils::blanks(std::max<int>(0, 10 - static_cast<int>(fn.length()))) << fn << " # "
                    << std::to_string(++stepCnt) << " # "
                    << text << '\n';
        t.item_start = std::chrono::high_resolution_clock::now();
    }

    TimeTriple runWithTiming(const std::string &gdxfn, bool useLegacy, bool quiet, int symCountLimit, int recCountLimit) {
        if(quiet) silencePrintT = true;
        stepCnt = 0;
        TimeTriple t;
        PerfGDX cgdx;
        cgdx.setLimits(symCountLimit, recCountLimit);
        cgdx.pfinit("", useLegacy);
        int rc{cgdx.pfopenread(gdxfn.c_str())};
        printT(t, gdxfn, "capi cgdxopenread"s);
        assert(!rc);
        rc = cgdx.pfreadraw(0);
        printT(t, gdxfn, "capi reading symbols raw");
        assert(!rc);
        rc = cgdx.pfreadstr(0, 0);
        printT(t, gdxfn, "capi reading symbols str");
        assert(!rc);
        rc = cgdx.pfregister_uels(0,0);
        printT(t, gdxfn, "capi register uels reading reverse=false");
        assert(!rc);
        rc = cgdx.pfreadmap();
        printT(t, gdxfn, "capi reading symbols map reverse=false");
        assert(!rc);
        rc = cgdx.pfclose();
        assert(!rc);
        rc = cgdx.pfopenread(gdxfn.c_str());
        printT(t, gdxfn, "capi cgdxopenread");
        assert(!rc);
        rc = cgdx.pfregister_uels(1,0);
        printT(t, gdxfn, "capi register uels reading reverse=true");
        assert(!rc);
        rc = cgdx.pfreadmap();
        printT(t, gdxfn, "capi reading symbols map reverse=true");
        assert(!rc);
        rc = cgdx.pfreadraw(1);
        printT(t, gdxfn, "capi reading and writing symbols raw");
        assert(!rc);
        rc = cgdx.pfwritemap(0);
        printT(t, gdxfn, "capi reading raw and writing map reverse=false");
        assert(!rc);
        rc = cgdx.pfwritemap(1);
        printT(t, gdxfn, "capi reading raw and writing map reverse=true");
        assert(!rc);
        rc = cgdx.pfreadstr(0, 1);
        printT(t, gdxfn, "capi reading and writing symbols str reverse=false");
        assert(!rc);
        rc = cgdx.pfreadstr(1, 1);
        printT(t, gdxfn, "capi reading and writing symbols str reverse=true");
        assert(!rc);
        rc = cgdx.pfclose();
        assert(!rc);
        rc = cgdx.pffini();
        assert(!rc);
        // silence rc not used warning when compiling in release build (asserts removed).
        if(rc) std::cout << "Final result code non-zero!" << std::endl;
        std::filesystem::remove("out.gdx"s);
        return t;
    }
}