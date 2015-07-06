//
// Copyright 2015 Muhamad Lotfy
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#define DEBUG_LOG
#include "Fx.h"
#include "RPi2\SwServoPwm.h"
#include <iostream>
#include <time.h>
#include <thread>

using namespace WinWire::RPi2;
using namespace WinWire;
using namespace std;

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
        LogError("Failed to init WinWire lib");
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