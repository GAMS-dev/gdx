#include "gxdefs.h"

namespace gxdefs {

    bool CanBeQuoted(const std::string &s) {
        bool saw_single{}, saw_double{};
        for(auto Ch : s) {
            if(Ch == '\'') {
                if(saw_double) return false;
                saw_single = true;
            } else if(Ch == '\"') {
                if(saw_single) return false;
                saw_double = true;
            } else if(Ch < ' ') return false;
        }
        return true;
    }

    bool GoodUELString(const std::string &s) {
        return s.length() <= GLOBAL_UEL_IDENT_SIZE-1 && CanBeQuoted(s); // also checks Ch < '
    }

}