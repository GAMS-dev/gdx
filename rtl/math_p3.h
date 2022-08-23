#pragma once

#include <set>

namespace rtl::math_p3 {

    enum TFPUException {
        exInvalidOp, exDenormalized, exZeroDivide,
        exOverflow, exUnderflow, exPrecision
    };
    using TFPUExceptionMask = std::set<TFPUException>;

    double LnXP1(double x);

    TFPUExceptionMask GetExceptionMask();
    TFPUExceptionMask SetExceptionMask(const TFPUExceptionMask &Mask);
    void SetExceptionMask2P3();
    void ClearExceptions();

    double IntPower(double X, int I);

}