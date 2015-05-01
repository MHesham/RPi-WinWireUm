#include "pch.h"
#define DEBUG_HPC
#include "Wi2Pi.h"

using namespace Wi2Pi;

#define BENCHMARK_GPIO_PIN 22
#define BENCHMARK_ITERATIONS 1000000

int __cdecl wmain()
{
	if (!Wi2Pi::Init())
	{
		LogInfo("Failed to init WinWiringPi lib");
		return -1;
	}

	BenchmarkGpio(BENCHMARK_GPIO_PIN, BENCHMARK_ITERATIONS);

	return 0;
}