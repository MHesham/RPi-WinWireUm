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

//
// BCM Clock Manager
//

#define BCM_CM_CPU_BASE				0x3F101000
#define BCM_CM_REG_LEN				0xA8

#define BCM_CM_PWM_PSSWD			(0x5A000000)
#define BCM_CM_PCM_PSSWD			BCM_CM_PWM_PSSWD

//
// Clock Manager PWM & PCM Control Flags
//
#define BCM_PWMCLK_CTL_SRC_PLLD		0x6
#define BCM_PCMCLK_CTL_SRC_PLLD		BCM_PWMCLK_CTL_SRC_PLLD

#define BCM_PWMCLK_CTL_ENABLE		(1<<4)
#define BCM_PCMCLK_CTL_ENABLE		BCM_PWMCLK_CTL_ENABLE

#define BCM_PWMCLK_CTL_KILL			(1<<5)
#define BCM_PCMCLK_CTL_KILL			BCM_PWMCLK_CTL_KILL

#define BCM_PWMCLK_CTL_BUSY			(1<<7)
#define BCM_PCMCLK_CTL_BUSY			BCM_PWMCLK_CTL_BUSY

//
// Clock Manager PWM & PCM Divisor Flags
//
#define BCM_PWMCLK_DIV_INT(X)		((X)<<12)
#define BCM_PCMCLK_DIV_INT(X)		BCM_PWMCLK_DIV_INT(X)

//
// Trial and Error Hacks
//
#define BCM_PWMCLK_WAIT_STALL_US	5
#define BCM_PCMCLK_WAIT_STALL_US	BCM_PWMCLK_WAIT_STALL_US

#define BCM_PWMCLK_WAIT_CYCLE		10
#define BCM_PCMCLK_WAIT_CYCLE		BCM_PWMCLK_WAIT_CYCLE

namespace WinWire {
    namespace RPi2 {

#include <pshpack4.h>
        typedef struct _BCM_CM_REGISTERS
        {
            ULONG Reserved0[38];
            ULONG PcmControl;          // CM_PWMCTL
            ULONG PcmDivisor;          // CM_PWMDIV
            ULONG PwmControl;          // CM_PWMCTL
            ULONG PwmDivisor;          // CM_PWMDIV
        } BCM_CM_REGISTERS, *PBCM_CM_REGISTERS;
#include <poppack.h>

        static PBCM_CM_REGISTERS CmReg;

        bool StopPwmClock()
        {
            // Turn off PWM clock and wait for busy flag to go low
            WRITE_REGISTER_ULONG(&CmReg->PwmControl, BCM_CM_PWM_PSSWD);
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
                WRITE_REGISTER_ULONG(&CmReg->PwmControl, BCM_CM_PWM_PSSWD | BCM_PWMCLK_CTL_KILL);
                MicroDelay(100);

                if (READ_REGISTER_ULONG(&CmReg->PwmControl) & BCM_PWMCLK_CTL_BUSY)
                {
                    LogError("Killing PWM clock failed");
                    return false;
                }
            }

            return true;
        }

        bool StopPcmClock()
        {
            // Turn off PCM clock and wait for busy flag to go low
            WRITE_REGISTER_ULONG(&CmReg->PcmControl, BCM_CM_PCM_PSSWD);
            MicroDelay(100);

            int cnt;
            for (cnt = 0; cnt < BCM_PCMCLK_WAIT_CYCLE &&
                READ_REGISTER_ULONG(&CmReg->PcmControl) & BCM_PCMCLK_CTL_BUSY;
                ++cnt)
            {
                MicroDelay(BCM_PCMCLK_WAIT_STALL_US);
            }

            // If we couldn't shutdown the PWM clock, then forcibly kill it
            if (cnt == BCM_PCMCLK_WAIT_CYCLE)
            {
                LogInfo("PCM clock graceful shutdown failed, forcibly killing it!");
                WRITE_REGISTER_ULONG(&CmReg->PcmControl, BCM_CM_PCM_PSSWD | BCM_PCMCLK_CTL_KILL);
                MicroDelay(100);

                if (READ_REGISTER_ULONG(&CmReg->PcmControl) & BCM_PCMCLK_CTL_BUSY)
                {
                    LogError("Killing PCM clock failed");
                    return false;
                }
            }

            return true;
        }

        void DumpCmRegisters()
        {
            LogInfo(
                "\nDumping CM Registers\n"
                "    PCM Clock Control =    0x%08x\n"
                "    PCM Clock Divisor =    0x%08x\n"
                "    PWM Clock Control =    0x%08x\n"
                "    PWM Clock Divisor =    0x%08x\n",
                READ_REGISTER_ULONG(&CmReg->PcmControl),
                READ_REGISTER_ULONG(&CmReg->PcmDivisor),
                READ_REGISTER_ULONG(&CmReg->PwmControl),
                READ_REGISTER_ULONG(&CmReg->PwmDivisor));
        }
    }
}
