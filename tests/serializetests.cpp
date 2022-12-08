#include <filesystem>
#include "doctest.h"
#include "../gxfile.h"

using namespace std::literals::string_literals;
using namespace gxfile;
using namespace gdxinterface;

namespace tests::serializetests {
    TEST_SUITE_BEGIN("Test serialization of GDX to write operations and YAML");

    enum class TestType {
        TEXT,
        TYAML
    };

    static const auto
        gdx_fn {"serialize_test.gdx"s},
        yaml_fn {"serialize_test.yaml"s},
        text_fn {"serialize_test.txt"s};

    void commonFileCreation(TestType t) {
        std::string msg;
        TGXFileObj obj {msg};
        obj.SetWriteModes(t == TestType::TYAML, t == TestType::TEXT);
        int errNr;
        obj.gdxOpenWrite(gdx_fn, "serializetests", errNr);
        obj.gdxDataWriteStrStart("i", "expl", 1, dt_set, 0);
        TgdxStrIndex keyStrs {};
        TgdxValues vals {};
        for(int i{}; i<5; i++) {
            keyStrs.front() = "i"s+std::to_string(i+1);
            const char *keyptrs[] = {keyStrs.front().c_str()};
            obj.gdxDataWriteStr(keyptrs, vals.data());
        }
        obj.gdxDataWriteDone();
        obj.gdxClose();
    }

    TEST_CASE("Write write ops as text") {
        commonFileCreation(TestType::TEXT);
        REQUIRE(std::filesystem::is_regular_file(text_fn));
        std::filesystem::remove(gdx_fn);
        std::filesystem::remove(text_fn);
    }

#ifdef YAML
    TEST_CASE("YAML representation of GDX file data") {
        commonFileCreation(TestType::TYAML);
        REQUIRE(std::filesystem::is_regular_file(yaml_fn));
        std::filesystem::remove(gdx_fn);
        std::filesystem::remove(yaml_fn);
    }
#endif

    TEST_SUITE_END();
}