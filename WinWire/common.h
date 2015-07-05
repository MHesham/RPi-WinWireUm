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

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <crtdbg.h>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include "HpcTimer.h"

#define SAFE_CLOSE(X)	if (X) { CloseHandle(X); (X) = NULL; }
#define ROUNDUP(X, M)	((((X) / (M)) * (M)) + ((X) % (M) > 0 ? (M) : 0))

namespace WinWire
{
    static LARGE_INTEGER HpcFreq;
    static LARGE_INTEGER WinWireT0;
    static double HpcMicroNumTicks;
    static double HpcPerdiodNs;

    void DbgPrintf(const wchar_t* pTxtFormat, ...)
    {
        const size_t LogBufferMax = 2048;
        wchar_t buffer[LogBufferMax];

        va_list formatArgs;
        va_start(formatArgs, pTxtFormat);
        vswprintf_s(buffer, pTxtFormat, formatArgs);
        va_end(formatArgs);

#ifdef DEBUG_LOG
        LARGE_INTEGER t1;
        (void)QueryPerformanceCounter(&t1);
        LARGE_INTEGER elapsedMillis;

        elapsedMillis.QuadPart = t1.QuadPart - WinWireT0.QuadPart;
        elapsedMillis.QuadPart *= 1000;
        elapsedMillis.QuadPart /= HpcFreq.QuadPart;

        wprintf_s(L"[%lld]", elapsedMillis.QuadPart);
#endif

        wprintf_s(buffer);

        if (IsDebuggerPresent())
            OutputDebugStringW(buffer);
    }

#define LogInfo(S, ...) WinWire::DbgPrintf(L##S##"\n", __VA_ARGS__)

#ifdef LOG_VERBOSE
#define LogVerbose(S, ...) WinWire::DbgPrintf(L##S##"\n", __VA_ARGS__)
#define LogFuncEnter() WinWire::DbgPrintf(L##"->" __FUNCTION__ "\n")
#define LogFuncExit() WinWire::DbgPrintf(L##"<-" __FUNCTION__ "\n")
#else
#define LogVerbose(S, ...)
#define LogFuncEnter()
#define LogFuncExit()

#endif

#define LogError(S, ...) WinWire::DbgPrintf(L##"Error: "##S##"\n", __VA_ARGS__)

    void MicroDelay(unsigned us)
    {
        LARGE_INTEGER t0, t1, dTicks;

        dTicks.QuadPart = (LONGLONG)((double)us * HpcMicroNumTicks);

        QueryPerformanceCounter(&t0);

        for (;;)
        {
            QueryPerformanceCounter(&t1);
            if (t1.QuadPart - t0.QuadPart > dTicks.QuadPart)
            {
#ifdef DEBUG_HPC
                LARGE_INTEGER elapsedMicros;

                elapsedMicros.QuadPart = t1.QuadPart - t0.QuadPart;
                elapsedMicros.QuadPart *= 1000000;
                elapsedMicros.QuadPart /= HpcFreq.QuadPart;

                LogInfo("MicroDelay(%dus) quitted after %lldus", us, elapsedMicros.QuadPart);
#endif
                return;
            }
        }
    }

    void MilliDelay(unsigned ms)
    {
        MicroDelay(ms * 1000);
    }


    int MapRange(int n, int n0, int n1, int m0, int m1)
    {
        double p = (double)(n - n0) / (double)(n1 - n0);
        return m0 + (int)((m1 - m0) * p);
    }

}