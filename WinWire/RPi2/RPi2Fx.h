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

#pragma once

#include "Fx.h"
#include "FxKm.h"
#include "regaccess.h"
#include "RPi2\bcmdma.h"
#include "RPi2\bcmpwm.h"
#include "RPi2\bcmpcm.h"
#include "RPi2\bcmcm.h"
#include "RPi2\bcmgpio.h"
#include "RPi2\SwPwm.h"

namespace WinWire {
	namespace RPi2
	{
		class RPi2Fx : public Fx
		{
		public:

			static RPi2Fx& Inst()
			{
				static RPi2Fx inst;
				return inst;
			}

			bool MapPrephirals()
			{
				PwmReg = (PBCM_PWM_REGISTERS)FxKm::Inst().Map((PVOID)BCM_PWM_CPU_BASE, BCM_PWM_REG_LEN).UserAddress;

				if (!PwmReg)
				{
					LogError("Map PWM registers failed");
					return false;
				}

				LogInfo("PWM Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", PwmReg, BCM_PWM_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_PWM_CPU_BASE));

				PcmReg = (PBCM_PCM_REGISTERS)FxKm::Inst().Map((PVOID)BCM_PCM_CPU_BASE, BCM_PCM_REG_LEN).UserAddress;

				if (!PcmReg)
				{
					LogError("Map PCM registers failed");
					return false;
				}

				LogInfo("PCM Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", PcmReg, BCM_PCM_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_PCM_CPU_BASE));

				GpioReg = (PBCM_GPIO_REGISTERS)FxKm::Inst().Map((PVOID)BCM_GPIO_CPU_BASE, BCM_GPIO_REG_LEN).UserAddress;

				if (!GpioReg)
				{
					LogError("Map GPIO registers failed");
					return false;
				}

				LogInfo("GPIO Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", GpioReg, BCM_GPIO_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_GPIO_CPU_BASE));

				DmaReg = (PBCM_DMA_REGISTERS)FxKm::Inst().Map((PVOID)BCM_DMA_CPU_BASE, BCM_DMA_REG_LEN).UserAddress;

				if (!DmaReg)
				{
					LogError("Map DMA registers failed");
					return false;
				}

				LogInfo("DMA Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", DmaReg, BCM_DMA_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_DMA_CPU_BASE));

				CmReg = (PBCM_CM_REGISTERS)FxKm::Inst().Map((PVOID)BCM_CM_CPU_BASE, BCM_CM_REG_LEN).UserAddress;

				if (!CmReg)
				{
					LogError("Map CM registers failed");
					return false;
				}

				LogInfo("Clock Manager Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", CmReg, BCM_CM_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_CM_CPU_BASE));

				return true;
			}

		protected:
			RPi2Fx() {}
		};
	}
}