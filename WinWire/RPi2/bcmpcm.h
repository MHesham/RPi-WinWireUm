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
// BCM PWM Controller
//

#include "common.h"
#include "RPi2\bcm.h"
#include "regaccess.h"
#include "FxKm.h"

//
// BCM PWM controller registers.
// BCM : Broadcom PWM Controller
//

#define BCM_PCM_BASE_OFFSET			0x00203000
#define BCM_PCM_CPU_BASE			(BCM_CPU_PERIPH_BASE + BCM_PCM_BASE_OFFSET)
#define BCM_PCM_BUS_BASE			(BCM_BUS_PERIPH_BASE + BCM_PCM_BASE_OFFSET)
#define BCM_PCM_REG_LEN				0x24
#define BCM_PCM_FIFO_LEN			64

#define BCM_PCM_DMA_DREQ			2

//
// PCM Control and Status (CS_A)
//

#define BCM_PCM_REG_CS_EN		(1<<0)
#define BCM_PCM_REG_CS_RXON		(1<<1)
#define BCM_PCM_REG_CS_TXON		(1<<2)
#define BCM_PCM_REG_CS_TXCLR	(1<<3)
#define BCM_PCM_REG_CS_RXCLR	(1<<4)
#define BCM_PCM_REG_CS_DMAEN	(1<<9)

//
// PCM Transmit Configuration (TXC_A)
//

#define BCM_PCM_REG_TXC_CH1WEX(X)			((X)<<31)
#define BCM_PCM_REG_TXC_CH1EN(X)			((X)<<30)
#define BCM_PCM_REG_TXC_CH1POS(X)			((X)<<20)
#define BCM_PCM_REG_TXC_CH1WID(X)			((X)<<16)

//
// PCM DMA DREQ Configuration (DREQ_A)
//
#define BCM_PCM_REG_DREQ_TX(X)				((X)<<8)
#define BCM_PCM_REG_DREQ_TX_PANIC(X)		((X)<<24)

//
// PCM Mode (MODA_A)
//
#define BCM_PCM_REG_MODE_FLEN(X)			((X)<<10)

namespace WinWire {
    namespace RPi2
    {
#include <pshpack4.h>
        typedef struct _BCM_PCM_REGISTERS
        {
            ULONG ControlAndStatus;			// CS_A
            ULONG Fifo;						// FIFO_A
            ULONG Mode;						// MODE_A
            ULONG ReceiveConfig;			// RXC_A
            ULONG TransmitConfig;			// TXC_A
            ULONG DreqLevel;				// DREQ_A
            ULONG InterruptEnables;			// INTEN_A
            ULONG InterruptStatusAndClear;	// INTSTC_A
            ULONG GrayCodeMode;             // GRAY
        } BCM_PCM_REGISTERS, *PBCM_PCM_REGISTERS;
#include <poppack.h>

        class BcmPcm
        {
        public:
            BcmPcm() :
                PcmReg(nullptr)
            {}

            static BcmPcm& Inst()
            {
                static BcmPcm inst;
                return inst;
            }

            bool Init()
            {
                if (PcmReg)
                    return true;

                PcmReg = (PBCM_PCM_REGISTERS)FxKm::Inst().Map((PVOID)BCM_PCM_CPU_BASE, BCM_PCM_REG_LEN).UserAddress;

                if (!PcmReg)
                {
                    LogError("Map PCM registers failed");
                    return false;
                }

                LogInfo("PCM Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", PcmReg, BCM_PCM_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_PCM_CPU_BASE));

                return true;
            }

            void DumpRegisters()
            {
                LogInfo(
                    "\nDumping PCM Registers\n"
                    "    Control and Status =         0x%08x\n"
                    "    DREQ Level =                 0x%08x\n"
                    "    Fifo =                       0x%08x\n"
                    "    GrayCode Mode =              0x%08x\n"
                    "    Interrupt Enables =          0x%08x\n"
                    "    Interrupt Status and Clear = 0x%08x\n"
                    "    Mode =                       0x%08x\n"
                    "    Receive Config =             0x%08x\n"
                    "    Transmit Config =            0x%08x\n",
                    READ_REGISTER_ULONG(&PcmReg->ControlAndStatus),
                    READ_REGISTER_ULONG(&PcmReg->DreqLevel),
                    READ_REGISTER_ULONG(&PcmReg->Fifo),
                    READ_REGISTER_ULONG(&PcmReg->GrayCodeMode),
                    READ_REGISTER_ULONG(&PcmReg->InterruptEnables),
                    READ_REGISTER_ULONG(&PcmReg->InterruptStatusAndClear),
                    READ_REGISTER_ULONG(&PcmReg->Mode),
                    READ_REGISTER_ULONG(&PcmReg->ReceiveConfig),
                    READ_REGISTER_ULONG(&PcmReg->TransmitConfig));
            }

            PBCM_PCM_REGISTERS Reg() const { return PcmReg; }

        private:
            PBCM_PCM_REGISTERS PcmReg;
        };
    }
}
