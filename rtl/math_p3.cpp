#include "math_p3.h"
#include "../global/modhead.h"
#include <cmath>

namespace rtl::math_p3 {

    double IntPower(double X, int I) {
        double res {1.0};
        for(int Y {std::abs(I)}; Y > 0; Y--, res *= X) {
            while(!(Y % 2)) {
                Y >>= 1;
                X*=X;
            }
        }
        if(I < 0) res = 1.0 / res;
        return res;
    }

    double LnXP1(double x) {
        return log1p(x);
    }

    TFPUExceptionMask GetExceptionMask() {
        // ...
        STUBWARN();
        return {};
    }

    TFPUExceptionMask SetExceptionMask(const TFPUExceptionMask &Mask) {
        STUBWARN();
        // ...
        return {};
    }

    void SetExceptionMask2P3() {
        SetExceptionMask({exDenormalized, exUnderflow, exPrecision, exInvalidOp, exZeroDivide, exOverflow});
    }

    void ClearExceptions() {
        STUBWARN();
        // ...
    }

}