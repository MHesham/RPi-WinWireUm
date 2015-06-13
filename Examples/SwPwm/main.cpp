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
#include "RPi2\SwPwm.h"
#include <iostream>
#include <thread>

using namespace WinWire::RPi2;
using namespace std;

void PwmTestWorker()
{
	LogFuncEnter();

	for (int i = 100; i >= 0 && !Fx::Inst().GlobalShutdownFlag; i -= 5)
	{
		double dutyCycle = (double)i / 100.0;
		LogInfo("Setting Duty Cycle=%f", dutyCycle);

		SwPwm::Inst().SetChannelDutyCycle(0, dutyCycle);
		SwPwm::Inst().SetChannelDutyCycle(1, 1.0 - dutyCycle);
		Sleep(2000);
	}

	LogFuncExit();
}

int __cdecl wmain()
{
	if (!Fx::Inst().Init())
	{
		LogInfo("Failed to init WinWiringPi lib");
		return -1;
	}

	LogInfo("Initializing software PWM");

	double pwmFreqHz = 0.0;

	cout << "PWM Freq (Hz): ";
	cin >> pwmFreqHz;

	int pwmPins[] = { BCM_GPIO16, BCM_GPIO17 };
	if (!SwPwm::Inst().Init(pwmPins, ARRAYSIZE(pwmPins), pwmFreqHz))
	{
		LogError("Failed to init software PWM");
		return -1;
	}

	thread pwmTestThread(PwmTestWorker);
	system("pause");
	Fx::Inst().Deinit();
	pwmTestThread.join();

	Fx::Inst().Deinit();

	return 0;
}