#pragma once

#include <string>
#include <vector>

namespace gdlib::runner {

    const int EC_Cannot_modify = 1;
    const int EC_Process_Active = 2;
    const int EC_Empty_CMD_Line = 3;

    enum TVisible {
        vis_hide, vis_minimized, vis_normal
    };

    class TMsgHandler {
        int FVerbose;
        std::string FMsgPfx;

    public:
        TMsgHandler(const std::string &MsgPfx);
        void ErrorMessage(int ec, const std::string &s);
        void LogMessage(const std::string &s);
        void DebugMessage(const std::string &s);
    };



    class TRunner {
        TMsgHandler FMsgHandler;
        std::string FExecutable;
        std::vector<std::string> FParams;
        std::string FWorkDir;
        std::string FCommandLine;
        bool FIsRunning, FInheritHandles, FUseShell;
        TVisible FVisible;
        int FProgRC;

    public:
        explicit TRunner();

        ~TRunner() = default;

        void ParamsAdd(const std::string &v);
        void SetExecutable(const std::string &v);
        int StartAndWait();

        int GetProgRC() const;
    };

}
