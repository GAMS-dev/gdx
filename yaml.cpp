#include "yaml.h"

using namespace std::literals::string_literals;

namespace yaml {

    const int numSpacesPerIndent = 2;

    TYAMLFile::TYAMLFile(const std::string &filename, bool _active) : active{_active}, curDepth{0} {
        if(active) {
            fp.open(filename);
            if(!fp.is_open()) {
                throw std::runtime_error("Unable to open " + filename + " for writing YAML version of workfile!");
            }
            fp << "---" << '\n';
            if(alwaysFlush) fp.flush();
        }
    }

    TYAMLFile::~TYAMLFile() {
        if(!active) return;
        if(fp.is_open()) fp.close();
    }

    void TYAMLFile::IncIndentLevel() {
        if(!active) return;
        curDepth++;
    }

    void TYAMLFile::DecIndentLevel() {
        if(!active) return;
        curDepth--;
        curDepth = std::max(curDepth, 0);
    }

    void TYAMLFile::AddItem(const std::string &s) {
        if(!active) return;
        WrIndent();
        fp << "- " << (s.empty() ? "\"\"" : s) << std::endl; // actually endl should flush
        if(alwaysFlush) fp.flush();
    }

    void TYAMLFile::AddKeyItem(const std::string &s) {
        if(!active) return;
        WrIndent();
        fp << "- " << s << ":" << std::endl; // actually endl should flush
        if(alwaysFlush) fp.flush();
    }

    void TYAMLFile::AddKey(const std::string &s) {
        if(!active) return;
        WrIndent();
        fp << s << ":" << std::endl; // actually endl should flush
        if(alwaysFlush) fp.flush();

    }

    void TYAMLFile::AddKeyValue(const std::string &key, const std::string &value) {
        if(!active) return;
        WrIndent();
        fp << key << ": " << value << '\n';
        if(alwaysFlush) fp.flush();
    }

    void TYAMLFile::AddKeyValue(const std::string &key, int value) {
        AddKeyValue(key, std::to_string(value));
    }

    void TYAMLFile::AddKeyValue(const std::string &key, bool value) {
        AddKeyValue(key, value ? "true"s : "false"s);
    }

    void TYAMLFile::WrIndent() {
        for(int i=1; i<=numSpacesPerIndent*curDepth; i++)
            fp << " ";
    }

}