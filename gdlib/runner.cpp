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

    void TRunner::SetExecutable(const std::string &v) {}

    int TRunner::StartAndWait() { return 0; }

    int TRunner::GetProgRC() const { return 0; }
    
    bool TRunner::ErrorWhenRunning(const std::string& s)
    {
        return false;
    }
    
    void TRunner::SetWorkDir(const std::string& v)
    {
    }
    
    void TRunner::SetInheritHandles(bool v)
    {
    }
    
    void TRunner::SetUseShell(bool v)
    {
    }
    
    void TRunner::CommandLineChanged()
    {
    }
    
    int TRunner::GetVerbose()
    {
        return 0;
    }
    
    void TRunner::SetVerbose(int v)
    {
    }
    
    void TRunner::SetVisible(TVisible v)
    {
    }
}