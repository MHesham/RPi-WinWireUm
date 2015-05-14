#pragma once

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winioctl.h>
#include <crtdbg.h>
#include "HpcTimer.h"

#define SAFE_CLOSE(X)	if (X) { CloseHandle(X); (X) = NULL; }
#define ROUNDUP(X, M)	((((X) / (M)) * (M)) + ((X) % (M) > 0 ? (M) : 0))

namespace Wi2Pi
{
	static LARGE_INTEGER HpcFreq;
	static LARGE_INTEGER Wi2PiT0;
	static double HpcMicroNumTicks;
	static double HpcPerdiodNs;

	void DbgPrintf(const wchar_t* pTxtFormat, ...)
	{
		const size_t LogBufferMax = 1024;
		wchar_t buffer[LogBufferMax];

		va_list formatArgs;
		va_start(formatArgs, pTxtFormat);
		vswprintf_s(buffer, pTxtFormat, formatArgs);
		va_end(formatArgs);

#ifdef DEBUG_LOG
		LARGE_INTEGER t1;
		(void)QueryPerformanceCounter(&t1);
		LARGE_INTEGER elapsedMillis;

		elapsedMillis.QuadPart = t1.QuadPart - Wi2PiT0.QuadPart;
		elapsedMillis.QuadPart *= 1000;
		elapsedMillis.QuadPart /= HpcFreq.QuadPart;

		wprintf_s(L"[%lld]", elapsedMillis.QuadPart);
#endif

		wprintf_s(buffer);

		if (IsDebuggerPresent())
			OutputDebugStringW(buffer);
	}

#define LogInfo(S, ...) Wi2Pi::DbgPrintf(L##S##"\n", __VA_ARGS__)

#ifdef LOG_VERBOSE
#define LogVerbose(S, ...) Wi2Pi::DbgPrintf(L##S##"\n", __VA_ARGS__)
#define LogFuncEnter() Wi2Pi::DbgPrintf(L##"->" __FUNCTION__ "\n")
#define LogFuncExit() Wi2Pi::DbgPrintf(L##"<-" __FUNCTION__ "\n")
#else
#define LogVerbose(S, ...)
#define LogFuncEnter()
#define LogFuncExit()

#endif

#define LogError(S, ...) Wi2Pi::DbgPrintf(L##"Error: "##S##"\n", __VA_ARGS__)

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