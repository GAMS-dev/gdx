#include "doctest.h"
#include "../gxfile.h"
#include <filesystem>
#include <iostream>

using namespace gxfile;
using namespace std::literals::string_literals;

namespace tests::gxfiletests {
    TEST_SUITE_BEGIN("gxfile");

    const std::string fn {"mytest.gdx"};

    TEST_CASE("Test creating a simple gdx file with gxfile port") {
        {
            std::string msg;
            TGXFileObj pgx{msg};
            int ErrNr;
            pgx.gdxOpenWrite(fn, "TGXFileObj", ErrNr);
            pgx.gdxDataWriteStrStart("Demand", "Demand data", 1, 1, 0);
            auto writeRec = [&pgx](const std::string &s, double v) {
                static std::array<std::string, 20> keys{};
                static std::array<double, 5> values{};
                keys.front() = s;
                values.front() = v;
                pgx.gdxDataWriteStr(keys, values);
            };
            writeRec("New-York"s, 324.0);
            writeRec("Chicago"s, 299.0);
            writeRec("Topeka"s, 274.0);
            pgx.gdxDataWriteDone();
            pgx.gdxClose();
        }
        //std::filesystem::remove(fn);
    }

    TEST_CASE("Test reading a simple gdx file with gxfile port") {
        std::string msg;
        TGXFileObj pgx{msg};

        int ErrNr{};
        REQUIRE(pgx.gdxOpenRead(fn, ErrNr));
        REQUIRE_EQ(0, ErrNr);
        //if (ErrNr) ReportIOError(ErrNr, "gdxOpenRead");
        std::string Producer;
        REQUIRE(pgx.gdxFileVersion(msg, Producer));

        std::cout << "GDX file written using version: " << msg << '\n';
        std::cout << "GDX file written by: " << Producer << '\n';

        int SyNr{};
        REQUIRE(pgx.gdxFindSymbol("demand", SyNr));

        int Dim{}, VarTyp{};
        std::string VarName{};
        REQUIRE(pgx.gdxSymbolInfo(SyNr, VarName, Dim, VarTyp));
        REQUIRE(Dim == 1);
        REQUIRE(global::gmsspecs::dt_par == VarTyp);

        int NrRecs;
        REQUIRE(pgx.gdxDataReadStrStart(SyNr, NrRecs));
        //if (!pgx.gdxDataReadStrStart(SyNr,NrRecs)) ReportGDXError(PGX);

        std::cout << "Parameter demand has " << std::to_string(NrRecs) << " records\n";

        gxdefs::TgdxStrIndex Indx;
        gxdefs::TgdxValues Values;
        for(int N{}; pgx.gdxDataReadStr(Indx, Values, N);) {
            /*if (0 == Values[global::gmsspecs::vallevel]) continue;
            for (int D{}; D < Dim; D++)
                std::cout << (D ? '.' : ' ') << Indx[D];
            std::cout << " = %7.2f\n" << Values[global::gmsspecs::vallevel] << '\n';*/
            std::cout << "Key=" << Indx.front() << ", Value=" << Values[global::gmsspecs::vallevel] << "\n";
        }

        REQUIRE(pgx.gdxDataReadDone());
        REQUIRE_FALSE(pgx.gdxClose());
    }

    TEST_SUITE_END();
}