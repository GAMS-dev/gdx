#include "gmsspecs.h"

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace global::gmsspecs {
    std::array<int, styequb+1> equstypInfo;

    tgmsvalue mapval(double x) {
        if (x < valund) return xvreal;
        if (x >= valacr) return xvacr;
        x /= valund;
        int k = static_cast<int>(std::round(x));
        if (std::abs(k - x) > 1.0e-5)
            return xvund;

        constexpr std::array<tgmsvalue, 5> kToRetMapping = {
            xvund, xvna, xvpin, xvmin, xveps
        };
        return k >= 1 && k <= kToRetMapping.size() ? kToRetMapping[k-1] : xvacr;
    }

    txgmsvalue xmapval(double x) {
        if (x < valund) {
            if (x < 0) return vneg;
            else if (x == 0.0) return vzero; // epsilon check?
            return vpos;
        }
        return txgmsvalue((int)mapval(x) + 2);
    }
}
