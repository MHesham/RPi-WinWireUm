//
// BCM GPIO Controller
//
#pragma once

#include "common.h"
#include "bcm.h"

#define BCM_GPIO_BASE_OFFSET		0x00200000
#define BCM_GPIO_CPU_BASE			(BCM_CPU_PERIPH_BASE + BCM_GPIO_BASE_OFFSET)
#define BCM_GPIO_BUS_BASE			(BCM_BUS_PERIPH_BASE + BCM_GPIO_BASE_OFFSET)
#define BCM_GPIO_REG_LEN			0x100

#define BCM_GPIO_FSEL_LENGTH		6
#define BCM_GPIO_SET_LENGTH			2
#define BCM_GPIO_CLR_LENGTH			2
#define BCM_GPIO_LEV_LENGTH			2

namespace Wi2Pi
{
#include <pshpack4.h>
	typedef struct _BCM_GPIO_REGISTERS
	{
		ULONG FuncSelect[BCM_GPIO_FSEL_LENGTH];			// GPFSEL0-5
		ULONG Reserved0;
		ULONG Set[BCM_GPIO_SET_LENGTH];					// GPSET0-1
		ULONG Reserved1;
		ULONG Clear[BCM_GPIO_CLR_LENGTH];				// GPCLR0-1
		ULONG Reserved2;
		ULONG Level[BCM_GPIO_LEV_LENGTH];				// GPLEV0-1

	} BCM_GPIO_REGISTERS, *PBCM_GPIO_REGISTERS;
#include <poppack.h>

	enum BCM_AVAIL_GPIO_PIN
	{
		BCM_GPIO4 = 4,	// P1-7
		BCM_GPIO17 = 17,	// P1-11
		BCM_GPIO18 = 18,	// P1-12
		BCM_GPIO21 = 21,   // P1-13
		BCM_GPIO22 = 22,   // P1-15
		BCM_GPIO23 = 23,   // P1-16
		BCM_GPIO24 = 24,	// P1-18
		BCM_GPIO25 = 25,   // P1-22
	};

	enum BCM_GPIO_FSEL
	{
		BCM_GPIO_FSEL_Input = 0x0,
		BCM_GPIO_FSEL_Output,
		BCM_GPIO_FSEL_Alt0 = 0x4,
		BCM_GPIO_FSEL_Alt1,
		BCM_GPIO_FSEL_Alt2,
		BCM_GPIO_FSEL_Alt3,
		BCM_GPIO_FSEL_Alt4 = 0x3,
		BCM_GPIO_FSEL_Alt5 = 0x2,
	};

	static PBCM_GPIO_REGISTERS GpioReg;

	__inline void GpioFuncSelect(int pinNum, BCM_GPIO_FSEL func)
	{
		/*
		A 3-in-1 Raspbian like function
		#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
		#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
		#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
		*/

		// Each reg control 10 GPIO pins
		int nFsel = pinNum / 10;
		ULONG fsel = GpioReg->FuncSelect[nFsel];
		// Each FSEL register controls 10 GPIO pins, and each pin
		// has 3-bits for its function
		int numShifts = (pinNum % 10) * 3;

		// Unset the pin FSEL 3-bits
		fsel &= ~(0x7 << numShifts);

		// Set desired ALT function
		fsel |= func << numShifts;

		WRITE_REGISTER_ULONG(GpioReg->FuncSelect + nFsel, fsel);
	}

	__inline void GpioPinWrite(int pinNum, bool state)
	{
		// Each reg controls 32 GPIO pin
		_ASSERT(pinNum < 32 && pinNum > 0 && "Pin number should be in the range [0,31]");

		// true -> HIGH
		// false -> LOW
		WRITE_REGISTER_NOFENCE_ULONG(state ? GpioReg->Set : GpioReg->Clear, 1 << pinNum);
	}

	__inline bool GpioPinRead(int pinNum)
	{
		// Each reg controls 32 GPIO pin
		_ASSERT(pinNum < 32 && pinNum > 0 && "Pin number should be in the range [0,31]");

		return (READ_REGISTER_NOFENCE_ULONG(GpioReg->Level) & (1 << pinNum)) > 0;
	}

	__inline void BenchmarkGpio(int pinNum, const int numSamples)
	{
		PerfTimer timer;

		GpioFuncSelect(pinNum, BCM_GPIO_FSEL_Output);

		timer.Start();
		for (int i = 0; i < numSamples; ++i)
		{
			GpioPinWrite(pinNum, (i & 1) > 0);
		}
		timer.Stop();

		LogInfo("GPIO Writes/Second = %f", timer.CalcOpsPerSecond(numSamples));

		GpioFuncSelect(pinNum, BCM_GPIO_FSEL_Input);

		timer.Start();
		for (int i = 0; i < numSamples; ++i)
		{
			bool state = GpioPinRead(pinNum);
		}
		timer.Stop();

		LogInfo("GPIO Reads/Second = %f", timer.CalcOpsPerSecond(numSamples));
	}
}