// This is a thin wrapper around the xp-level API for palxxx (palmcc.h)
// It is required since palmcc.h does not provide the full interface of the PAL object
// Should make porting of the PAL object to C++ easier afterwards since it is directly used in CMEX in a OOP fashion

#pragma once

#include <string>
#include <array>

// ==============================================================================================================
// Interface
// ==============================================================================================================

struct palRec;
typedef struct palRec *palHandle_t;

namespace palxxx::paldoorg {

    enum optMsgType {
        optMsgInputEcho,
        optMsgHelp,
        optMsgDefineError,
        optMsgValueError,
        optMsgValueWarning,
        optMsgDeprecated,
        optMsgFileEnter,
        optMsgFileLeave,
        optMsgTooManyMsgs,
        optMsgUserError,
    };

    const int
        maxlicense       =     65 , // size of license strings
        LicenseMaxSub    =     28 ; // including the gams code

    const std::string CmexLicCodes = "00010203040506";

    const int
        graceeval        =     30 , // grace period for evaluation
        gracemaint       =     30 , // grace period for maintenance
        gracebeta        =     30 ; // days for free maintenance

    using tlicstring = std::string;

    std::string GamsLicensesText(int n);

    class tpalobject {

        std::array<char, 256> msgBuf{};
        palHandle_t obj{};

        tlicstring license1, license2, license3, license4, license5, license6;
        const std::array<tlicstring *, 6> licenses = { &license1, &license2, &license3, &license4, &license5, &license6 };
        int LicenseLevel{}, LicenseVersion{}, LicenseStatus{ 1 }, CurrentSearchHelper{}, LicenseActSub{};
        std::string gdlAuditLine;

        bool licensecheckV1to3(int v1, int v2, int v3);

        void updateFields();

    public:
        tpalobject();
        virtual ~tpalobject();

        std::string palGetRelDat() const;
        std::string palGetLicDat() const;

        std::string palGetRel() const;

        std::string palGetGold() const;

        int &palLicenseLevel = LicenseLevel;

        std::string palLicenseDisplay(int i) const;

        int palLicenseGetKey() const;
        int palLicenseGetLevel() const;
        bool palSecurityCheck() const;

        std::string palLicenseGetLevelText() const;

        std::string palLicenseGetMudText() const;

        const tlicstring& palLicense1 = license1;
        const tlicstring& palLicense2 = license2;
        const tlicstring& palLicense3 = license3;
        const tlicstring& palLicense4 = license4;
        const tlicstring& palLicense5 = license5;
        const tlicstring& palLicense6 = license6;

        void palLicenseClear();

        /*
        { looks like this
        CPLEX Wrapper 0. ALPHA  6Feb08 22.7.1 WIN 3182.3187 VIS x86/MS Windows
        $                $            $  $ $ $   $    $    $   $
        $                $            $  $ $ $   $    $    $   $
        $                $            $  $ $ $   $    $    $   $ build code long text
        $                $            $  $ $ $   $    $    $ buildcode
        $                $            $  $ $ $   $    $ global revision
        $                $            $  $ $ $   $ local revison
        $                $            $  $ $ $ platform
        $                $            $  $ $ gold rel
        $                $            $  $ minrel
        $                $            $ majrel
        $                $ system date
        $ system name
        }
        */
        std::string palGetAuditLine() const { return gdlAuditLine; }

        void palAuditFields(const std::string &auditline,
                            std::string &v1,
                            std::string &v2,
            std::string& v3) const;

        std::string palGetShortAuditLine() const;

        void palSetAuditLine(const std::string& auditLine);

        bool palAuditRun() const;

        int palGetVer() const;
        std::string palGetCpr() const;
        std::string palGetCod() const;
        std::string palGetHdr() const;
        int palGetJul() const;
        std::string palGetBldCod() const;
        std::string palGetRevision() const;

        bool palLicenseReadU(const std::string& filename, std::string& msg, int& rc);

        std::string palLicenseGetPlatform() const;
        bool palLicenseValidateForPlatform(const std::string& pf);

        int palGetJulianDays(const std::string& s) const;

        int palLicenseGetJulBase() const;

        int palLicenseGetJulLice() const;

        void palLicenseRegisterSystem(int numcodes, const std::string& codes, int magicnum);

        void palLicenseRegisterGAMS(int linenr, const std::string &liceline);

        int palLicenseGetVersion();
        void palLicenseRegisterGAMSDone();

        bool palTamperCheck();


        void setSystemDirectory(const std::string& s);

        bool palLicenseValidation() const;

        void setLicenseStatus(int v);

        int getLicenseStatus() const;

        const std::array<std::string, 8> licenseStatusTxts = {
            "license validated successfully",
            "license status unknown",
            "could not open specified license file",
            "license validation failed",
            "tamper check failed",
            "license file format too old",
            "evaluation license has expired",
            "maintenance has expired",
        };

        std::string palLicenseStatusMessage();

        int palLicenseStatus() const;

        std::string palLicenseGetDC() const;

        std::string palLicenseGetID() const;

        std::string palLicenseGetLicensee();

        std::string palLicenseGetInstitution() const;

        std::string palLicenseGetPlatformText() const;

        std::string palLicenseGetInstDC() const;

        bool palLicenseIsAcademic() const;

        bool palLicenseCheckSubSys(const std::string& codes);

        bool palIsBeta() const;

        bool palIsAlpha() const;

        bool palLicenseHas6Lines() const;

        int LNumToInt(const std::string& s) const;

        int palLicenseGetEvalDate();

        int palLicenseGetMaintDate();

        std::string palLicenseGetTLType();

        bool palLicenseSolverCheckSizes(const std::string& codes, int M, int N, int NZ, int NLNZ, int NDisc);

        bool palLicenseSolverCheck(const std::string& codes);

        std::string palLicenseGetVendor();

        bool palLicenseGetMessage(std::string& msg) const;

        bool palLicenseCheckSubX(const std::string &sname, const std::string &Codes, int &daysleft);

        int palLicenseGetSubMaint();

        void palSetSystemName(const std::string& sname);

        void palLicenseSetSubSearch();

        bool palLicenseGetDates(std::string &Lcode, int &eval, int &maint);
        bool palLicenseGetSubNext();

        std::string LicenseGetSubString();
        int LicenseGetSubEval();
        int LicenseGetSubMaint();
    };
}
