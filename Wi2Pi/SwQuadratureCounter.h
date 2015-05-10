#pragma once

#include "Wi2Pi.h"
#include <thread>
#include <functional>

namespace Wi2Pi
{
	class SwQuadratureCounter
	{
	public:
		static const int NA = INT_MAX;
		static const LONG CounterLockSpinCount = 5000;

		SwQuadratureCounter(int chAPin, int chBPin, int countsPerRev, std::function<void()> onTargetCounterReachedCallback) :
			LocalShutdownFlag(false),
			GpioBank0(0),
			lastChAB(0),
			Counter(0),
			MissedPulseCount(0),
			TargetCounter(INT_MAX),
			LocalShutdownEvt(TargetCounterReachedWaitableEvts[0]),
			TargetCounterReachedEvt(TargetCounterReachedWaitableEvts[1]),
			Direction(0),
			ChAPin(chAPin),
			ChBPin(chBPin),
			CountsPerRev(countsPerRev),
			OnTargetCounterReachedCallback(onTargetCounterReachedCallback),
			EncoderReadings(0)
		{
			T0.QuadPart = 0;
			T1.QuadPart = 0;

			CounterStateMachineTickCount = 0;
			CounterStateMachineStartTime.QuadPart = 0;

			ZeroMemory(TargetCounterReachedWaitableEvts, sizeof(TargetCounterReachedWaitableEvts));
		}

		~SwQuadratureCounter()
		{
			DeleteCriticalSection(&CounterLock);
		}

		bool Init()
		{
			GpioFuncSelect(ChAPin, BCM_GPIO_FSEL_Input);
			GpioFuncSelect(ChBPin, BCM_GPIO_FSEL_Input);

			if (!InitializeCriticalSectionAndSpinCount(&CounterLock, CounterLockSpinCount))
			{
				LogError("InitializeCriticalSectionAndSpinCount failed, error=%d", GetLastError());
				return false;
			}

			TargetCounterReachedEvt = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (!TargetCounterReachedEvt)
			{
				LogError("CreateEvent failed, error=%d", GetLastError());
				return false;
			}

			LocalShutdownEvt = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!LocalShutdownEvt)
			{
				LogError("CreateEvent failed, error=%d", GetLastError());
				return false;
			}

			CounterStateMachineThread = std::thread([&]() { CounterStateMachineWorker(); });
			if (!SetThreadPriority(CounterStateMachineThread.native_handle(), THREAD_PRIORITY_HIGHEST))
			{
				LogError("SetThreadPriority failed, error=%d", GetLastError());
				return false;
			}

			GlobalShutdownWatcherThread = std::thread([&]() { GlobalShutdownWatcherWorker(); });
			GlobalShutdownWaitableEvts[0] = GlobalShutdownEvt;
			GlobalShutdownWaitableEvts[1] = LocalShutdownEvt;

			return true;
		}

		void Deinit()
		{
			LocalShutdown();

			SAFE_CLOSE(TargetCounterReachedEvt);
			SAFE_CLOSE(LocalShutdownEvt);

			CounterStateMachineThread.join();
			GlobalShutdownWatcherThread.join();
		}

		void WaitTargetReached()
		{
			DWORD waitRes = WaitForMultipleObjectsEx(
				ARRAYSIZE(TargetCounterReachedWaitableEvts),
				TargetCounterReachedWaitableEvts,
				FALSE,
				INFINITE,
				FALSE);
			switch (waitRes)
			{
			case WAIT_FAILED:
				LogError("WaitForMultipleObjectsEx failed, error=%d", GetLastError());
				break;
			}
		}

		void SetTargetCounter(int targetCounter)
		{
			EnterCriticalSection(&CounterLock);
			Counter = 0;
			TargetCounter = targetCounter;
			LeaveCriticalSection(&CounterLock);
		}

		double GetOversamplingFrequency() const
		{
			LARGE_INTEGER now;

			(void)QueryPerformanceCounter(&now);

			double samplingElapsedTime = (double)(now.QuadPart - CounterStateMachineStartTime.QuadPart) / (double)HpcFreq.QuadPart;
			return (double)CounterStateMachineTickCount / samplingElapsedTime;
		}

		int GetCounter() const { return Counter; }

		int GetDirection() const { return Direction; }

		int GetMissedPulseCount() const { return MissedPulseCount; }

