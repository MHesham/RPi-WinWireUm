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

#pragma once

#include "QuadratureDecoder.h"
#include "HBridge.h"

namespace WinWire {
    template<class TGpioProvider, class TPwmProvider>
    class NxtMotor
    {
    public:

        typedef QuadratureDecoder<TGpioProvider> Decoder;

        static const int PwmFrequencyHz = (int)1000;

        NxtMotor(int enablePin, int hbridgeIN1Pin, int hbridgeIN2Pin, int enoderChAPin, int encoderChBPin) :
            EnablePin(enablePin),
            MotorDriver(hbridgeIN1Pin, hbridgeIN2Pin),
            MotorDecoder(enoderChAPin, encoderChBPin, 720, std::bind(&NxtMotor::OnDecoderTargetReached, this))
        {
        }

        bool Init()
        {
            TGpioProvider::GpioPinSetDir(EnablePin, TGpioProvider::DIR_Output);

            if (!MotorDriver.Init())
            {
                LogError("Failed to init motor driver");
                return false;
            }

            if (!MotorDecoder.Init())
            {
                LogError("Failed to init NXT motor quadrature decoder");
                return false;
            }

            StopBrake();

            return true;
        }

        void Deinit()
        {
            StopBrake();

            MotorDecoder.Deinit();
        }

        const Decoder& GetDecoder() const { return MotorDecoder; }

        void Forward(int powerPerct)
        {
            SetPower(0);
            MotorDecoder.ResetCounter();
            MotorDriver.Forward();
            SetPower(powerPerct);
        }

        void ForwardInDegrees(int angle, int powerPerct)
        {
            SetPower(0);
            MotorDecoder.ResetCounterAndSetTargetAngle(angle);
            MotorDriver.Forward();
            SetPower(powerPerct);
        }

        void Backward(int powerPerct)
        {
            SetPower(0);
            MotorDecoder.ResetCounter();
            MotorDriver.Backward();
            SetPower(powerPerct);
        }

        void BackwardInDegrees(int angle, int powerPerct)
        {
            SetPower(0);
            MotorDecoder.ResetCounterAndSetTargetAngle(-angle);
            MotorDriver.Backward();
            SetPower(powerPerct);
        }

        void StopCoast()
        {
            SetPower(0);
            MotorDecoder.ResetCounter();
        }

        void StopBrake()
        {
            SetPower(0);
            MotorDriver.Stop();
            MotorDecoder.ResetCounter();
            SetPower(100);
        }

        void WaitTargetReached()
        {
            MotorDecoder.WaitTargetReached();
        }

    private:

        void OnDecoderTargetReached()
        {
            StopBrake();
        }

        void SetPower(int powerPerct)
        {
            TPwmProvider::Inst().SetPinDutyCycle(EnablePin, (double)powerPerct / 100.0);
        }

        int EnablePin;
        HBridge<TGpioProvider> MotorDriver;
        Decoder MotorDecoder;
    };
}
