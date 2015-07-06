#include "pch.h"
#include "StartupTask.h"
#include "NxtMotor.h"
#include "Test.h"
#include "Fx.h"

using namespace KinematicRT;
using namespace WinWire;

using namespace Platform;
using namespace Windows::ApplicationModel::Background;
using namespace Windows::Devices::Gpio;
using namespace Windows::Devices::Enumeration;

class GpioRT
{
public:
    enum
    {
        DIR_Input = GpioPinDriveMode::Input,
        DIR_Output = GpioPinDriveMode::Output
    };

    enum
    {
        PULL_None = GpioPinDriveMode::Input,
        PULL_Down = GpioPinDriveMode::InputPullDown,
        PULL_Up = GpioPinDriveMode::InputPullUp
    };

    static GpioRT& Inst()
    {
        static GpioRT inst;
        return inst;
    }

    bool Init()
    {
        Dev = GpioController::GetDefault();
        GpioPin ^pin;
        GpioOpenStatus status;

        Pins.resize(Dev->PinCount);
        
        for (int i = 0; i < Dev->PinCount; ++i)
        {
            if (Dev->TryOpenPin(i, GpioSharingMode::Exclusive, &pin, &status))
            {
                LogInfo("Opened pin%d", i);
                Pins[i] = pin;
            }
        }

        return true;
    }

    void GpioPinSetDir(int pinNum, int dir)
    {
        Pins[pinNum]->SetDriveMode(GpioPinDriveMode(dir));
    }

    void GpioPinSetPull(int pinNum, int pull)
    {
        Pins[pinNum]->SetDriveMode(GpioPinDriveMode(pull));
    }

    void GpioPinWrite(int pinNum, bool state)
    {
        Pins[pinNum]->Write(state ? GpioPinValue::High : GpioPinValue::Low);
    }

    bool GpioPinRead(int pinNum)
    {
        return Pins[pinNum]->Read() == GpioPinValue::High ? true : false;
    }

    ULONG GpioBankRead(int bank)
    {
        return 0;
    }

    std::vector<GpioPin^> Pins;
    Windows::Devices::Gpio::GpioController ^Dev;
};

class PwmRT
{
public:
    static PwmRT& Inst()
    {
        static PwmRT inst;
        return inst;
    }

    bool Init()
    {
        return true;
    }

    void SetPinDutyCycle(int pin, double perct)
    {
    }
};

// The Background Application template is documented at http://go.microsoft.com/fwlink/?LinkID=533884&clcid=0x409

void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)
{
    if (!Fx::Inst().Init())
    {
        LogError("WinWire framework init failed");
        return;
    }

    if (!GpioRT::Inst().Init())
    {
        LogError("GpioRT init failed");
        return;
    }

    if (!PwmRT::Inst().Init())
    {
        LogError("PwmRT init failed");
        return;
    }

    BenchmarkGpio<GpioRT>(4, 1000);

   /* NxtMotor<GpioRT, PwmRT> motor(4, 5, 6, 7, 8);

    motor.Forward(50);
    motor.StopBrake();
    motor.Backward(50);
    motor.StopCoast();
    motor.ForwardInDegrees(30, 95);
    motor.WaitTargetReached();
    motor.BackwardInDegrees(30, 95);

    motor.Deinit();*/
}
