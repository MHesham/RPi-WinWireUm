#pragma once

namespace Wi2Pi
{
	enum TIMESPAN_TICKS : INT64
	{
		TIMESPAN_TICKS_PER_MILLISECOND = 10LL * 1000LL,
		TIMESPAN_TICKS_PER_SECOND = TIMESPAN_TICKS_PER_MILLISECOND * 1000LL,
		TIMESPAN_TICKS_PER_MINUTE = TIMESPAN_TICKS_PER_SECOND * 60LL,
		TIMESPAN_TICKS_PER_HOUR = TIMESPAN_TICKS_PER_MINUTE * 60LL,
		TIMESPAN_TICKS_PER_DAY = TIMESPAN_TICKS_PER_HOUR * 24LL
	};

	class PerfTimer
	{
	public:
		PerfTimer()
		{
			QueryPerformanceFrequency(&this->ticksPerSecond);
			start.QuadPart = 0LL;
			stop.QuadPart = 0LL;
		}

		void Start()
		{
			QueryPerformanceCounter(&this->start);
		}

		void Stop()
		{
			QueryPerformanceCounter(&this->stop);
		}

		double CalcOpsPerSecond(LONG numOps) const
		{
			// numOps / sec = (Ticks/s) * numOps / Ticks
			return (double)(this->ticksPerSecond.QuadPart * numOps) /
				(stop.QuadPart - start.QuadPart);
		}

		double CalcOpsPerSecondNow(LONG numOps) const
		{
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);

			// numOps / sec = (Ticks/s) * numOps / Ticks
			return (double)(this->ticksPerSecond.QuadPart * numOps) /
				(now.QuadPart - start.QuadPart);
		}

		double CalcSecondsNow() const
		{
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);

			return (double)(now.QuadPart - start.QuadPart) /
				this->ticksPerSecond.QuadPart;
		}

		double CalcSeconds() const
		{
			return (double)(stop.QuadPart - start.QuadPart) /
				this->ticksPerSecond.QuadPart;
		}

		// Calculate duration in 100ns intervals
		INT64 CalcDuration() const
		{
			if (ticksPerSecond.QuadPart == TIMESPAN_TICKS_PER_SECOND)
				return stop.QuadPart - start.QuadPart;
			else
				return TIMESPAN_TICKS_PER_SECOND * (stop.QuadPart - start.QuadPart) /
				ticksPerSecond.QuadPart;
		}

	private:
		LARGE_INTEGER ticksPerSecond;
		LARGE_INTEGER start;
		LARGE_INTEGER stop;
	};
}