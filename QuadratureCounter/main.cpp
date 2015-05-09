#include "pch.h"
//#define LOG_VERBOSE
#define DEBUG_TIMING
#include "Wi2Pi.h"
#include "NxtMotor.h"

using namespace Wi2Pi;
using namespace std;

void RobotArmControlTestWorker()
{
	NxtMotor baseMotor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);
	NxtMotor armMotor(BCM_GPIO16, BCM_GPIO5, BCM_GPIO6, BCM_GPIO12, BCM_GPIO13);

	LogFuncEnter();

	LogInfo("NXT Robot Arm Control Test");

	if (!baseMotor.Init() || !baseMotor.Init())
	{
		LogInfo("Failed to init NXT motor");
		goto Exit;
	}

	char cmd;

	for (;!GlobalShutdownFlag;)
	{
		cout << ">";
		cin >> cmd;

		switch (cmd)
		{
		case 'W':
		case 'w':
			armMotor.Forward(100);
			break;
		case 'S':
		case 's':
			armMotor.Backward(100);

		case 'A':
		case 'a':
			baseMotor.Backward(100);

		case 'D':
		case 'd':
			baseMotor.Backward(100);
			break;

		case 'Q':
		case 'q':
			baseMotor.StopBrake();
			break;

		case 'E':
		case 'e':
			armMotor.StopBrake();
			break;

		case 'X':
		case 'x':
			goto Exit;

		default:
			LogError("Unknown command %c", cmd);
		}
	}

Exit:

	baseMotor.StopCoast();
	armMotor.StopCoast();

	baseMotor.Deinit();
	armMotor.Deinit();

	LogFuncExit();
}

void MotorControlTestWorker()
{
	LogFuncEnter();

	LogInfo("NXT Motor Control Test");

	NxtMotor motor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);

	if (!motor.Init())
	{
		LogInfo("Failed to init NXT motor");
		return;
	}

	char cmd;

	for (;!GlobalShutdownFlag;)
	{
		cout << ">";
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

	LogInfo("NXT Motor Rpm Test");

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

	//
	// To find out motor RPM using Oscilloscope:
	//    RPM = (EncoderFreq * 2 / 720) * 60
	//
	// NXT motor is capable of 117RPM @ 9V, which is 117/60 = 1.95 rev/sec
	// 1 rev -> 720 encoder sample on X4 mode
	// 1.95 rev/sec = 1404 sample/sec = sampling at 1404 Hz
	// The oscilloscope encoder reading will be showing 1404/2 = 720Hz
	//
	double oversamplingFreq = motor.GetEncoder().GetOversamplingFrequency();

	LogInfo(
		"Oversampling Freq: %dHz~%dMHz, Sample Period: %fns", 
		(int)oversamplingFreq,
		(int)oversamplingFreq / 1000000,
		1000000000.0 / oversamplingFreq);

	motor.StopCoast();

	motor.Deinit();

	LogFuncExit();
}

void MotorDegreeTestWorker()
{
	LogFuncEnter();

	LogInfo("NXT Motor By Degree Control Test");

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
		motorThread.join();
		Wi2Pi::Deinit();

		break;
	case 'D':
	case 'd':
		motorThread = std::thread(MotorDegreeTestWorker);
		system("pause");
		Wi2Pi::Deinit();
		motorThread.join();
		break;

	case 'R':
	case 'r':
		motorThread = std::thread(MotorRpmTestWorker);
		system("pause");
		Wi2Pi::Deinit();
		motorThread.join();

	case 'A':
	case 'a':
		motorThread = std::thread(RobotArmControlTestWorker);
		motorThread.join();
		Wi2Pi::Deinit();
		break;

	default:
		break;
	}

	return 0;
}