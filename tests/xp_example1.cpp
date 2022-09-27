#include "doctest.h"
#include "../gxfile.h"
#include "../xpwrap.h"
#include <iostream>
#include <filesystem>

using namespace std::literals::string_literals;

namespace tests::xp_example1 {
    TEST_SUITE_BEGIN("gxfile");

    static std::ostream mycout {&gxfile::null_buffer};

    void ReportIOError(int N, const std::string &msg) {
        mycout << "**** Fatal I/O Error = " << N << " when calling " << msg << "\n";
        // exit(1)
    }

    void ReportGDXError(gdxinterface::GDXInterface &PGX) {
        std::string S;
        mycout << "**** Fatal GDX Error\n";
        REQUIRE(PGX.gdxErrorStr(PGX.gdxGetLastError(), S));
        mycout << "**** " << S << "\n";
        // exit(1);
    }

    static gdxinterface::StrIndexBuffers Indx {};
    static gxdefs::TgdxValues Values{};

    void WriteData(gdxinterface::GDXInterface &PGX,
                   const std::string &s,
                   double V) {
        Indx[0] = s;
        Values[global::gmsspecs::vallevel] = V;
        const char *keyptrs[] = {s.c_str()};
        PGX.gdxDataWriteStr(keyptrs, Values.data());
    }

    enum class GDXImplType {
        wrapped,
        ported
    };

    gdxinterface::GDXInterface *SetupGDXObject(GDXImplType implType) {
        std::string Msg;
        auto *PGX{ implType == GDXImplType::ported ?
            static_cast<gdxinterface::GDXInterface *>(new gxfile::TGXFileObj{Msg}) :
            static_cast<gdxinterface::GDXInterface *>(new xpwrap::GDXFile {Msg}) };
        if(!Msg.empty()) {
            mycout   << "**** Could not load GDX library\n"
                        << "**** " << Msg << "\n";
            // exit(1)
        }
        REQUIRE(Msg.empty());
        return PGX;
    }

    void TeardownGDXObject(gdxinterface::GDXInterface **pgx) {
        (*pgx)->gdxClose();
        delete *pgx;
        *pgx = nullptr;
    }

    void runXpExample1Main(GDXImplType implType) {
        auto PGX = SetupGDXObject(implType);

        // Write demand data
        int ErrNr;
        REQUIRE(PGX->gdxOpenWrite("demanddata.gdx"s, "xp_example1", ErrNr));
        if(ErrNr)
            ReportIOError(ErrNr, "gdxOpenWrite"s);
        REQUIRE_EQ(0, ErrNr);

        bool res {PGX->gdxDataWriteStrStart("Demand",
                                            "Demand data",
                                            1,
                                            global::gmsspecs::dt_par,
                                            0) > 0};
        if(!res)
            ReportGDXError(*PGX);
        REQUIRE(res);

        WriteData(*PGX, "New-York", 324.0);
        WriteData(*PGX, "Chicago", 299.0);
        WriteData(*PGX, "Topeka", 274.0);
        res = PGX->gdxDataWriteDone();
        if(!res)
            ReportGDXError(*PGX);
        REQUIRE(res);
        mycout << "Demand data written by xp_example1\n";
        TeardownGDXObject(&PGX);

        system("gamslib trnsport > gamslibLog.txt");
        // FIXME: Actually load data from GDX file by modifying trnsport src here
        system("gams trnsport gdx=result lo=0 o=lf");// > gamsLog.txt");
        std::filesystem::remove("gamslibLog.txt");
        //std::filesystem::remove("gamsLog.txt");

        PGX = SetupGDXObject(implType);
        REQUIRE(PGX->gdxOpenRead("result.gdx", ErrNr));
        if(ErrNr) ReportIOError(ErrNr, "gdxOpenRead");
        REQUIRE_EQ(0, ErrNr);
        std::string Msg, Producer;
        REQUIRE(PGX->gdxFileVersion(Msg, Producer));
        mycout <<    "GDX file written using version " << Msg <<
                  "\nGDX file written by " << Producer << "\n";

        int VarNr;
        res = PGX->gdxFindSymbol("x", VarNr);
        REQUIRE(res);
        if(!res) {
            mycout << "Could not find variable X\n";
            //exit(1);
        }

        std::string VarName;
        int Dim, VarTyp;
        REQUIRE(PGX->gdxSymbolInfo(VarNr, VarName, Dim, VarTyp));
        REQUIRE_EQ(2, Dim);
        REQUIRE_EQ(global::gmsspecs::dt_var, VarTyp);
        if(Dim != 2 || VarTyp != global::gmsspecs::dt_var) {
            mycout << "**** X is not a two dimensional variable: " << Dim << ":" << VarTyp << "\n";
            // exit(1);
        }

        int NrRecs;
        res = PGX->gdxDataReadStrStart(VarNr, NrRecs);
        REQUIRE(res);
        if(!res) ReportGDXError(*PGX);

        int N;
        while(PGX->gdxDataReadStr(Indx.ptrs(), Values.data(), N)) {
            if(0.0 == Values[global::gmsspecs::vallevel]) continue;
            for(int D{}; D<Dim; D++)
                mycout << (D ? '.':' ') << Indx[D].str();
            //printf(" = %7.2f\n", Values[global::gmsspecs::vallevel]);
        }
        mycout << "All solution values shown\n";
        PGX->gdxDataReadDone();

        TeardownGDXObject(&PGX);

        std::array filesToDelete{
            "demanddata.gdx"s, "result.gdx"s, "trnsport.gms"s, "lf"s
        };
        for (const auto& fn : filesToDelete)
            std::filesystem::remove(fn);
    }

    TEST_CASE("xp_example1_main_wrapped") {
        runXpExample1Main(GDXImplType::wrapped);
    }

    TEST_CASE("xp_example1_main_ported") {
        runXpExample1Main(GDXImplType::ported);
    }

    TEST_SUITE_END();
}