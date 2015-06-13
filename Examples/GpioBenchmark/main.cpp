#define DEBUG_HPC
#include "RPi2\Fx.h"

using namespace WinWire::RPi2;

#define BENCHMARK_GPIO_PIN 22
#define BENCHMARK_ITERATIONS 1000000

int __cdecl wmain()
{
	if (!Fx::Inst().Init())
	{
		LogInfo("Failed to init WinWiringPi lib");
		return -1;
	}

	BenchmarkGpio(BENCHMARK_GPIO_PIN, BENCHMARK_ITERATIONS);

	return 0;
}