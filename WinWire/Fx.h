//
// Copyright 2015 Muhamad Lotfy
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include "common.h"

namespace WinWire {
    class Fx
    {
    public:

        static Fx& Inst()
        {
            static Fx inst;
            return inst;
        }

        bool Init()
        {
            GlobalShutdownEvt = CreateEvent(NULL, TRUE, FALSE, NULL);

            (void)QueryPerformanceCounter(&WinWireT0);
            (void)QueryPerformanceFrequency(&HpcFreq);

            HpcPerdiodNs = 1000000000.0 / HpcFreq.QuadPart;
            HpcMicroNumTicks = (double)HpcFreq.QuadPart / 1000000.0;

            LogInfo("Initializing WinWire Library");

#ifdef SetPriorityClass
            if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
            {
                LogError("SetPriorityClass REALTIME_PRIORITY_CLASS failed, %d", GetLastError());
                return false;
            }
#else
            LogWarning("SetPriorityClass is not available, process will run in normal priority");
#endif

            LogInfo("HPC Info: Freq=%lldHz, Period=%fns 1MicroNumTicks=%f", HpcFreq.QuadPart, HpcPerdiodNs, HpcMicroNumTicks);

#ifdef DEBUG_TIMING

            LARGE_INTEGER t0, t1, t2, t3;
            QueryPerformanceCounter(&t0);
            QueryPerformanceCounter(&t1);
            QueryPerformanceCounter(&t2);
            QueryPerformanceCounter(&t3);
            LogInfo("t0=%lld\nt1=%lld\nt2=%lld\nt3=%lld\n",
                t0.QuadPart, t1.QuadPart, t2.QuadPart, t3.QuadPart);

            LogInfo("t1-t0=%lld~%lldns\nt2-t1=%lld~%lldns\nt3-t2=%lld~%lldns",
                t1.QuadPart - t0.QuadPart, ((t1.QuadPart - t0.QuadPart) * 1000000000) / HpcFreq.QuadPart,
                t2.QuadPart - t1.QuadPart, ((t2.QuadPart - t1.QuadPart) * 1000000000) / HpcFreq.QuadPart,
                t3.QuadPart - t2.QuadPart, ((t3.QuadPart - t2.QuadPart) * 1000000000) / HpcFreq.QuadPart);

            LogInfo("\nStart Testing HPC");
            MicroDelay(1);
            MicroDelay(10);
            MicroDelay(100);
            MicroDelay(1000);
            LogInfo("End Testing HPC\n");

            ArrowResult arrowTestRes;
            arrowTestRes.ThrowBackTimeUs = 0;
            arrowTestRes.ThrowTimeUs = 0;
            const int ArrowNumSamples = 100;

            for (int i = 0; i < ArrowNumSamples; ++i)
            {
                ArrowResult currRes = FxKm::Inst().Arrow();

                LogVerbose(
                    "Sample%d: User->Kernel=%lldus, Kernel->User=%lldus",
                    i,
                    currRes.ThrowTimeUs,
                    currRes.ThrowBackTimeUs);

                arrowTestRes.ThrowTimeUs += currRes.ThrowTimeUs;
                arrowTestRes.ThrowBackTimeUs += currRes.ThrowBackTimeUs;
            }

            arrowTestRes.ThrowTimeUs /= (LONGLONG)ArrowNumSamples;
            arrowTestRes.ThrowBackTimeUs /= (LONGLONG)ArrowNumSamples;

            LogInfo(
                "KMDF IOCTL Timing: User->Kernel=%lldus, Kernel->User=%lldus, User<->Kernel= %lldus over %d samples",
                arrowTestRes.ThrowTimeUs,
                arrowTestRes.ThrowBackTimeUs,
                arrowTestRes.ThrowTimeUs + arrowTestRes.ThrowBackTimeUs,
                ArrowNumSamples);

#endif
            return true;
        }

        void Shutdown()
        {
            GlobalShutdownFlag = true;
            SetEvent(GlobalShutdownEvt);
        }

        volatile bool GlobalShutdownFlag;
        HANDLE GlobalShutdownEvt;
    };
}