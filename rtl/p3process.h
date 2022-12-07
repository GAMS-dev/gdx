#pragma once
#include <cstdint>
#include <string>
#include "../global/delphitypes.h"

namespace rtl::p3process {
	enum TKillHow { soft, hard };
	int P3SystemP(const std::string& CmdPtr, int& ProgRC);
	int P3ExecP(const std::string& CmdPtr, int& ProgRC);
    using tCtrlHandler = std::function<void(void)>;
}