		int GetRpm() const
		{
			if (T0.QuadPart == T1.QuadPart)
				return 0;

			double revolutionPeriod = (double)(T1.QuadPart - T0.QuadPart) / (double)HpcFreq.QuadPart;
			return (int)(60.0 / revolutionPeriod);
		}

		// We implement a X4 mode only
		int GetXResolution() const { return 4; }

		void ResetCounter()
		{
			EnterCriticalSection(&CounterLock);
			Counter = 0;
			LeaveCriticalSection(&CounterLock);
		}

	private:

		void GlobalShutdownWatcherWorker()
		{
			DWORD waitRes = WaitForMultipleObjectsEx(
				ARRAYSIZE(GlobalShutdownWaitableEvts),
				GlobalShutdownWaitableEvts,
				FALSE,
				INFINITE,
				FALSE);
			switch (waitRes)
			{
			case WAIT_FAILED:
				LogError("WaitForMultipleObjectsEx failed, error=%d", GetLastError());
				break;
			}

			LocalShutdown();
		}

		void CounterStateMachineWorker()
		{
			LogFuncEnter();

			const int X4CounterStateMachine1Way[] = {
				0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0
			};

			ULONG chAMask = (1 << ChAPin);
			ULONG chBMask = (1 << ChBPin);

			(void)QueryPerformanceCounter(&T0);
			T1.QuadPart = T0.QuadPart;

			GpioBank0 = GpioBank0Read();

			ULONG newChAB = ((GpioBank0 & chAMask) ? 2 : 0) | ((GpioBank0 & chBMask) ? 1 : 0);
			EncoderReadings = newChAB;

			QueryPerformanceCounter(&CounterStateMachineStartTime);

			for (;!LocalShutdownFlag;)
			{
				++CounterStateMachineTickCount;

				// On overflow, reset tick count and sampling start time
				if (CounterStateMachineTickCount < 0)
				{
					CounterStateMachineTickCount = 1;
					(void)QueryPerformanceCounter(&CounterStateMachineStartTime);
					LogInfo("CounterStateMachineTickCount overflow!");
				}

				ULONG bank = GpioBank0Read();

				// No change in the GPIO pins state, nothing to do
				if (bank == GpioBank0)
					continue;

				GpioBank0 = bank;

				ULONG newChAB = ((bank & chAMask) ? 2 : 0) | ((bank & chBMask) ? 1 : 0);

				EncoderReadings <<= 2;
				EncoderReadings |= newChAB;

				LONG step = X4CounterStateMachine1Way[EncoderReadings & 0b1111];

				EnterCriticalSection(&CounterLock);

				Counter += step;

				if (step != 0)
					Direction = step;

				if (Counter == TargetCounter)
				{
					OnTargetCounterReachedCallback();
					TargetCounter = INT_MAX;
					(void)SetEvent(TargetCounterReachedEvt);
				}

				if (Counter == CountsPerRev)
				{
					Counter = 0;

					T0.QuadPart = T1.QuadPart;
					(void)QueryPerformanceCounter(&T1);
				}

				LeaveCriticalSection(&CounterLock);
			}

			LogFuncExit();
		}

		void LocalShutdown()
		{
			LocalShutdownFlag = true;
			(void)SetEvent(LocalShutdownEvt);
		}

		volatile int LocalShutdownFlag;
		volatile ULONG GpioBank0;
		volatile ULONG  lastChAB;
		volatile int Counter;
		volatile int MissedPulseCount;
		volatile int Direction;
		volatile int TargetCounter;
		volatile int CountsPerRev;
		volatile int EncoderReadings;
		int ChAPin;
		int ChBPin;

		HANDLE& LocalShutdownEvt;
		HANDLE& TargetCounterReachedEvt;
		HANDLE TargetCounterReachedWaitableEvts[2];

		HANDLE GlobalShutdownWaitableEvts[2];

		std::function<void()> OnTargetCounterReachedCallback;
		std::thread CounterStateMachineThread;
		std::thread GlobalShutdownWatcherThread;

		LARGE_INTEGER T0;
		LARGE_INTEGER T1;
		LARGE_INTEGER CounterStateMachineStartTime;
		CRITICAL_SECTION CounterLock;
		volatile LONGLONG CounterStateMachineTickCount;
	};
}