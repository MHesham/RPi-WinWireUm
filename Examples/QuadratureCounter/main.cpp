/*
	Copyright 2015 Muhamad Lotfy

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

//#define LOG_VERBOSE
//#define DEBUG_TIMING
#include "RPi2\Fx.h"
#include "RPi2\NxtMotor.h"
#include "RPi2\SwServoPwm.h"
#include <iostream>

using namespace WinWire::RPi2;
using namespace std;

void RobotArmControlTestWorker()
{
	NxtMotor baseMotor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);
	NxtMotor armMotor(BCM_GPIO16, BCM_GPIO5, BCM_GPIO6, BCM_GPIO12, BCM_GPIO13);

	LogFuncEnter();

	LogInfo("NXT Robot Arm Control Test");

	if (!baseMotor.Init() || !armMotor.Init())
	{
		LogInfo("Failed to init NXT motor");
		goto Exit;
	}

	char cmd;

	for (;!Fx::Inst().Inst().GlobalShutdownFlag;)
	{
		cout << ">";
		cin >> cmd;

		switch (cmd)
		{
		case 'W':
		case 'w':
			armMotor.Backward(100);
			break;

		case 'S':
		case 's':
			armMotor.Forward(100);
			break;

		case 'A':
		case 'a':
			baseMotor.Forward(100);
			break;

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

	for (;!Fx::Inst().Inst().GlobalShutdownFlag;)
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

	for (int i = 0; i < 20 && !Fx::Inst().GlobalShutdownFlag; ++i)
	{
		LogInfo("Forward RPM: %f", motor.GetDecoder().GetRpm());
		Sleep(1000);
	}

	motor.Backward(100);

	for (int i = 0; i < 20 && !Fx::Inst().GlobalShutdownFlag; ++i)
	{
		LogInfo("Backward RPM: %f", motor.GetDecoder().GetRpm());
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
	double oversamplingFreq = motor.GetDecoder().GetOversamplingFrequency();

	LogInfo(
		"Oversampling Freq: %dHz~%dMHz, Sample Period: %fns",
		(int)oversamplingFreq,
		(int)oversamplingFreq / 1000000,
		1000000000.0 / oversamplingFreq);

	motor.Deinit();

	LogFuncExit();
}

void MultiMotorPowerControlTest()
{
	NxtMotor baseMotor(BCM_GPIO17, BCM_GPIO24, BCM_GPIO25, BCM_GPIO22, BCM_GPIO23);
	NxtMotor armMotor(BCM_GPIO16, BCM_GPIO5, BCM_GPIO6, BCM_GPIO12, BCM_GPIO13);

	LogFuncEnter();

	LogInfo("NXT Robot Arm Control Test");

	if (!baseMotor.Init() || !armMotor.Init())
	{
		LogInfo("Failed to init NXT motor");
		goto Exit;
	}

	for (int i = 0; !Fx::Inst().Inst().GlobalShutdownFlag && i <= 100; i+=10)
	{
		LogInfo("Base Forward @%d Arm Backward @%d", i, i);

		baseMotor.Forward(i);
		armMotor.Forward(i);

		Sleep(2000);
	}

	for (int i = 0; !Fx::Inst().Inst().GlobalShutdownFlag && i <= 100; i+=10)
	{
		LogInfo("Base Backward @%d Arm Backward @%d", 100 - i, 100 - i);

		baseMotor.Backward(100 - i);
		armMotor.Backward(100 - i);

		Sleep(2000);
	}

Exit:

	baseMotor.Deinit();
	armMotor.Deinit();

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

	LogInfo("Going forward in 1' increments");

	for (int i = 1; !Fx::Inst().GlobalShutdownFlag && i <= 360; ++i)
	{
		LogVerbose("Itr%d Turn forward 1'", i);

		motor.ForwardInDegrees(1, 100);
		motor.WaitTargetReached();
		motor.StopBrake();
	}

	//LogInfo("Going backward in 1' increments");

	//for (int i = 1; !Fx::Inst().GlobalShutdownFlag && i <= 360; ++i)
	//{
	//	LogVerbose("Itr%d Turn backward 1'", i);

	//	motor.BackwardInDegrees(-1, 100);
	//	motor.WaitTargetReached();
	//	motor.StopBrake();
	//}

	motor.Deinit();

	LogFuncExit();
}

int __cdecl wmain()
{
	if (!Fx::Inst().Init())
	{
		LogInfo("Failed to init WinWiringPi lib");
		return -1;
	}

	int pwmPins[] = { BCM_GPIO16, BCM_GPIO17 };
	if (!SwPwm::Inst().Init(pwmPins, ARRAYSIZE(pwmPins), NxtMotor::PwmFrequencyHz))
	{
		LogError("Failed to init software PWM");
		return -1;
	}

	cout << "\nNXT Motor Test" << endl;
	cout << "  C|c: Command control Test" << endl;
	cout << "  D|d: Rotation in degrees test" << endl;
	cout << "  R|r: RPM measurement test" << endl;
	cout << "  A|a: NXT robot arm control test" << endl;
	cout << "  P|p: Multi motor power control test" << endl;

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
		Fx::Inst().Deinit();
		break;

	case 'A':
	case 'a':
		motorThread = std::thread(RobotArmControlTestWorker);
		motorThread.join();
		Fx::Inst().Deinit();
		break;

	case 'D':
	case 'd':
		motorThread = std::thread(MotorDegreeTestWorker);
		system("pause");
		Fx::Inst().Deinit();
		motorThread.join();
		break;

	case 'R':
	case 'r':
		motorThread = std::thread(MotorRpmTestWorker);
		system("pause");
		Fx::Inst().Deinit();
		motorThread.join();
		break;

	case 'P':
	case 'p':
		motorThread = std::thread(MultiMotorPowerControlTest);
		system("pause");
		Fx::Inst().Deinit();
		motorThread.join();
		break;

	default:
		LogInfo("Unknown command %c", cmd);
		break;
	}

	return 0;
}