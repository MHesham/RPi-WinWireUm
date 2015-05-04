#include "pch.h"
#define DEBUG_TIMING
#include "Wi2Pi.h"
#include "NxtMotor.h"

using namespace Wi2Pi;

void MotorWorker()
{
	LogVerbose("->MotorWorker");

	NxtMotor motor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);

	if (!motor.Init())
	{
		LogInfo("Failed to init NXT motor");
		return;
	}

	for (int i = 0; !GlobalShutdownFlag; ++i)
	{
		if (i & 1)
			motor.ForwardInDegrees(180, 100);
		else
			motor.BackwardInDegrees(90, 100);
	}

	motor.StopCoast();

	motor.Deinit();

	LogVerbose("<-MotorWorker");
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
	
	Wi2Pi::Deinit();

	motorThread.join();

	return 0;
}