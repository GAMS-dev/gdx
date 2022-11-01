#include "runner.h"

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

    void TRunner::ParamsAdd(const std::string &v) {}

    void TRunner::ParamsClear() {}

    void TRunner::SetExecutable(const std::string &v) {}

    std::string TRunner::GetExecutable() {
        return FExecutable;
    }

    bool TRunner::IsRunning() const {
        return FIsRunning;
    }

    int TRunner::ParamsCount() {
        return FParams.size();
    }

    std::string TRunner::CommandLine() {
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