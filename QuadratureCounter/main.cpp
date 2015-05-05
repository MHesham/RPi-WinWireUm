#include "pch.h"
//#define LOG_VERBOSE
#define DEBUG_TIMING
#include "Wi2Pi.h"
#include "NxtMotor.h"

using namespace Wi2Pi;
using namespace std;

void MotorControlTestWorker()
{
	LogFuncEnter();

	NxtMotor motor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);

	if (!motor.Init())
	{
		LogInfo("Failed to init NXT motor");
		return;
	}

	char cmd;

	for (;!GlobalShutdownFlag;)
	{
		cin >> cmd;

		switch (cmd)
		{
		case 'W':
		case 'w':
			motor.Forward(100);
			break;
		case 'S':
		case 's':
			motor.Backward(100);
			break;
		case 'B':
		case 'b':
			motor.StopBrake();
			break;
		case 'X':
		case 'x':
			goto Exit;
		default:
			LogError("Unknown command %c", cmd);
		}
	}

Exit:

	motor.StopCoast();

	motor.Deinit();

	LogFuncExit();
}

void MotorRpmTestWorker()
{
	LogFuncEnter();

	NxtMotor motor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);

	if (!motor.Init())
	{
		LogInfo("Failed to init NXT motor");
		return;
	}

	motor.Forward(100);

	for (int i = 0; i < 5 && !GlobalShutdownFlag; ++i)
	{
		LogInfo("Forward RPM: %d", motor.GetEncoder().GetRpm());
		Sleep(1000);
	}

	motor.Backward(100);

	for (int i = 0; i < 5 && !GlobalShutdownFlag; ++i)
	{
		LogInfo("Backward RPM: %d", motor.GetEncoder().GetRpm());
		Sleep(1000);
	}

	LogInfo("Oversampling Freq: %dHz", motor.GetEncoder().GetOversamplingFrequency());

	motor.StopCoast();

	motor.Deinit();

	LogFuncExit();
}

void MotorDegreeTestWorker()
{
	LogFuncEnter();

	NxtMotor motor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);

	if (!motor.Init())
	{
		LogInfo("Failed to init NXT motor");
		return;
	}

	for (int i = 0; !GlobalShutdownFlag; ++i)
	{
		if (i & 1)
			motor.ForwardInDegrees(45, 100);
		else
			motor.BackwardInDegrees(90, 100);
	}

	motor.StopCoast();

	motor.Deinit();

	LogFuncExit();
}

int __cdecl wmain()
{
	if (!Wi2Pi::Init())
	{
		LogInfo("Failed to init WinWiringPi lib");
		return -1;
	}

	cout << "\nNXT Motor Test" << endl;
	cout << "  C|c: Command control Test" << endl;
	cout << "  D|d: Rotation in degrees test" << endl;
	cout << "  R|r: RPM measurement test" << endl;

	char cmd;
	cout << ">";
	cin >> cmd;

	std::thread motorThread;

	switch (cmd)
	{
	case 'C':
	case 'c':
		motorThread = std::thread(MotorControlTestWorker);
		break;
	case 'D':
	case 'd':
		motorThread = std::thread(MotorDegreeTestWorker);
		system("pause");
		break;
	case 'R':
	case 'r':
		motorThread = std::thread(MotorRpmTestWorker);
		system("pause");
		break;
	default:
		break;
	}

	Wi2Pi::Deinit();

	if (motorThread.joinable())
		motorThread.join();

	return 0;
}