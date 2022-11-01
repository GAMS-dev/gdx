#pragma once
#include <cstdint>
#include <string>


#include "../global/delphitypes.h"

namespace rtl::p3process {

	enum TKillHow { soft, hard };

	struct TProcInfo {
		uint32_t pid; // process ID
		uint32_t tid; // thread ID
		void *hProcess;
	};

	bool p3GetCPUInfo(int& nSockets, int& nCores, int& nThreads, int& coresPerSocket, int& threadsPerCore);
	int p3GetNumberOfProcessors();

	int P3SystemP(const std::string& CmdPtr, int& ProgRC);
	int P3ExecP(const std::string& CmdPtr, int& ProgRC);

	int p3ASyncSystemP(const std::string& cmdPtr, bool newConsole, TProcInfo& procInfo, std::string& msg);
	int p3ASyncExecP(const std::string& cmdPtr, bool newConsole, TProcInfo& procInfo, std::string& msg);

	int p3ASyncStatus(TProcInfo& procInfo, int& progRC, std::string& msg);

	bool p3KillProcGroupTP(const TProcInfo& procInfo, TKillHow how);
	bool p3IsPIDValid(global::delphitypes::Cardinal pid);

    using tCtrlHandler = std::function<void(void)>;

    enum CtrlHandlerState {
        P3CtrlHandlerOK,
        P3CtrlHandlerWasEmpty,
        P3CtrlHandlerSysFail
    };

    CtrlHandlerState P3InstallCtrlHandler(const tCtrlHandler &newHandler);
    int P3UninstallCtrlHandler();
    tCtrlHandler P3GetCtrlHandler();

}