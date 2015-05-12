#pragma once

#include "Wi2Pi.h"
#include <thread>
#include <functional>

namespace Wi2Pi
{
	class QuadratureDecoder
	{
	public:
		static const LONG CounterLockSpinCount = 5000;

		enum Mode
		{
			X4
		};

		QuadratureDecoder(int chAPin, int chBPin, int x4CountsPerRev, std::function<void()> onTargetCounterReachedCallback) :
			LocalShutdownFlag(false),
			GpioBank0(0),
			lastChAB(0),
			CounterTicks(0),
			TargetCounter(INT_MAX),
			LocalShutdownEvt(TargetCounterReachedWaitableEvts[0]),
			TargetCounterReachedEvt(TargetCounterReachedWaitableEvts[1]),
			Direction(0),
			ChAPin(chAPin),
			ChBPin(chBPin),
			CounterTicksPerRev(x4CountsPerRev),
			OnTargetCounterReachedCallback(onTargetCounterReachedCallback),
			EncoderReadings(0)
		{
			CountT0.QuadPart = 0;

			CounterStateMachineTickCount = 0;
			CounterStateMachineT0.QuadPart = 0;

			ZeroMemory(TargetCounterReachedWaitableEvts, sizeof(TargetCounterReachedWaitableEvts));
		}

		~QuadratureDecoder()
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

		double GetOversamplingFrequency() const
		{
			LARGE_INTEGER t1;

			(void)QueryPerformanceCounter(&t1);

			double samplingElapsedTime = (double)(t1.QuadPart - CounterStateMachineT0.QuadPart) / (double)HpcFreq.QuadPart;
			return (double)CounterStateMachineTickCount / samplingElapsedTime;
		}

		int GetDirection() const { return Direction; }

		double GetRpm() const
		{
			LARGE_INTEGER t1;
			(void)QueryPerformanceCounter(&t1);

			// RPM formula source: http://www.ni.com/tutorial/3921/en/ with the exception that we don't
			// use fixed time interval
			double elapsedTime = (double)(t1.QuadPart - CountT0.QuadPart) / (double)HpcFreq.QuadPart;
			return (((double)abs(CounterTicks) / (double)CounterTicksPerRev) * 60.0) / elapsedTime;
		}

		// We implement a X4 mode only
		Mode GetMode() const { return X4; }

		void ResetCounter()
		{
			EnterCriticalSection(&CounterLock);
			CounterTicks = 0;
			(void)QueryPerformanceCounter(&CountT0);
			LeaveCriticalSection(&CounterLock);
		}

		void ResetCounterAndSetTargetAngle(int targetAngle)
		{
			if (targetAngle == 0)
			{
				LogError("Invalid target counter value %d", TargetCounter);
				return;
			}

			EnterCriticalSection(&CounterLock);
			CounterTicks = 0;
			TargetCounter = targetAngle * 2;
			(void)QueryPerformanceCounter(&CountT0);
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

			(void)QueryPerformanceCounter(&CountT0);

			GpioBank0 = GpioBank0Read();

			ULONG newChAB = ((GpioBank0 & chAMask) ? 2 : 0) | ((GpioBank0 & chBMask) ? 1 : 0);
			EncoderReadings = newChAB;

			QueryPerformanceCounter(&CounterStateMachineT0);

			for (;!LocalShutdownFlag;)
			{
				++CounterStateMachineTickCount;

				// On overflow, reset tick count and sampling start time
				if (CounterStateMachineTickCount < 0)
				{
					CounterStateMachineTickCount = 1;
					(void)QueryPerformanceCounter(&CounterStateMachineT0);
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

				CounterTicks += step;

				if (step != 0)
					Direction = step;

				if (TargetCounter != 0 &&
					CounterTicks == TargetCounter)
				{
					OnTargetCounterReachedCallback();
					TargetCounter = 0;
					(void)SetEvent(TargetCounterReachedEvt);
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
		volatile int CounterTicks;
		volatile int CounterTicksPerRev;
		volatile int Direction;
		volatile int TargetCounter;
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

		LARGE_INTEGER CountT0;
		LARGE_INTEGER CounterStateMachineT0;
		CRITICAL_SECTION CounterLock;
		volatile LONGLONG CounterStateMachineTickCount;
	};
}