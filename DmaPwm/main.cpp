#include "pch.h"
//#define LOG_VERBOSE
//#define DEBUG_TIMING
#include "Wi2Pi.h"
#include "SwPwm.h"

using namespace Wi2Pi;
using namespace std;

void PwmTestWorker()
{
	LogFuncEnter();

	for (int i = 100; i >= 0 && !Fx::Inst().GlobalShutdownFlag; i-=5)
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
	/*cout << "sizeof(ULONG)=" << sizeof(ULONG) << endl;
	cout << "sizeof(BCM_DMA_CB)=" << sizeof(BCM_DMA_CB) << endl;
	cout << "sizeof(SwServoPwmControlData::CB)=" << sizeof(SwServoPwmControlData::CB) << endl;
	cout << "sizeof(SwServoPwmControlData::Step)=" << sizeof(SwServoPwmControlData::Step) << endl;
	cout << "sizeof(SwServoPwmControlData)=" << sizeof(SwServoPwmControlData) << endl;

	return 0;*/

	if (!Fx::Inst().Init())
	{
		LogInfo("Failed to init WinWiringPi lib");
		return -1;
	}


	LogInfo("Initializing software PWM");

	int pwmPins[] = { BCM_GPIO16, BCM_GPIO17 };
	if (!SwPwm::Inst().Init(pwmPins, ARRAYSIZE(pwmPins), PWM_DEFAULT_FREQ_HZ))
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