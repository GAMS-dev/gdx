#include "runner.h"
#include "../utils.h"
#include "../rtl/p3process.h"

using namespace std::literals::string_literals;

namespace gdlib::runner {

    TMsgHandler::TMsgHandler(const std::string& MsgPfx) : FMsgPfx{ MsgPfx }, FVerbose{ 1 } {
    }

    void TMsgHandler::ErrorMessage(int ec, const std::string &s) {
        std::cout << "*** Error: "s + s << '\n';
    }

    void TMsgHandler::LogMessage(const std::string &s) {
        if (FVerbose >= 1)
            std::cout << s << std::endl;
    }

    void TMsgHandler::DebugMessage(const std::string &s) {
        if (FVerbose >= 2)
            LogMessage(FMsgPfx + ": "s + s);
    }

    TRunner::TRunner() :
        FMsgHandler{ "Runner" },
        FInheritHandles{},
        FProgRC{},
        FUseShell{},
        FVisible{},
        FIsRunning{},
        FExecutable{},
        FCommandLine{},
        FParams{},
        FWorkDir{}
    {
    }

    TRunner::~TRunner() {}

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

    void TRunner::SetExecutable(const std::string &v) {
        if (ErrorWhenRunning("Executable")) return;
        FExecutable = v;
        CommandLineChanged();
    }

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

    int TRunner::StartAndWait() {
        if (IsRunning()) {
            FMsgHandler.ErrorMessage(EC_Process_Active, "Cannot start an active process"s);
            return EC_Process_Active;
        }
        if (FCommandLine.empty()) {
            FMsgHandler.ErrorMessage(EC_Empty_CMD_Line, "No Command Line specified"s);
            return EC_Empty_CMD_Line;
        }
        int res{};
        if (FUseShell) {
            FMsgHandler.DebugMessage("Use shell: "s + FCommandLine);
            return rtl::p3process::P3SystemP(FCommandLine, FProgRC);
        }
        else {
            FMsgHandler.DebugMessage("Direct call: "s + FCommandLine);
            int res{ rtl::p3process::P3ExecP(FCommandLine, FProgRC) };
            FMsgHandler.DebugMessage("Return = "s + std::to_string(res) + " RC = "s + std::to_string(FProgRC));
            return res;
        }
    }

    int TRunner::GetProgRC() const {
        return 0;
    }
    
    bool TRunner::ErrorWhenRunning(const std::string& s) {
        bool res{ IsRunning() };
        if (res)
            FMsgHandler.ErrorMessage(EC_Cannot_modify, "Cannot modify "s + s + "when process is active"s);
        return res;
    }
    
    std::string TRunner::GetWorkDir() const {
        return FWorkDir;
    }

    void TRunner::SetWorkDir(const std::string& v)
    {
        if (ErrorWhenRunning("WorkDir")) return;
        FWorkDir = v;
    }
    
    void TRunner::SetInheritHandles(bool v)
    {
        if (ErrorWhenRunning("InheritHandles")) return;
        FInheritHandles = v;
    }

    bool TRunner::GetInheritHandles() const {
        return FInheritHandles;
    }
    
    void TRunner::SetUseShell(bool v) {
        if (ErrorWhenRunning("UseShell")) return;
        FUseShell = v;
        CommandLineChanged();
    }

    bool TRunner::GetUseShell() const {
        return FUseShell;
    }
    
    void TRunner::CommandLineChanged() {
        FCommandLine.clear();
    }
    
    int TRunner::GetVerbose() {
        return FMsgHandler.FVerbose;
    }
    
    void TRunner::SetVerbose(int v) {
        FMsgHandler.FVerbose = v;
    }
    
    void TRunner::SetVisible(TVisible v) {
        if (ErrorWhenRunning("Visible")) return;
        FVisible = v;
    }

    TVisible TRunner::GetVisible() const {
        return FVisible;
    }
}