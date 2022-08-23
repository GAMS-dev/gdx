
#pragma once
#include <stdexcept>
#include <string>
#include <sstream>
// only supported by MSVC so far :(
//#include <format>
#include <functional>
#include <set>
#include <cmath>


// Interface
namespace global::delphitypes {
    template<typename T>
    inline void FreeAndNil(T* &ptr) {
        if (ptr) {
            delete ptr;
            ptr = nullptr;
        }
    }

    // According to Embarcadero docs
    // http://docwiki.embarcadero.com/RADStudio/Sydney/en/Simple_Types_(Delphi)
    using Byte = uint8_t; // unsigned char
    using Word = uint16_t; // unsigned short
    using LongWord = unsigned;
    using Cardinal = uint32_t; // unsigned int
    using Longint = int;
    using Int32 = int32_t ; // int
    using Int64 =  int64_t  ; // long long
    using Shortint = signed char ;
    using Integer = int;
    using Smallint = int16_t; // short

    using tDateTime = double;
    using Text = std::fstream*;

    // Both bounds are inclusive
    template<typename T, T lowerBoundIncl, T upperBoundIncl>
    class Bounded {
        T value;
    public:
        Bounded() : value(lowerBoundIncl) {}

        Bounded(T initialValue) : value(initialValue) {
            checkBounds();
        }

        Bounded& operator=(const T& rhs) {
            value = rhs;
            checkBounds();
            return *this;
        }

        Bounded& operator+=(const T& rhs) {
            value += rhs;
            checkBounds();
            return *this;
        }

        Bounded& operator-=(const T& rhs) {
            value -= rhs;
            checkBounds();
            return *this;
        }

        operator T() const {
            return value;
        }

        // Prefix increment (apply and then read)
        Bounded& operator++() {
            value += 1;
            checkBounds();
            return *this;
        }

        // Prefix decrement (apply and then read)
        Bounded& operator--() {
            value -= 1;
            checkBounds();
            return *this;
        }

        // Postfix increment (read and then apply)
        Bounded operator++(int) {
            Bounded temp = *this;
            ++*this;
            return temp;
        }

        // Postfix decrement (read and then apply)
        Bounded operator--(int) {
            Bounded temp = *this;
            --*this;
            return temp;
        }

        inline void checkBounds() {
            if (value < lowerBoundIncl || value > upperBoundIncl) {
                std::stringstream msgStream;
                msgStream << value << " is out of bounded range [" << lowerBoundIncl << "," << upperBoundIncl << "]";
                throw std::out_of_range(msgStream.str());
            }
        }

        constexpr static T getLowerBound() {
            return lowerBoundIncl;
        }

        constexpr static T getUpperBound() {
            return upperBoundIncl;
        }

        T *getStorage() {
            return &value;
        }
    };

    template<typename T>
    std::set<T> setUnion(const std::set<T>& A, const std::set<T>& B) {
        std::set<T> C = A;
        C.insert(B.begin(), B.end());
        return C;
    }

    inline double frac(double v) {
        return v - trunc(v);
    }

}