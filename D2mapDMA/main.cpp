//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Simple program to test PWM on the Raspberry Pi.
//
#include "pch.h"
#define DEBUG_LOG
#include "SwServoPwm.h"
#include <iostream>
#include <time.h>
#include <thread>

using namespace std;
using namespace Wi2Pi;

SwServoPwm pwm;

void Calibrate()
{
	int pin;
	int param;

	for (;;)
	{
		cin >> pin;

		if (pin == -1)
			break;

		cin >> param;

		if (param > 180)
		{
			pwm.SetChannelWidth(GpioPinChannel[pin], param);
		}
		else
		{
			if (pin == 22)
				pwm.SetServoAngle(GpioPinChannel[pin], param, ServoMicroSG90);
			else if (pin == 23)
				pwm.SetServoAngle(GpioPinChannel[pin], param, ServoHS311);
		}
	}
}

volatile bool Shutdown = false;

void StressSwPwmWorker()
{
	LogInfo("Start SwPwm Stress");

	srand((unsigned)time(0));

	const int ServoTravelTimeMs = 1000;

	int pin;
	int param;

	for (;!Shutdown;)
	{
		pin = BCM_GPIO22;
		param = (rand() % 7) * 30;
		pwm.SetServoAngle(GpioPinChannel[pin], param, ServoMicroSG90);

		pin = BCM_GPIO23;
		param = (rand() % 5) * 45;
		pwm.SetServoAngle(GpioPinChannel[pin], param, ServoHS311);

		Sleep(ServoTravelTimeMs);
	}

	LogInfo("Exiting SwPwm Stress");
}

void StressSwPwm()
{
	thread stressThread(StressSwPwmWorker);

	system("pause");
	Shutdown = true;

	stressThread.join();
}

int __cdecl wmain()
{
	if (!Fx::Inst().Init())
	{
		LogError("Failed to init Wi2Pi lib");
		return false;
	}

	if (!pwm.Init())
	{
		LogError("Failed to init SwPwm");
		return -1;
	}

	LogInfo("\n## PWM via DMA and GPIO ##\n");

	//Calibrate();
	StressSwPwm();

	LogInfo("Exiting...");

	return 0;
}