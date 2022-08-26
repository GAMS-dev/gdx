#include "paldoorg.h"
#include <map>
#include "../expertapi/palmcc.h"
#include "../global/gmsglobx2.h"
#include "../global/delphitypes.h"
#include "../utils.h"

#include "../global/modhead.h"

using namespace global::gmsglobx2;
using namespace global::delphitypes;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace palxxx::paldoorg {
    std::string strGetFromPal(palHandle_t obj, const std::function<char *(palHandle_t, char *)> &f) {
        static std::array<char, 256> buf{};
        return f(obj, buf.data());
    }

    void tpalobject::updateFields()
    {
        static std::array<char, 256> lineBuf {};
        for (int i{}; i < 6; i++) {
            ::palLicenseGetLLine(obj, i+1, lineBuf.data());
            licenses[i]->assign(lineBuf.data());
        }
        LicenseLevel = ::palLicenseLevel(obj);
        gdlAuditLine = strGetFromPal(obj, ::palGetAuditLine);
        LicenseVersion = palLicenseGetVersion();
        LicenseStatus = 1;
    }

    bool tpalobject::licensecheckV1to3(int v1, int v2, int v3)
    {
        STUBWARN();
        return false;
    }

    static int val1{3}, val2{97}, val3{13};

    tpalobject::tpalobject()
    {
        bool succ{ ::palCreate(&obj, msgBuf.data(), (int)msgBuf.size()) != 0 };
        std::string msg{ msgBuf.data()+1 };
        if (!succ && !msg.empty()) {
            std::cout << "WARNING: Problem loading PAL object library: "s << msg << std::endl;
            obj = nullptr;
            return;
        }
        updateFields();
    }

    tpalobject::~tpalobject() {
        if(!obj) return;
        ::palFree(&obj);
    }

    std::string tpalobject::palGetRelDat() const {
        return strGetFromPal(obj, ::palGetRelDat);
    }

    std::string tpalobject::palGetLicDat() const {
        return strGetFromPal(obj, ::palGetLicDat);
    }

    std::string tpalobject::palGetRel() const {
        return strGetFromPal(obj, ::palGetRel);
    }

    std::string tpalobject::palGetGold() const {
        return strGetFromPal(obj, ::palGetGold);
    }

    std::string tpalobject::palLicenseDisplay(int i) const {
        return i - 1 < 0 || i - 1 >= licenses.size() ? "****Unknown license display line" : *licenses[i - 1];
    }

    int tpalobject::palLicenseGetKey() const {
        int i, res;
        utils::val(license3.substr(0, 8), res, i);
        if (i) res = 0;
        return res;
    }

    int tpalobject::palLicenseGetLevel() const {
        return LicenseLevel;
    }

    bool tpalobject::palSecurityCheck() const {
        STUBWARN();
        return true;
    }

    void tpalobject::palLicenseClear() {
        LicenseStatus = 1;
        LicenseLevel = -1;
        license1 = "license1 not set";
        license2 = "license2 not set";
        license3 = license4 = license5 = license6 = "";
        CurrentSearchHelper = LicenseMaxSub + 1;
    }

    void tpalobject::palAuditFields(const std::string& auditline, std::string& v1, std::string& v2, std::string& v3) const {
        std::array<std::array<char, 256>, 3> bufs{};
        ::palAuditFields(obj, auditline.c_str(), bufs[0].data(), bufs[1].data(), bufs[2].data());
        v1.assign(bufs[0].data());
        v2.assign(bufs[1].data());
        v3.assign(bufs[2].data());
    }

    std::string tpalobject::palGetShortAuditLine() const {
        return strGetFromPal(obj, ::palGetShortAuditLine);
    }

    void tpalobject::palSetAuditLine(const std::string& auditLine) {
        ::palSetAuditLine(obj, auditLine.c_str());
        gdlAuditLine = auditLine;
    }

    bool tpalobject::palAuditRun() const {
        return ::palAuditRun(obj);
    }

    int tpalobject::palGetVer() const { return ::palGetVer(obj); }

    std::string tpalobject::palGetCpr() const { return strGetFromPal(obj, ::palGetCpr); }

    std::string tpalobject::palGetCod() const { return strGetFromPal(obj, ::palGetCod); }

    std::string tpalobject::palGetHdr() const { return strGetFromPal(obj, ::palGetHdr); }

    int tpalobject::palGetJul() const { return ::palGetJul(obj); }

    std::string tpalobject::palGetBldCod() const { return strGetFromPal(obj, ::palGetBldCod); }

    std::string tpalobject::palGetRevision() const { return strGetFromPal(obj, ::palGetRevision); }

    bool tpalobject::palLicenseReadU(const std::string& filename, std::string& msg, int& rc)
    {
        msg.clear();
        bool res = ::palLicenseReadU(obj, filename.c_str(), msgBuf.data(), &rc);
        if(!res) msg.assign(msgBuf.data());
        updateFields();
        return res;
    }

    std::string tpalobject::palLicenseGetPlatform() const
    {
        return strGetFromPal(obj, ::palLicenseGetPlatform);
    }

    bool PlatformVals(int n, int& val1, int& val2, int& val3) {
        std::map<int, std::tuple<int, int, int>> nToVals = {
             {  1, {  3, 97, 13}},{
                 2, {  7, 19, 83}},{
                 3, { 79, 23, 11}},{
                 4, { 73, 23, 13}},{
                 5, {  7, 83, 19}},{
                 6, {  3, 97, 13}},{
                 7, { 11, 79, 19}},{
                 8, {  7, 83, 19}},{
                 9, {  7, 19, 83}},{
                10, { 73, 23, 13}},{
                11, { 79, 23, 11}},{
                12, { 79, 23, 11}},{
                13, {  5, 89, 19}},{
                14, {  5, 89, 19}}
        };
        if (utils::in(n, nToVals)) {
            const auto vals = nToVals.at(n);
            val1 = std::get<0>(vals);
            val2 = std::get<1>(vals);
            val3 = std::get<2>(vals);
            return true;
        }
        return false;
    }

    bool tpalobject::palLicenseValidateForPlatform(const std::string& pf)
    {
        std::array<int, 3> vs{};
        PlatformVals(global::gmsglobx2::PlatformsLookup(pf), vs[0], vs[1], vs[2]);
        return licensecheckV1to3(vs[0], vs[1], vs[2]);
    }

    int tpalobject::palGetJulianDays(const std::string& s) const
    {
        return ::palGetJulianDays(obj, s.c_str());
    }

    int tpalobject::palLicenseGetJulBase() const {
        int res = palGetJulianDays(license1.substr(48, 6));
        if (LicenseVersion > 1) res -= 30 * (std::stoi(license1.substr(55, 2)) - 1);
        return res;
    }

    int tpalobject::palLicenseGetJulLice() const {
        return palGetJulianDays(license1.substr(48, 6));
    }

    void tpalobject::palLicenseRegisterSystem(int numcodes, const std::string& codes, int magicnum) {
        ::palLicenseRegisterSystem(obj, numcodes, codes.c_str(), magicnum);
    }

    void tpalobject::palLicenseRegisterGAMS(int linenr, const std::string &liceline) {
        ::palLicenseRegisterGAMS(obj, linenr, liceline.c_str());
    }

    int tpalobject::palLicenseGetVersion() {
        char c = license1[54];
        switch(c) {
            case '|': return 3;
            case '/': return 2;
            case ':': return 1;
            default: return 0;
        }
    }

    void tpalobject::palLicenseRegisterGAMSDone() {
        ::palLicenseRegisterGAMSDone(obj);
    }

    void tpalobject::setSystemDirectory(const std::string& s) {
        //sysDir = s;
    }

    bool tpalobject::palLicenseValidation() const {
        return ::palLicenseValidation(obj);
    }

    void tpalobject::setLicenseStatus(int v) {
        LicenseStatus = v;
    }

    int tpalobject::getLicenseStatus() const {
        return LicenseStatus;
    }

    std::string tpalobject::palLicenseStatusMessage() {
        return LicenseStatus >= 0 && LicenseStatus < licenseStatusTxts.size() ? licenseStatusTxts[LicenseStatus] : "*** no message found ***";
    }

    int tpalobject::palLicenseStatus() const {
        return LicenseStatus;
    }

    std::string tpalobject::palLicenseGetDC() const {
        auto i = license5.find(' ');
        return i == std::string::npos ? "" : license5.substr(0, i);
    }

    std::string tpalobject::palLicenseGetID() const {
        return license1.substr(47, 18);;
    }

    std::string tpalobject::palLicenseGetLicensee() {
        return utils::trim(license1.substr(0, 47));
    }

    [[nodiscard]]
    std::string tpalobject::palLicenseGetInstitution() const {
        return utils::trim(license2);
    }

    std::string tpalobject::palLicenseGetPlatformText() const {
        std::string xxx = palLicenseGetPlatform();
        return utils::sameText(xxx, "any") ? "Any platform demo system" : global::gmsglobx2::PlatformsText(global::gmsglobx2::PlatformsLookup(xxx));
    }

    [[nodiscard]]
    std::string tpalobject::palLicenseGetInstDC() const {
        std::string dc{ palLicenseGetDC() };
        std::string inst{ palLicenseGetInstitution() };
        const int targetWidth = 65;
        const int numBlanks = targetWidth - (int)dc.length() - (int)inst.length();
        return inst + (numBlanks > 0 ? std::string(numBlanks, ' ') : "") + dc;
    }

    bool tpalobject::palLicenseIsAcademic() const {
        return ::palLicenseIsAcademic(obj);
    }

    bool tpalobject::palLicenseCheckSubSys(const std::string& codes) {
        return ::palLicenseCheckSubSys(obj, codes.c_str());
    }

    bool tpalobject::palIsBeta() const {
        return ::palIsBeta(obj);
    }

    bool tpalobject::palIsAlpha() const {
        return ::palIsAlpha(obj);
    }

    bool tpalobject::palLicenseHas6Lines() const {
        return license1[54] == '|';
    }

    int tpalobject::LNumToInt(const std::string& s) const {
        if (s.empty() || s.length() > 1) return 0;
        else if (LicenseVersion == 1) return static_cast<int>(s.front() - '0');
        else if (LicenseVersion > 1) {
            if (isdigit(s[0])) return static_cast<int>(s.front() - '0');
            else if (isupper(s.front())) return static_cast<int>(s.front() - 'A') + 10;
            else if (islower(s.front())) return static_cast<int>(s.front() - 'a') + 36;
            else return 0;
        }
        else return 0;
    }

    int tpalobject::palLicenseGetEvalDate() {
        int r{ LNumToInt(LicenseVersion < 2 ? utils::substr(license3, 64, 1) : utils::substr(license4, 9, 1)) };
        if (!r) return std::numeric_limits<int>::max();
        else return palLicenseGetJulLice() + 30 * r;
    }

    int tpalobject::palLicenseGetMaintDate()
    {
        if (LicenseVersion >= 1)
            return palLicenseGetJulBase() + 30 * (LicenseVersion == 1 ? std::stoi(license4.substr(8, 2)) : LNumToInt(license4.substr(8, 1)));
        return std::numeric_limits<int>::max();
    }

    std::string tpalobject::palLicenseGetTLType() {
        if (palLicenseGetEvalDate() == std::numeric_limits<int>::max())
            return "Not a time limited license"s;
        else {
            std::string res{ utils::trim(license5.substr(48, 16)) };
            if (utils::sameText("eval"s, res)) res = "EVALUATION"s;
            int n{ global::gmsglobx2::TLLicenseLookup(res) };
            return n == -1 ? "Other time limited license" : global::gmsglobx2::TLLicenseText(n);
        }
    }

    bool tpalobject::palLicenseSolverCheckSizes(const std::string& codes, int M, int N, int NZ, int NLNZ, int NDisc) {
        return ::palLicenseSolverCheckSizes(obj, codes.c_str(), M, N, NZ, NLNZ, NDisc);
    }

    bool tpalobject::palLicenseSolverCheck(const std::string& codes) {
        return ::palLicenseSolverCheck(obj, codes.c_str());
    }

    bool tpalobject::palLicenseGetMessage(std::string& msg) const {
        msg.resize(256);
        return ::palLicenseGetMessage(obj, msg.data(), (int)msg.size());
    }

    bool tpalobject::palLicenseCheckSubX(const std::string &sname, const std::string &Codes, int &daysleft) {
        return ::palLicenseCheckSubX(obj, sname.c_str(), Codes.c_str(), &daysleft);
    }

    void BlankFill(std::string& t, int maxt, const std::string& s) {
        t.resize(maxt);
        for (int i = 1; i <= maxt; i++)
            t[i] = i < s.length() ? s[i] : ' ';
    }

    void tpalobject::palSetSystemName(const std::string& sname) {
        if(!obj) return;
        ::palSetSystemName(obj, sname.c_str());
        gdlAuditLine = strGetFromPal(obj, ::palGetAuditLine);
    }

    bool tpalobject::palTamperCheck() {
        // TODO: This is not exposed by palmcc.h, hence always return false (not tampered) for now
        return false;
    }

    void tpalobject::palLicenseSetSubSearch() {
        CurrentSearchHelper = 0;
    }

    bool tpalobject::palLicenseGetDates(std::string &Lcode, int &eval, int &maint) {
        while(palLicenseGetSubNext()) {
            Lcode = LicenseGetSubString();
            eval = LicenseGetSubEval();
            maint = LicenseGetSubMaint();
            if(!eval) {
                eval = std::numeric_limits<int>::max();
                maint = !maint ? eval : palLicenseGetJulBase() + 30*maint;
            } else {
                eval = palLicenseGetJulLice() + 30*eval;
                maint = eval;
            }
            return true;
        }
        Lcode.clear();
        eval = maint = 0;
        return false;
    }

    bool tpalobject::palLicenseGetSubNext() {
        // FIXME: LicenseActSub not correctly used/set beforehand
        if(CurrentSearchHelper > LicenseActSub) return false;
        CurrentSearchHelper++;
        if(license3[7+2*CurrentSearchHelper] == ' ')
            CurrentSearchHelper = LicenseActSub + 1;
        else return true;
        return false;
    }

    std::string tpalobject::LicenseGetSubString() {
        return license3.substr(7+2*CurrentSearchHelper, 2);
    }

    int tpalobject::LicenseGetSubEval() {
        return LicenseVersion < 2 ? license3[66-CurrentSearchHelper] - '0' : LNumToInt(license4.substr(7+2*CurrentSearchHelper+1, 1));
    }

    int tpalobject::LicenseGetSubMaint() {
        if(LicenseVersion == 1) return std::stoi(license4.substr(7+2*CurrentSearchHelper, 2));
        else if(LicenseVersion > 1) return LNumToInt(license4.substr(7+2*CurrentSearchHelper, 1));
        return 0;
    }

    std::string GamsLicensesText(int n)
    {
        const std::array<std::string, 7> lictxtsSuffixes = {
            "Demo"s,
            "Professional"s,
            "Run Time"s,
            "Secured Run Time"s,
            "Nodelock"s,
            "Community"s,
            "No Solve Professional"s
        };
        return n >= 1 && n <= 7 ? "GAMS/"s + lictxtsSuffixes[n-1] : "**** should never happen"s;
    }

}
