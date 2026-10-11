#pragma once
#include <cmath>
#if defined(ELMA_X87) && (defined(__i386__) || defined(__x86_64__))
inline double elma_math_sin(double value)
{
    __asm__ volatile ("fsin" : "+t" (value));
    return value;
}
inline double elma_math_cos(double value)
{
    __asm__ volatile ("fcos" : "+t" (value));
    return value;
}
inline double elma_math_sqrt(double value)
{
    double result;
    __asm__ volatile ("fsqrt" : "=t" (result) : "0" (value));
    return result;
}
inline double elma_math_atan2(double y, double x)
{
    double result;
    // FPATAN consumes ST(1)=y and ST(0)=x, matching the original executable.
    __asm__ volatile ("fpatan" : "=t" (result) : "0" (x), "u" (y) : "st(1)");
    return result;
}
#else
inline double elma_math_sin(double value) { return std::sin(value); }
inline double elma_math_cos(double value) { return std::cos(value); }
inline double elma_math_sqrt(double value) { return std::sqrt(value); }
inline double elma_math_atan2(double y, double x) { return std::atan2(y, x); }
#endif
