#include "pch.h"
#define DEBUG_TIMING
#include "Wi2Pi.h"
#include "NxtMotor.h"

using namespace Wi2Pi;

volatile bool Shutdown = false;

void OnHaflRev(SwQuadratureCounter* pQc)
{
	//LogInfo("Quadrature encoder compare reset @ %d", pQc->GetCounter());
}

void QuadratureReader(SwQuadratureCounter* pQc)
{
	LogInfo("Started quadrature encoder reader thread");

	for (;!Shutdown;)
	{
		LogInfo(
			"X%dCounter=%d, Dir=%d, Speed=%dstep/s, Missed=%dpulse(s)",
			pQc->GetXResolution(),
			pQc->GetCounter(),
			pQc->GetDirection(),
			(int)pQc->CounterFrequency(),
			pQc->GetMissedPulseCount());

		Sleep(1000);
	}
}

void MotorWorker()
{
	LogVerbose("->MotorWorker");

	NxtMotor motor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);

	if (!motor.Init())
	{
		LogInfo("Failed to init NXT motor");
		return;
	}

	motor.StopBrake();

	for (int i = 0; i < 5 && !Shutdown; ++i)
	{
		LogInfo("Going forward...");
		motor.Forward(100);
		Sleep(500);

		motor.StopBrake();
		Sleep(500);
	}

	for (int i = 0; i < 5 && !Shutdown; ++i)
	{
		LogInfo("Going forward...");
		motor.Forward(100);
		Sleep(500);

		motor.StopCoast();
		Sleep(500);
	}

	for (int i = 0; i < 5 && !Shutdown; ++i)
	{
		LogInfo("Going backward...");
		motor.Backward(100);
		Sleep(500);

		motor.StopBrake();
		Sleep(500);
	}

	for (int i = 0; i < 5 && !Shutdown; ++i)
	{
		LogInfo("Going backward...");
		motor.Backward(100);
		Sleep(500);

		motor.StopCoast();
		Sleep(500);
	}
}

int __cdecl wmain()
{
	if (!Wi2Pi::Init())
	{
		LogInfo("Failed to init WinWiringPi lib");
		return -1;
	}

	std::thread motorThread(MotorWorker);

	system("pause");
	Shutdown = true;
	motorThread.join();

	return 0;
}