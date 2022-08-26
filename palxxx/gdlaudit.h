#pragma once

#include <string>

namespace palxxx::gdlaudit {

    void gdlSetAuditLine(const std::string &AuditLine);
    void gdlSetAuditLineLib(const std::string &AuditLine);
    bool gdlAuditRun();
    std::string gdlGetAuditLine();
    std::string gdlGetShortAuditLine();
    void gdlAuditFields(const std::string &auditline,
                        std::string &v1,
                        std::string &v2,
                        std::string &v3);
    void gdlSetSystemName(const std::string &sname);

}