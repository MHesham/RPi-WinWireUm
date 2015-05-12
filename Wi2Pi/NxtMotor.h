#pragma once

#include "SwQuadratureCounter.h"

namespace Wi2Pi
{
	class HBridge
	{
	public:
		HBridge(int hbridgeIN1Pin, int hbridgeIN2Pin) :
			HBridgeIN1Pin(hbridgeIN1Pin),
			HBridgeIN2Pin(hbridgeIN2Pin)
		{}

		bool Init()
		{
			GpioFuncSelect(HBridgeIN1Pin, BCM_GPIO_FSEL_Output);
			GpioFuncSelect(HBridgeIN2Pin, BCM_GPIO_FSEL_Output);

			return true;
		}

		void Forward()
		{
			GpioPinWrite(HBridgeIN1Pin, true);
			GpioPinWrite(HBridgeIN2Pin, false);
		}

		void Backward()
		{
			GpioPinWrite(HBridgeIN1Pin, false);
			GpioPinWrite(HBridgeIN2Pin, true);
		}

		void Stop()
		{
			GpioPinWrite(HBridgeIN1Pin, false);
			GpioPinWrite(HBridgeIN2Pin, false);
		}

	private:
		int HBridgeIN1Pin;
		int HBridgeIN2Pin;
	};

	class NxtMotor
	{
	public:
		NxtMotor(int enablePin, int hbridgeIN1Pin, int hbridgeIN2Pin, int enoderChAPin, int encoderChBPin) :
			EnablePin(enablePin),
			MotorDriver(hbridgeIN1Pin, hbridgeIN2Pin),
			MotorEncoder(enoderChAPin, encoderChBPin, 720, std::bind(&NxtMotor::OnEncoderTargetReached, this))
		{
		}

		bool Init()
		{
			GpioFuncSelect(EnablePin, BCM_GPIO_FSEL_Output);

			if (!MotorDriver.Init())
			{
				LogError("Failed to init motor driver");
				return false;
			}

			if (!MotorEncoder.Init())
			{
				LogError("Failed to init NXT motor quadrature encoder");
				return false;
			}

			StopBrake();

			return true;
		}

		void Deinit()
		{
			StopBrake();

			MotorEncoder.Deinit();
		}

		const SwQuadratureCounter& GetEncoder() const { return MotorEncoder; }

		void Forward(int powerPerct)
		{
			SetPower(0);
			MotorEncoder.ResetCounter();
			MotorDriver.Forward();
			SetPower(powerPerct);
		}

		void ForwardInDegrees(int angle, int powerPerct)
		{
			SetPower(0);
			MotorEncoder.ResetCounterAndSetTarget(AngleToEncoderSteps(angle));
			MotorDriver.Forward();
			SetPower(powerPerct);
		}

		void Backward(int powerPerct)
		{
			SetPower(0);
			MotorEncoder.ResetCounter();
			MotorDriver.Backward();
			SetPower(powerPerct);
		}

		void BackwardInDegrees(int angle, int powerPerct)
		{
			SetPower(0);
			MotorEncoder.ResetCounterAndSetTarget(AngleToEncoderSteps(-angle));
			MotorDriver.Backward();
			SetPower(powerPerct);
		}

		void StopCoast()
		{
			SetPower(0);
			MotorEncoder.ResetCounter();
		}

		void StopBrake()
		{
			SetPower(0);
			MotorDriver.Stop();
			MotorEncoder.ResetCounter();
			SetPower(100);
		}

		void WaitTargetReached()
		{
			MotorEncoder.WaitTargetReached();
		}

	private:

		void OnEncoderTargetReached()
		{
			StopBrake();
			LogVerbose("Encoder target reached");
		}

		void SetPower(int powerPerct)
		{
			if (powerPerct > 0)
				GpioPinWrite(EnablePin, true);
			else
				GpioPinWrite(EnablePin, false);
		}

		int AngleToEncoderSteps(int angle)
		{
			// Because the encoder is running X4 mode
			return angle * 2;
		}

		int EnablePin;
		HBridge MotorDriver;
		SwQuadratureCounter MotorEncoder;
	};
}