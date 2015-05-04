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

		SwQuadratureCounter(int chAPin, int chBPin, int stepsPerRev, std::function<void()> onTargetCounterReachedCallback) :
			ShutdownWaitFlag(false),
			GpioBank0(0),
			lastChAB(0),
			Counter(0),
			MissedPulseCount(0),
			TargetCounter(0),
			ChAPin(chAPin),
			ChBPin(chBPin),
			StepsPerRev(stepsPerRev),
			OnTargetCounterReachedCallback(onTargetCounterReachedCallback),
			EncoderReadings(0)
		{}

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
			Counter = 0;
			TargetCounter = targetCounter;
		}

		int GetCounter() const { return Counter; }

		int GetDirection() const { return Direction; }

		int GetMissedPulseCount() const { return MissedPulseCount; }

		int CounterFrequency() const { return (int)Timer.CalcOpsPerSecondNow(abs(Counter)); }

		// We implement a X4 mode only
		int GetXResolution() const { return 4; }

	private:

		void GlobalShutdownWatcherWorker()
		{
			(void)WaitForSingleObject(GlobalShutdownEvt, INFINITE);
			ShutdownWaitFlag = true;
		}

		void CounterStateMachineWorker()
		{
			const int X4CounterStateMachine1Way[] = {
				0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0
			};

			ULONG chAMask = (1 << ChAPin);
			ULONG chBMask = (1 << ChBPin);

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

				Counter += step;

				if (step != 0)
					Direction = step;

				if (Counter == TargetCounter)
				{
					OnTargetCounterReachedCallback();
					TargetCounter = 0;
					TargetCounterReachedWaitFlag = true;
				}

				if (Counter == StepsPerRev)
				{
					Counter = 0;
				}
			}
		}

		volatile int ShutdownWaitFlag;
		volatile ULONG GpioBank0;
		volatile ULONG  lastChAB;
		volatile int Counter;
		volatile int MissedPulseCount;
		volatile int Direction;
		volatile int TargetCounter;
		volatile int StepsPerRev;
		volatile int EncoderReadings;
		volatile int TargetCounterReachedWaitFlag;
		int ChAPin;
		int ChBPin;
		std::function<void()> OnTargetCounterReachedCallback;
		std::thread CounterStateMachineThread;
		std::thread GlobalShutdownWatcherThread;
		PerfTimer Timer;
	};
}