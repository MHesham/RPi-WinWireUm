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
			GpioPinWrite(HBridgeIN1Pin, true);
			GpioPinWrite(HBridgeIN2Pin, true);
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
			Encoder(enoderChAPin, encoderChBPin)
		{}

		bool Init()
		{
			GpioFuncSelect(EnablePin, BCM_GPIO_FSEL_Output);

			if (!MotorDriver.Init())
			{
				LogError("Failed to init motor HBridge");
				return false;
			}

			if (!Encoder.Init())
			{
				LogError("Failed to init NXT motor quadrature encoder");
				return false;
			}

			return true;
		}

		void Forward(int powerPerct)
		{
			if (powerPerct > 0)
				GpioPinWrite(EnablePin, true);
			else
				GpioPinWrite(EnablePin, false);

			MotorDriver.Forward();
		}

		void Backward(int powerPerct)
		{
			if (powerPerct > 0)
				GpioPinWrite(EnablePin, true);
			else
				GpioPinWrite(EnablePin, false);

			MotorDriver.Backward();
		}

		void StopCoast()
		{
			GpioPinWrite(EnablePin, false);
		}

		void StopBrake()
		{
			GpioPinWrite(EnablePin, true);
			MotorDriver.Stop();
		}

	private:
		int EnablePin;
		HBridge MotorDriver;
		SwQuadratureCounter Encoder;
	};
}