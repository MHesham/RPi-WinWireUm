#pragma once

namespace Wi2Pi
{
	class HpcTimer
	{
	public:
		HpcTimer()
		{
			T0.QuadPart = 0LL;
			T1.QuadPart = 0LL;

			(void)QueryPerformanceFrequency(&TimerFreq);
		}

		void Start()
		{
			(void)QueryPerformanceCounter(&T0);
		}

		void Stop()
		{
			(void)QueryPerformanceCounter(&T1);
		}

		double OperationsPerSecond(int numOps) const
		{
			return (double)numOps / ElapsedSeconds();
		}

		double ElapsedSecondsNow() const
		{
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);

			return (double)(now.QuadPart - T0.QuadPart) /
				(double)TimerFreq.QuadPart;
		}

		double ElapsedSeconds() const
		{
			return (double)(T1.QuadPart - T0.QuadPart) /
				(double)TimerFreq.QuadPart;
		}

	private:
		LARGE_INTEGER TimerFreq;
		LARGE_INTEGER T0;
		LARGE_INTEGER T1;
	};
}