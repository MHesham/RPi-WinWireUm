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

		SwQuadratureCounter(int chAPin, int chBPin, int countsPerRev, std::function<void()> onTargetCounterReachedCallback) :
			ShutdownWaitFlag(false),
			GpioBank0(0),
			lastChAB(0),
			Counter(0),
			MissedPulseCount(0),
			TargetCounter(INT_MAX),
			TargetCounterReachedWaitFlag(false),
			Direction(0),
			ChAPin(chAPin),
			ChBPin(chBPin),
			CountsPerRev(countsPerRev),
			OnTargetCounterReachedCallback(onTargetCounterReachedCallback),
			EncoderReadings(0)
		{
			T0.QuadPart = 0;
			T1.QuadPart = 0;

			InitializeCriticalSectionAndSpinCount(&CounterLock, 5000);
		}

		~SwQuadratureCounter()
		{
			DeleteCriticalSection(&CounterLock);
		}

		bool Init()
		{
			GpioFuncSelect(ChAPin, BCM_GPIO_FSEL_Input);
			GpioFuncSelect(ChBPin, BCM_GPIO_FSEL_Input);

			CounterStateMachineThread = std::thread([&]() { CounterStateMachineWorker(); });
			(void)::SetThreadPriority(CounterStateMachineThread.native_handle(), THREAD_PRIORITY_HIGHEST);

			GlobalShutdownWatcherThread = std::thread([&]() { GlobalShutdownWatcherWorker(); });

			return true;
		}

		void Deinit()
		{
			ShutdownWaitFlag = true;
			CounterStateMachineThread.join();
		}

		void WaitTargetReached()
		{
			while (!TargetCounterReachedWaitFlag && !ShutdownWaitFlag);

			TargetCounterReachedWaitFlag = false;
		}

		void SetTargetCounter(int targetCounter)
		{
			EnterCriticalSection(&CounterLock);
			Counter = 0;
			TargetCounter = targetCounter;
			LeaveCriticalSection(&CounterLock);
		}

		int GetCounter() const { return Counter; }

		int GetDirection() const { return Direction; }

		int GetMissedPulseCount() const { return MissedPulseCount; }

		int GetRpm() const
		{
			if (T0.QuadPart == T1.QuadPart)
				return 0;

			double revolutionPeriod = (double)(T1.QuadPart - T0.QuadPart) / (double)HpcFreq.QuadPart;
			double revolutionFreq = 1 / revolutionPeriod;
			return (int)(revolutionFreq * 60.0);
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
			(void)WaitForSingleObject(GlobalShutdownEvt, INFINITE);
			ShutdownWaitFlag = true;
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

			for (;!ShutdownWaitFlag;)
			{
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
					TargetCounterReachedWaitFlag = true;
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

		volatile int ShutdownWaitFlag;
		volatile ULONG GpioBank0;
		volatile ULONG  lastChAB;
		volatile int Counter;
		volatile int MissedPulseCount;
		volatile int Direction;
		volatile int TargetCounter;
		volatile int CountsPerRev;
		volatile int EncoderReadings;
		volatile int TargetCounterReachedWaitFlag;
		int ChAPin;
		int ChBPin;
		std::function<void()> OnTargetCounterReachedCallback;
		std::thread CounterStateMachineThread;
		std::thread GlobalShutdownWatcherThread;
		LARGE_INTEGER T0;
		LARGE_INTEGER T1;
		CRITICAL_SECTION CounterLock;
	};
}