#pragma once

#include <string>
#include <fstream>

namespace yaml {

    class TYAMLFile {
        const bool alwaysFlush {true};

        std::ofstream fp;
        int curDepth{};
        bool active;

        void WrIndent();

    public:
        explicit TYAMLFile(const std::string &filename, bool active = true);
        virtual ~TYAMLFile();

        void IncIndentLevel();
        void DecIndentLevel();

        void AddItem(const std::string &s);
        void AddKeyItem(const std::string &s);

        void AddKey(const std::string &s);
        void AddKeyValue(const std::string &key, const std::string &value);
        void AddKeyValue(const std::string &key, int value);
        void AddKeyValue(const std::string &key, bool value);
    };

}