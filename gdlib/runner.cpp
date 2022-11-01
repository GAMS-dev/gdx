#include "runner.h"
#include "../utils.h"

using namespace std::literals::string_literals;

namespace gdlib::runner {

    TMsgHandler::TMsgHandler(const std::string &MsgPfx) {

    }

    void TMsgHandler::ErrorMessage(int ec, const std::string &s) {

    }

    void TMsgHandler::LogMessage(const std::string &s) {

    }

    void TMsgHandler::DebugMessage(const std::string &s) {

    }

    TRunner::TRunner() : FMsgHandler{"Runner"} {

    }

    void TRunner::ParamsAdd(const std::string &v) {
        if (ErrorWhenRunning("ParamsA")) return;
        FParams.push_back(v);
        CommandLineChanged();
    }

    void TRunner::ParamsClear() {
        if (ErrorWhenRunning("ParamsClear")) return;
        FParams.clear();
        CommandLineChanged();
    }

    void TRunner::SetExecutable(const std::string &v) {}

    std::string TRunner::GetExecutable() {
        return FExecutable;
    }

    bool TRunner::IsRunning() const {
        return FIsRunning;
    }

    int TRunner::ParamsCount() {
        return static_cast<int>(FParams.size());
    }

    std::string TRunner::CommandLine() {
        if (!FCommandLine.empty()) return FCommandLine;
        const char Q{ '\"' };
        FCommandLine = utils::quoteWhitespace(FExecutable, Q);
        for (const auto& param : FParams)
            FCommandLine += ' ' + utils::quoteWhitespace(param, Q);

        return FCommandLine;
    }

    int TRunner::StartAndWait() { return 0; }

    int TRunner::GetProgRC() const { return 0; }
    
    bool TRunner::ErrorWhenRunning(const std::string& s)
    {
        return false;
    }
    
    std::string TRunner::GetWorkDir() const {
        return FWorkDir;
    }

    void TRunner::SetWorkDir(const std::string& v)
    {
        FWorkDir = v;
    }
    
    void TRunner::SetInheritHandles(bool v)
    {
        FInheritHandles = v;
    }

    bool TRunner::GetInheritHandles() const {
        return FInheritHandles;
    }
    
    void TRunner::SetUseShell(bool v) {
        FUseShell = v;
    }

    bool TRunner::GetUseShell() const {
        return FUseShell;
    }
    
    void TRunner::CommandLineChanged()
    {
    }
    
    int TRunner::GetVerbose() {
        return FMsgHandler.FVerbose;
    }
    
    void TRunner::SetVerbose(int v) {
        FMsgHandler.FVerbose = v;
    }
    
    void TRunner::SetVisible(TVisible v) {
        FVisible = v;
    }

    TVisible TRunner::GetVisible() const {
        return FVisible;
    }
}