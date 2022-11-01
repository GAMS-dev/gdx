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

    class TRunner;

    class TMsgHandler {
        int FVerbose;
        std::string FMsgPfx;

    public:
        friend class TRunner;

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

        bool ErrorWhenRunning(const std::string &s);
        void CommandLineChanged();

    public:
        explicit TRunner();
        ~TRunner() = default;

        void ParamsAdd(const std::string &v);
        void ParamsClear();
        int ParamsCount();
        std::string CommandLine();
        int StartAndWait();

        void SetExecutable(const std::string &v);
        std::string GetExecutable();

        bool IsRunning() const;

        std::string GetWorkDir() const;
        void SetWorkDir(const std::string &v);

        void SetInheritHandles(bool v);
        bool GetInheritHandles() const;

        void SetUseShell(bool v);
        bool GetUseShell() const;

        int GetVerbose();
        void SetVerbose(int v);

        void SetVisible(TVisible v);
        TVisible GetVisible() const;

        int GetProgRC() const;
    };

}
