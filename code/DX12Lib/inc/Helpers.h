#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> 

#include <cstdint>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

namespace Math
{
    /***************************************************************************
    * These functions were taken from the MiniEngine.
    * Source code available here:
    * https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Math/Common.h
    * Retrieved: January 13, 2016
    **************************************************************************/
    template <typename T>
    inline T AlignUpWithMask(T value, size_t mask)
    {
        return (T)(((size_t)value + mask) & ~mask);
    }

    template <typename T>
    inline T AlignDownWithMask(T value, size_t mask)
    {
        return (T)((size_t)value & ~mask);
    }

    template <typename T>
    inline T AlignUp(T value, size_t alignment)
    {
        return AlignUpWithMask(value, alignment - 1);
    }

    template <typename T>
    inline T AlignDown(T value, size_t alignment)
    {
        return AlignDownWithMask(value, alignment - 1);
    }

    template <typename T>
    inline bool IsAligned(T value, size_t alignment)
    {
        return 0 == ((size_t)value & (alignment - 1));
    }
}