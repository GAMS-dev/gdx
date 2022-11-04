#include "doctest.h"
#include "../gxfile.h"
#include <filesystem>
#include <iostream>

using namespace gxfile;
using namespace std::literals::string_literals;

namespace tests::gxfiletests {
    TEST_SUITE_BEGIN("gxfile");

    static std::ostream mycout {&gxfile::null_buffer};

    const std::string fn {"mytest.gdx"};

    void writeFile() {
        std::string msg;
        TGXFileObj pgx{ msg };
        int ErrNr;
        pgx.gdxOpenWrite(fn, "TGXFileObj", ErrNr);
        pgx.gdxDataWriteStrStart("Demand", "Demand data", 1, 1, 0);
        auto writeRec = [&pgx](const std::string& s, double v) {
            static std::array<std::string, 20> keys{};
            static std::array<double, 5> values{};
            keys.front() = s;
            values.front() = v;
            const char *keyptrs[] = { s.c_str() };
            pgx.gdxDataWriteStr(keyptrs, values.data());
        };
        writeRec("New-York"s, 324.0);
        writeRec("Chicago"s, 299.0);
        writeRec("Topeka"s, 274.0);
        pgx.gdxDataWriteDone();
        pgx.gdxClose();
    }

    void readFile() {
        std::string msg;
        TGXFileObj pgx{ msg };

        int ErrNr{};
        REQUIRE(pgx.gdxOpenRead(fn, ErrNr));
        REQUIRE_EQ(0, ErrNr);
        //if (ErrNr) ReportIOError(ErrNr, "gdxOpenRead");
        std::string Producer;
        REQUIRE(pgx.gdxFileVersion(msg, Producer));

        mycout << "GDX file written using version: " << msg << '\n';
        mycout << "GDX file written by: " << Producer << '\n';

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

        mycout << "Parameter demand has " << std::to_string(NrRecs) << " records\n";

        std::array<std::array<char, 256>, global::gmsspecs::MaxDim> bufs {};
        std::array<char *, global::gmsspecs::MaxDim> Indx {};

        gdxinterface::StrIndexBuffers sibufs {};

        for(int i=0; i<Indx.size(); i++) {
            Indx[i] = bufs[i].data();
        }
        gxdefs::TgdxValues Values;
        for (int N{}; pgx.gdxDataReadStr(Indx.data(), Values.data(), N);) {
            /*if (0 == Values[global::gmsspecs::vallevel]) continue;
            for (int D{}; D < Dim; D++)
                mycout << (D ? '.' : ' ') << Indx[D];
            mycout << " = %7.2f\n" << Values[global::gmsspecs::vallevel] << '\n';*/
            mycout << "Key=" << Indx.front() << ", Value=" << Values[global::gmsspecs::vallevel] << "\n";
        }

        REQUIRE(pgx.gdxDataReadDone());
        REQUIRE_FALSE(pgx.gdxClose());
    }

    TEST_CASE("Test creating and reading a simple gdx file with gxfile port") {
        writeFile();
        readFile();
        std::filesystem::remove(fn);
    }

    TEST_CASE("Test integer mapping") {
        TIntegerMapping mapping;
        REQUIRE_FALSE(mapping.GetHighestIndex());
        mapping[3] = 5;
        REQUIRE_EQ(5, mapping[3]);
        const int memused = mapping.MemoryUsed();
        REQUIRE(memused > 0);
        mapping.clear();
        REQUIRE(mapping.MemoryUsed() < memused);
        mapping.SetMapping(3, 5);
        REQUIRE_EQ(5, mapping[3]);
    }

    TEST_CASE("Test function for making a good explanatory text") {
        REQUIRE_EQ("x", MakeGoodExplText("x"));
        REQUIRE_EQ("Das hier ist ein Test?", MakeGoodExplText("Das hier ist ein Test\r"));
        REQUIRE_EQ("Ein \"gemischter\" \"Text\"!", MakeGoodExplText("Ein \"gemischter\" 'Text'!"));
    }

    TEST_CASE("Test checking whether an identifier is well-formed (good)") {
        REQUIRE_FALSE(IsGoodIdent(""));
        REQUIRE_FALSE(IsGoodIdent(std::string(100, ' ')));
        REQUIRE_FALSE(IsGoodIdent(" abc"));
        REQUIRE_FALSE(IsGoodIdent("A!"));

        REQUIRE(IsGoodIdent("x"));
        REQUIRE(IsGoodIdent("abc_123"));
        REQUIRE(IsGoodIdent("a1"));
    }

    TEST_CASE("Test check mode (indirectly)") {
        std::string errMsg;
        TGXFileObj gdx{errMsg};
        int errNr;
        const std::string tmpfn {"tmp.gdx"};
        REQUIRE(gdx.gdxOpenWrite(tmpfn, "gxfiletest", errNr));
        REQUIRE_FALSE(gdx.gdxDataWriteRaw(nullptr, nullptr));
        gdx.gdxClose();
        std::filesystem::remove(tmpfn);
    }

    TEST_SUITE_END();
}