//
// BCM Clock Manager
//
#pragma once

#define BCM_CM_CPU_BASE				0x3F101000
#define BCM_CM_REG_LEN				0xA8

#define BCM_PWMCLK_PSSWD			(0x5A000000)

//
// Clock Manager PWM Control Flags
//
#define BCM_PWMCLK_CTL_SRC_PLLD		0x6
#define BCM_PWMCLK_CTL_ENABLE		(1<<4)
#define BCM_PWMCLK_CTL_KILL			(1<<5)
#define BCM_PWMCLK_CTL_BUSY			(1<<7)

//
// Clock Manager PWM Divisor Flags
//
#define BCM_PWMCLK_DIV_INT(X)		((X)<<12)

//
// Trial and Error Hacks
//
#define BCM_PWMCLK_WAIT_STALL_US	5
#define BCM_PWMCLK_WAIT_CYCLE		10

namespace Wi2Pi
{
#include <pshpack4.h>
	typedef struct _BCM_CM_REGISTERS
	{
		ULONG Reserved0[40];
		ULONG PwmControl;          // CM_PWMCTL
		ULONG PwmDivisor;          // CM_PWMDIV
	} BCM_CM_REGISTERS, *PBCM_CM_REGISTERS;
#include <poppack.h>

	bool StopPwmClock(PBCM_CM_REGISTERS CmReg)
	{
		// Turn off PWM clock and wait for busy flag to go low
		WRITE_REGISTER_ULONG(&CmReg->PwmControl, BCM_PWMCLK_PSSWD);
		MicroDelay(100);

		int cnt;
		for (cnt = 0; cnt < BCM_PWMCLK_WAIT_CYCLE &&
			READ_REGISTER_ULONG(&CmReg->PwmControl) & BCM_PWMCLK_CTL_BUSY;
			++cnt)
		{
			MicroDelay(BCM_PWMCLK_WAIT_STALL_US);
		}

		// If we couldn't shutdown the PWM clock, then forcibly kill it
		if (cnt == BCM_PWMCLK_WAIT_CYCLE)
		{
			LogInfo("PWM clock graceful shutdown failed, forcibly killing it!");
			WRITE_REGISTER_ULONG(&CmReg->PwmControl, BCM_PWMCLK_PSSWD | BCM_PWMCLK_CTL_KILL);
			MicroDelay(100);

			if (READ_REGISTER_ULONG(&CmReg->PwmControl) & BCM_PWMCLK_CTL_BUSY)
			{
				LogError("Killing PWM clock failed");
				return false;
			}
		}

		return true;
	}

	static PBCM_CM_REGISTERS CmReg;

	void DumpCmRegisters()
	{
		LogInfo(
			"\nDumping CM Registers\n"
			"    PWM Clock Control =    0x%08x\n"
			"    PWM Clock Divisor =    0x%08x\n",
			READ_REGISTER_ULONG(&CmReg->PwmControl),
			READ_REGISTER_ULONG(&CmReg->PwmDivisor));
	}
}