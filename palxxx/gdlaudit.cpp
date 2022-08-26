#include <string>

//#include "../ctv.h"
#include "../global/unit.h"

#include "gdlaudit.h"
#include "paldoorg.h"

#include <memory>

using namespace palxxx::paldoorg;

namespace palxxx::gdlaudit {

    static std::unique_ptr<tpalobject> PalObj{};
    std::string msg;

    static tpalobject& getPalObj() {
        if (!PalObj) PalObj = std::make_unique<tpalobject>();
        return *PalObj;
    }

    void gdlSetAuditLine(const std::string &AuditLine) {
        getPalObj().palSetAuditLine(AuditLine);
    }

    void gdlSetAuditLineLib(const std::string &AuditLine) {
        getPalObj().palSetAuditLine(AuditLine);
    }

    bool gdlAuditRun() {
        return getPalObj().palAuditRun();
    }

    std::string gdlGetAuditLine() {
        return getPalObj().palGetAuditLine();
    }

    std::string gdlGetShortAuditLine() {
        return getPalObj().palGetShortAuditLine();
    }

    void gdlAuditFields(const std::string &auditline,
                        std::string &v1,
                        std::string &v2,
                        std::string &v3) {
        return getPalObj().palAuditFields(auditline, v1, v2, v3);
    }

    void gdlSetSystemName(const std::string &sname) {
        getPalObj().palSetSystemName(sname);
    }

    void initialization() {
    }

    void finalization() {
    }

    UNIT_INIT_FINI();
}