#pragma once

#include "raster.h"

// Ratio class implementation from https://stackoverflow.com/a/50976626

namespace Raster {
    // counts number of subsequent least significant bits equal to 0
    // example: for 1001000 (binary) returns 3
    static uint32_t CountTrailingZeroes(uint32_t v) {
        uint32_t counter = 0;

        while(counter < 32 && (v & 1u) == 0)
        {
            v >>= 1u;
            ++counter;
        }

        return counter;
    }

    struct Ratio {
        int64_t numerator; // numerator includes sign
        uint64_t denominator;

        float ToFloat() const
        {
            return static_cast<float>(numerator) / denominator;
        }

        static Ratio FromFloat(float v)
        {
            // First, obtain bitwise representation of the value
            const uint32_t bitwiseRepr = *reinterpret_cast<uint32_t*>(&v);

            // Extract sign, exponent and mantissa bits (as stored in memory) for convenience:
            const uint32_t signBit = bitwiseRepr >> 31u;
            const uint32_t expBits = (bitwiseRepr >> 23u) & 0xffu; // 8 bits set
            const uint32_t mntsBits = bitwiseRepr & 0x7fffffu; // 23 bits set

            // Handle some special cases:
            if(expBits == 0 && mntsBits == 0)
            {
                // special case: +0 and -0
                return {0, 1};
            }
            else if(expBits == 255u && mntsBits == 0)
            {
                // special case: +inf, -inf
                // Let's agree that infinity is always represented as 1/0 in Ratio 
                return {signBit ? -1 : 1, 0};
            }
            else if(expBits == 255u)
            {
                // special case: nan
                // Let's agree, that if we get NaN, we returns max int64_t by 0
                return {std::numeric_limits<int64_t>::max(), 0};
            }

            // mask lowest 23 bits (mantissa)
            uint32_t significand = (1u << 23u) | mntsBits;

            const uint32_t nTrailingZeroes = CountTrailingZeroes(significand);
            significand >>= nTrailingZeroes;

            const int64_t signFactor = signBit ? -1 : 1;

            const int32_t exp = expBits - 127 - 23 + nTrailingZeroes;

            if(exp < 0)
            {
                return {signFactor * static_cast<int64_t>(significand), 1u << static_cast<uint32_t>(-exp)};
            }
            else
            {
                return {signFactor * static_cast<int64_t>(significand * (1u << static_cast<uint32_t>(exp))), 1};
            }
        }
    };
}