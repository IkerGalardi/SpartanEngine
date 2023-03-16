/*
Copyright(c) 2016-2023 Panos Karabelas

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

//= INCLUDES ===========
#include "Definitions.h"
//======================

namespace Spartan
{
    enum class FpsLimitType
    {
        Unlocked,
        Fixed,
        FixedToMonitor
    };

    class SP_CLASS Timer
    {
    public:
        static void Initialize();
        static void Tick();

        // FPS Limit
        static void SetFpsLimit(double fps);
        static double GetFpsLimit();
        static FpsLimitType GetFpsLimitType();

        // Times
        static double GetTimeMs();
        static double GetTimeSec();
        static double GetDeltaTimeMs();
        static double GetDeltaTimeSec();
        static double GetDeltaTimeSmoothedMs();
        static double GetDeltaTimeSmoothedSec();
    };
}
