#include "doctest.h"
#include "../gxfile.h"
#include "../xpwrap.h"
#include <iostream>
#include <filesystem>

using namespace std::literals::string_literals;

namespace tests::xp_example1 {
    TEST_SUITE_BEGIN("gxfile");

    enum class GDXImplType {
        wrapped,
        ported
    };

    void ReportIOError(int N, const std::string &msg);
    void ReportGDXError(gdxinterface::GDXInterface &PGX);
    void WriteData(gdxinterface::GDXInterface &PGX, const std::string &s, double V);
    gdxinterface::GDXInterface *SetupGDXObject(GDXImplType implType);
    void TeardownGDXObject(gdxinterface::GDXInterface **pgx);
    void runXpExample1Main(GDXImplType implType);

    static std::ostream mycout {&gxfile::null_buffer};

    void ReportIOError(int N, const std::string &msg) {
        mycout << "**** Fatal I/O Error = " << N << " when calling " << msg << "\n";
        // exit(1)
    }

    void ReportGDXError(gdxinterface::GDXInterface &PGX) {
        std::array<char, GMS_SSSIZE> S;
        mycout << "**** Fatal GDX Error\n";
        REQUIRE(PGX.gdxErrorStr(PGX.gdxGetLastError(), S.data()));
        mycout << "**** " << S.data() << "\n";
    }

    static gdxinterface::StrIndexBuffers Indx {};
    static gdxinterface::TgdxValues Values{};

    void WriteData(gdxinterface::GDXInterface &PGX,
                   const std::string &s,
                   double V) {
        Indx[0] = s;
        Values[GMS_VAL_LEVEL] = V;
        const char *keyptrs[] = {s.c_str()};
        PGX.gdxDataWriteStr(keyptrs, Values.data());
    }

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
                                            dt_par,
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

        int rc = system("gamslib trnsport > gamslibLog.txt");
        REQUIRE_FALSE(rc);
        // FIXME: Actually load data from GDX file by modifying trnsport src here
        rc = system("gams trnsport gdx=result lo=0 o=lf");// > gamsLog.txt");
        REQUIRE_FALSE(rc);
        std::filesystem::remove("gamslibLog.txt");
        //std::filesystem::remove("gamsLog.txt");

        PGX = SetupGDXObject(implType);
        REQUIRE(PGX->gdxOpenRead("result.gdx", ErrNr));
        if(ErrNr) ReportIOError(ErrNr, "gdxOpenRead");
        REQUIRE_EQ(0, ErrNr);
        std::array<char, GMS_SSSIZE> Msg, Producer;
        REQUIRE(PGX->gdxFileVersion(Msg.data(), Producer.data()));
        mycout <<    "GDX file written using version " << Msg.data() <<
                  "\nGDX file written by " << Producer.data() << "\n";

        int VarNr;
        res = PGX->gdxFindSymbol("x", VarNr);
        REQUIRE(res);
        if(!res) {
            mycout << "Could not find variable X\n";
            //exit(1);
        }

        char VarName[GMS_SSSIZE];
        int Dim, VarTyp;
        REQUIRE(PGX->gdxSymbolInfo(VarNr, VarName, Dim, VarTyp));
        REQUIRE_EQ(2, Dim);
        REQUIRE_EQ(dt_var, VarTyp);
        if(Dim != 2 || VarTyp != dt_var) {
            mycout << "**** X is not a two dimensional variable: " << Dim << ":" << VarTyp << "\n";
            // exit(1);
        }

        int NrRecs;
        res = PGX->gdxDataReadStrStart(VarNr, NrRecs);
        REQUIRE(res);
        if(!res) ReportGDXError(*PGX);

        int N;
        while(PGX->gdxDataReadStr(Indx.ptrs(), Values.data(), N)) {
            if(0.0 == Values[GMS_VAL_LEVEL]) continue;
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