/*
Copyright(c) 2016-2022 Panos Karabelas

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

//= INCLUDES ===================
#include "../Core/Definitions.h"
#include <array>
#include <mutex>
//==============================

namespace Spartan
{
    enum class ProgressType
    {
        model_importing,
        world_io,
        resource_cache_io
    };

    class Progress
    {
    public:
        void Start(const uint32_t job_count, const std::string& text);
        float GetFraction();
        bool IsLoading();
        void JobDone();
        const std::string& GetText();
        void SetText(const std::string& text);

    private:
        std::atomic<uint32_t> m_jobs_done = 0;
        std::atomic<uint32_t> m_job_count = 0;
        std::string m_text;
    };

    class SP_CLASS ProgressTracker
    {
    public:
        static Progress& GetProgress(const ProgressType progress_type)
        {
            static ProgressTracker instance;
            return instance.GetProgressInternal(progress_type);
        }

        static Progress& GetProgressInternal(const ProgressType progress_type);

    private:
        static std::array<Progress, 3> m_progresses;
        static std::mutex m_mutex_progress_access;
    };
}
