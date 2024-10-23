#include <pch.h>
#include "Colors.h"

namespace Colors
{
    static inline float Convert_sRGB_FromLinear(float theLinearValue)
    {
        return theLinearValue <= 0.0031308f
            ? theLinearValue * 12.92f
            : powf(theLinearValue, 1.0f / 2.2f) * 1.055f - 0.055f;
    }

    static inline float Convert_sRGB_ToLinear(float thesRGBValue)
    {
        return thesRGBValue <= 0.04045f
            ? thesRGBValue / 12.92f
            : powf((thesRGBValue + 0.055f) / 1.055f, 2.2f);
    }

    ImVec4 ConvertFromSRGB(ImVec4 colour)
    {
        return ImVec4(Convert_sRGB_FromLinear(colour.x),
            Convert_sRGB_FromLinear(colour.y),
            Convert_sRGB_FromLinear(colour.z),
            colour.w);
    }

    ImVec4 ConvertToSRGB(ImVec4 colour)
    {
        return ImVec4(std::pow(colour.x, 2.2f),
            std::pow(colour.y, 2.2f),
            std::pow(colour.z, 2.2f),
            colour.w);
    }
}
