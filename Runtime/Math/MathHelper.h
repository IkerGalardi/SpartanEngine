/*
Copyright(c) 2016-2020 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES ====
#include <cmath>
#include <limits>
#include <random>
//===============

namespace Spartan::Math
{
	enum Intersection
	{
		Outside,
		Inside,
		Intersects
	};

	constexpr float M_EPSILON	= 0.000001f;
	constexpr float PI			= 3.14159265359f;
	constexpr float PI_2		= 6.28318530718f;
	constexpr float PI_DIV_2	= 1.57079632679f;
	constexpr float PI_DIV_4	= 0.78539816339f;
	constexpr float PI_INV		= 0.31830988618f;
	constexpr float DEG_TO_RAD	= PI / 180.0f;
	constexpr float RAD_TO_DEG	= 180.0f / PI;

    inline double Cot(float x)							    { return cos(x) / sin(x); }
    inline float CotF(float x)							    { return cosf(x) / sinf(x); }
	constexpr float DegreesToRadians(const float degrees)	{ return degrees * DEG_TO_RAD; }
	constexpr float RadiansToDegrees(const float radians)	{ return radians * RAD_TO_DEG; }

	template <typename T>
	constexpr T Clamp(T x, T a, T b) { return x < a ? a : (x > b ? b : x); }

	// Lerp linearly between to values
	template <class T, class U>
	constexpr T Lerp(T lhs, T rhs, U t) { return lhs * (static_cast<U>(1) - t) + rhs * t; }

	// Returns the absolute value
	template <class T>
	constexpr T Abs(T value) { return value >= static_cast<T>(0) ? value : -value; }

	// Check for equality but allow for a small error
	template <class T>
	constexpr bool Equals(T lhs, T rhs, T error = std::numeric_limits<T>::epsilon()) { return lhs + error >= rhs && lhs - error <= rhs; }

	template <class T>
	constexpr T Max(T a, T b) { return a > b ? a : b; }

    template <class T>
    constexpr T Max3(T a, T b, T c) { return Max(a, Max(b, c)); }

	template <class T>
	constexpr T Min(T a, T b) { return a < b ? a : b; }

    template <class T>
    constexpr T Min3(T a, T b, T c) { return Min(a, Min(b, c)); }

	template <class T>
	constexpr T Sqrt(T x) { return sqrt(x); }

	template <class T>
	constexpr T Floor(T x) { return floor(x); }

	template <class T>
	constexpr T Ceil(T x) { return ceil(x); }

	template <class T>
	constexpr T Round(T x) { return round(x); }

    template <class T>
    constexpr T Tan(T x) { return tan(x); }

	template <typename T>
	constexpr int Sign(T x) { return (static_cast<T>(0) < x) - (x < static_cast<T>(0)); }

	template <class T>
	constexpr T Pow(T x, T y) { return pow(x, y); }

    template <class T>
    constexpr T Saturate(T x) { return Clamp(x, static_cast<T>(0), static_cast<T>(1)); }

    template <class T>
    inline T Random(T from = static_cast<T>(0), T to = static_cast<T>(1))
    {
        std::random_device rd;                              // obtain a random number from hardware
        std::mt19937 eng(rd());                             // seed the generator
        std::uniform_real_distribution<T> distr(from, to);  // define the range
        return distr(eng);
    }
}
