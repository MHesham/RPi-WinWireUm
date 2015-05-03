#pragma once

#include "Wi2Pi.h"
#include <thread>

namespace Wi2Pi
{
	class SwQuadratureCounter;

	typedef void(*StepCounterCompareCallback)(SwQuadratureCounter* pQc);

	class SwQuadratureCounter
	{
	public:
		static const int NA = INT_MAX;

		SwQuadratureCounter(int chAPin, int chBPin) :
			Shutdown(false),
			GpioBank0(0),
			lastChAB(0),
			Counter(0),
			MissedPulseCount(0),
			CounterCallback(nullptr),
			CounterCompare(-1),
			ChAPin(chAPin),
			ChBPin(chBPin)
		{}

		~SwQuadratureCounter()
		{
			Shutdown = true;
			CounterStateMachineWorkerThread.join();
		}

		bool Init()
		{
			GpioFuncSelect(ChAPin, BCM_GPIO_FSEL_Input);
			GpioFuncSelect(ChBPin, BCM_GPIO_FSEL_Input);

			CounterStateMachineWorkerThread = std::thread([&]() { CounterStateMachineWorker(); });
			(void)::SetThreadPriority(CounterStateMachineWorkerThread.native_handle(), THREAD_PRIORITY_HIGHEST);

			return true;
		}

		void SetCounterCompare(int counterCompare, StepCounterCompareCallback counterCallback)
		{
			CounterCompare = counterCompare;
			CounterCallback = counterCallback;
		}

		int GetCounter() const { return Counter; }

		int GetDirection() const { return Direction; }

		int GetMissedPulseCount() const { return MissedPulseCount; }

		int CounterFrequency() const { return (int)Timer.CalcOpsPerSecondNow(abs(Counter)); }

		// We implement a X4 mode only
		int GetXResolution() const { return 4; }

	private:

		void CounterStateMachineWorker()
		{
			const int X4CounterStateMachine[][4] = {
				{ 0, -1,  1,  NA },
				{ 1,  0,  NA, -1 },
				{ -1, NA, 0,  1 },
				{ NA, 1, -1,  0 },
			};

			ULONG chAMask = (1 << ChAPin);
			ULONG chBMask = (1 << ChBPin);

			Timer.Start();

			for (;!Shutdown;)
			{
				ULONG bank = GpioBank0Read();

				// No change in the GPIO pins state, nothing to do
				if (bank == GpioBank0)
					continue;

				GpioBank0 = bank;

				ULONG newChAB = ((bank & chAMask) ? 2 : 0) | ((bank & chBMask) ? 1 : 0);

				// Some other pin state changed but ChA and ChB state didn't
				if (newChAB == lastChAB)
					continue;

				int step = X4CounterStateMachine[lastChAB][newChAB];

				if (step == NA)
				{
					Counter = 0;
					++MissedPulseCount;

					lastChAB = newChAB;
					Direction = 0;
				}
				else
				{
					Counter += step;

					lastChAB = newChAB;
					Direction = step;

					if (CounterCallback &&
						Counter == CounterCompare)
					{
						CounterCallback(this);

						Counter = 0;
						MissedPulseCount = 0;
						Direction = 0;
						Timer.Start();
					}
				}
			}
		}

		volatile int Shutdown;
		volatile ULONG GpioBank0;
		volatile ULONG  lastChAB;
		volatile int Counter;
		volatile int MissedPulseCount;
		volatile int Direction;
		volatile int CounterCompare;
		int ChAPin;
		int ChBPin;
		volatile StepCounterCompareCallback CounterCallback;
		std::thread CounterStateMachineWorkerThread;
		PerfTimer Timer;
	};
}