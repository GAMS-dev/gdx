#pragma once

#include <string>

namespace gdlib::runner {

    class TRunner {
    public:
        TRunner() = default;
        ~TRunner() = default;

        void ParamsAdd(const std::string &v) {}
        void SetExecutable(const std::string &v) {}
        int StartAndWait() { return 0; }

        int GetProgRC() const { return 0; }
    };

}
