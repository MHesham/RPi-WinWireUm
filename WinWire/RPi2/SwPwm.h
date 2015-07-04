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

#include "FxKm.h"

namespace WinWire {
    namespace RPi2 {
#define PWM_DEFAULT_FREQ_HZ		390.625
#define PWM_MAX_FREQ_HZ			3906.25

        typedef struct _ControlData
        {
            PBYTE Base;
            ULONG Length;
            ULONG *Step;
            ULONG StepLength;
            BCM_DMA_CB *CB;
            ULONG CbLength;
            ULONG PA;
            int PulseNumSteps;
            int PulseNumDmaCbs;

            PULONG GetChannelStepVA(int channel, int stepIdx)
            {
                return (Step + (channel * PulseNumSteps) + stepIdx);
            }

            ULONG GetChannelStepBA(int channel, int stepIdx)
            {
                int stepOffset = (PBYTE)(Step + (channel * PulseNumSteps) + stepIdx) - (PBYTE)Base;
                return BCM_CPU_TO_BUS_DRAM_ADDR((ULONG)PA + stepOffset);
            }

            PBCM_DMA_CB GetChannelCbVA(int channel, int cbIdx)
            {
                return (CB + (channel * PulseNumDmaCbs) + cbIdx);
            }

            ULONG GetChannelCbBA(int channel, int cbIdx)
            {
                int cbOffset = (PBYTE)(CB + (channel * PulseNumDmaCbs) + cbIdx) - (PBYTE)Base;
                ULONG ba = BCM_CPU_TO_BUS_DRAM_ADDR((ULONG)PA + cbOffset);
                _ASSERT(!(ba % 32) && "CB BA should be 256-bit (32-byte)aligned");

                return ba;
            }

        } ControlData, *PControlData;

        class SwPwm
        {
        public:
            // 1 CB for setting GPIO CLR or SET register, the other wait for PWM fifo and do dummy write
            static const int STEP_NUM_CBS = 2;
            static const int CHANNEL_WIDTH_MAX = 256;

            ~SwPwm()
            {
                FinalizePrephirals();
            }

            static SwPwm& Inst()
            {
                static SwPwm inst;
                return inst;
            }

            bool Init(const int *pwmPins, int numPins, double freqHz)
            {
                NumChannels = numPins;

                if (freqHz > PWM_MAX_FREQ_HZ)
                    freqHz = PWM_MAX_FREQ_HZ;

                PulseFreqHz = freqHz;
                PulsePeriodUs = int(round(1000000.0 / PulseFreqHz));

                PulseStepTimeUs = int(round(PulsePeriodUs / CHANNEL_WIDTH_MAX));

                // Adjust frequency/period for 8-bit resolution based on the 8-bit divisible step time
                PulsePeriodUs = PulseStepTimeUs * CHANNEL_WIDTH_MAX;
                PulseFreqHz = int(round(1000000.0 / PulsePeriodUs));

                PulseNumSteps = (PulsePeriodUs / PulseStepTimeUs);

                // PulseNumDmaCbs is number of DMA Control Blocks for 1 Pulse
                PulseNumDmaCbs = (PulseNumSteps * STEP_NUM_CBS);

                ULONG controlDataNonAlignedLength =
                    ULONG(sizeof(ULONG) * PulseNumSteps * NumChannels) +
                    ULONG(sizeof(BCM_DMA_CB) * PulseNumDmaCbs * NumChannels);

                ControlDataAlignedLength = ROUNDUP(controlDataNonAlignedLength, 4096);

                LogInfo("SwPwm Configurations:\n"
                    "    Pulse Freq: %fHz\n"
                    "    Pulse Period: %dus\n"
                    "    Pulse Step Time: %dus\n"
                    "    Pulse Num Steps: %d\n"
                    "    Num Channels: %d\n"
                    "    Channel Max Width: %d (%dus)\n"
                    "    Channel Num DMA CBs: %d\n"
                    "    Control Data Byte Aligned Size: %d\n",
                    PulseFreqHz,
                    PulsePeriodUs,
                    PulseStepTimeUs,
                    PulseNumSteps,
                    NumChannels,
                    CHANNEL_WIDTH_MAX,
                    CHANNEL_WIDTH_MAX * PulseStepTimeUs,
                    PulseNumDmaCbs,
                    ControlDataAlignedLength);

                LogInfo("Channel to GPIO pin mapping:");
                for (int channel = 0; channel < NumChannels; ++channel)
                {
                    int pin = pwmPins[channel];
                    ChannelGpioPin[channel] = pin;
                    GpioPinChannel[pin] = channel;

                    ChannelDmaReg[channel] = (PBCM_DMA_CH_REGISTERS)((PBYTE)DmaReg + (BCM_DMA_NUM_CHANNELS - channel - 1) * BCM_DMA_CHANNEL_REG_LEN);

                    LogInfo("    CH%d -> GPIO%d", channel, pin);
                }

                if (!CreateAndInitDmaControlBlocks())
                {
                    LogError("Failed to create DMA control blocks");
                    return false;
                }

                if (!InitPrephirals())
                {
                    LogError("Failed to init prephirals");
                    return false;
                }

                DumpPcmRegisters();
                DumpCmRegisters();
                DumpDmaRegisters();

                return true;
            }

            void SetPinDutyCycle(int pin, double perct)
            {
                SetChannelDutyCycle(GpioPinChannel[pin], perct);
            }

            void SetChannelDutyCycle(int channel, double perct)
            {
                if (channel >= NumChannels)
                {
                    LogError("Invalid CH%d", channel);
                    return;
                }

                int width = (int)(perct * CHANNEL_WIDTH_MAX);

                // Clamp width to the allowed range
                if (width < 0)
                    width = 0;
                else if (width > CHANNEL_WIDTH_MAX)
                    width = CHANNEL_WIDTH_MAX;

                ULONG gpioClr0BA = BCM_GPIO_BUS_BASE + ((ULONG)GpioReg->Clear - (ULONG)GpioReg);
                ULONG gpioSet0BA = BCM_GPIO_BUS_BASE + ((ULONG)GpioReg->Set - (ULONG)GpioReg);
                PBCM_DMA_CB channelCB0 = CtrlDataVA.GetChannelCbVA(channel, 0);
                PULONG channelStep = CtrlDataVA.GetChannelStepVA(channel, 0);

                ULONG channelMask = 1 << ChannelGpioPin[channel];

                channelStep[width] = channelMask;

                if (width == 0)
                {
                    channelCB0->DEST_AD = gpioClr0BA;
                }
                else
                {
                    for (int i = width - 1; i > 0; --i)
                        channelStep[i] = 0;

                    channelStep[0] = channelMask;
                    channelCB0->DEST_AD = gpioSet0BA;
                }
            }

        private:

            SwPwm() :
                NumChannels(0),
                PulsePeriodUs(0),
                PulseStepTimeUs(0),
                PulseNumSteps(0),
                PulseNumDmaCbs(0),
                PulseFreqHz(PWM_DEFAULT_FREQ_HZ)
            {
                ZeroMemory((PVOID)GpioPinChannel, sizeof(GpioPinChannel));
                ZeroMemory((PVOID)ChannelGpioPin, sizeof(ChannelGpioPin));
                ZeroMemory((PVOID)ChannelDmaReg, sizeof(ChannelDmaReg));
            }

            bool InitPrephirals()
            {
                LogInfo("Initializing prehirals");

                for (int i = 0; i < NumChannels; ++i)
                {
                    GpioFuncSelect(ChannelGpioPin[i], BCM_GPIO_FSEL_Output);
                    GpioPinWrite(ChannelGpioPin[i], 0);
                }

                LogInfo("All Channels GPIO pins are set to output and asserted LOW");

                // Enable PCM and Disabe TX/RX
                WRITE_REGISTER_ULONG(&PcmReg->ControlAndStatus, BCM_PCM_REG_CS_EN);
                MicroDelay(10);

                if (!StopPcmClock())
                    return false;

                // Set PCM clock source
                WRITE_REGISTER_ULONG(&CmReg->PcmControl, BCM_CM_PCM_PSSWD | BCM_PCMCLK_CTL_SRC_PLLD);
                MicroDelay(100);

                // Divisor = 50 = 10MHz Clock
                WRITE_REGISTER_ULONG(&CmReg->PcmDivisor, BCM_CM_PCM_PSSWD | BCM_PCMCLK_DIV_INT(50));
                MicroDelay(100);

                // Enabl PCM clock and wait for busy flag to go high
                WRITE_REGISTER_ULONG(&CmReg->PcmControl, BCM_CM_PCM_PSSWD | BCM_PCMCLK_CTL_ENABLE | BCM_PCMCLK_CTL_SRC_PLLD);
                MicroDelay(100);

                while (!(READ_REGISTER_ULONG(&CmReg->PcmControl) & BCM_PCMCLK_CTL_BUSY));

                LogInfo("PCM clock configured");

                // Configure TX for 1 channel with 8-bits width
                WRITE_REGISTER_ULONG(
                    &PcmReg->TransmitConfig,
                    BCM_PCM_REG_TXC_CH1WEX(0) | BCM_PCM_REG_TXC_CH1EN(1) | BCM_PCM_REG_TXC_CH1POS(0) | BCM_PCM_REG_TXC_CH1WID(0));
                MicroDelay(10);

                // Set Frame Length
                WRITE_REGISTER_ULONG(&PcmReg->Mode, BCM_PCM_REG_MODE_FLEN((PulseStepTimeUs * 10) - 1));
                MicroDelay(10);

                // Clear TX/RX FIFOs
                WRITE_REGISTER_ULONG(&PcmReg->ControlAndStatus, READ_REGISTER_ULONG(&PcmReg->ControlAndStatus) | BCM_PCM_REG_CS_TXCLR | BCM_PCM_REG_CS_RXCLR);
                MicroDelay(10);

                // Configure the DMA DREQ and Panic levels to be generated when the 64 x 32-bit TX FIFO has 1 slot available
                WRITE_REGISTER_ULONG(&PcmReg->DreqLevel, BCM_PCM_REG_DREQ_TX(BCM_PCM_FIFO_LEN) | BCM_PCM_REG_DREQ_TX_PANIC(BCM_PCM_FIFO_LEN));
                MicroDelay(10);

                // Enable PCM DMA DREQ generation for the FIFO
                WRITE_REGISTER_ULONG(&PcmReg->ControlAndStatus, READ_REGISTER_ULONG(&PcmReg->ControlAndStatus) | BCM_PCM_REG_CS_DMAEN);
                MicroDelay(10);

                LogInfo("PCM controller configured");

                // Initialize the DMA
                for (int ch = 0; ch < NumChannels; ++ch)
                {
                    PBCM_DMA_CH_REGISTERS dmaReg = ChannelDmaReg[ch];

                    WRITE_REGISTER_ULONG(&dmaReg->ControlAndStatus, BCM_DMA_CS_ABORT);
                    MicroDelay(100);

                    WRITE_REGISTER_ULONG(&dmaReg->ControlAndStatus, BCM_DMA_CS_RESET);
                    MicroDelay(10);

                    // Clear the read-only INT and END status flags by setting them
                    WRITE_REGISTER_ULONG(&dmaReg->ControlAndStatus, BCM_DMA_CS_INT | BCM_DMA_CS_END);

                    WRITE_REGISTER_ULONG(&dmaReg->ControlBlockAddr, CtrlDataVA.GetChannelCbBA(ch, 0));

                    // Clear interesting debug flags
                    WRITE_REGISTER_ULONG(&dmaReg->Debug, BCM_DMA_DEBUG_FIFO_ERROR | BCM_DMA_DEBUG_READ_ERROR);

                    WRITE_REGISTER_ULONG(
                        &dmaReg->ControlAndStatus,
                        BCM_DMA_CS_ACTIVE | BCM_DMA_WAIT_FOR_OUTSTADNDING_WRITES | BCM_DMA_CS_PANIC_PRIORITY(0x8) | BCM_DMA_CS_PRIORITY(0x8));
                }

                LogInfo("DMA controllers configured");

                // Enable TX
                WRITE_REGISTER_ULONG(&PcmReg->ControlAndStatus, READ_REGISTER_ULONG(&PcmReg->ControlAndStatus) | BCM_PCM_REG_CS_TXON);

                return true;
            }

            bool CreateAndInitDmaControlBlocks()
            {
                LogInfo("Creating and initializing DMA control blocks");

                auto res = FxKm::Inst().AllocMap(ControlDataAlignedLength);
                CtrlDataVA.PA = res.PhysicalAddress;
                CtrlDataVA.Base = (PBYTE)res.UserAddress;

                if (CtrlDataVA.Base == NULL)
                {
                    LogError("FxKm::AllocMap failed");
                    return false;
                }

                CtrlDataVA.Length = ControlDataAlignedLength;

                CtrlDataVA.Step = (ULONG*)CtrlDataVA.Base;
                _ASSERT((ULONG)CtrlDataVA.Step % 32 == 0);
                CtrlDataVA.PulseNumSteps = PulseNumSteps;
                CtrlDataVA.StepLength = PulseNumSteps * NumChannels;

                CtrlDataVA.CB = (PBCM_DMA_CB)(CtrlDataVA.Step + CtrlDataVA.StepLength);
                _ASSERT((ULONG)CtrlDataVA.CB % 32 == 0);
                CtrlDataVA.PulseNumDmaCbs = PulseNumDmaCbs;
                CtrlDataVA.CbLength = PulseNumDmaCbs * NumChannels;

                LogInfo("Allocated %d byte ControlData block @VA:0x%08x @PA:0x%08x", CtrlDataVA.Length, CtrlDataVA.Base, CtrlDataVA.PA);

                ZeroMemory(CtrlDataVA.Base, ControlDataAlignedLength);
                for (int channel = 0; channel < NumChannels; ++channel)
                {
                    for (int step = 0; step < PulseNumSteps; ++step)
                    {
                        CtrlDataVA.Step[(channel * PulseNumSteps) + step] = 1 << ChannelGpioPin[channel];
                    }
                }

                LogInfo("Touched all allocated DMA control blocks physical memory");

                ULONG pcmFifoBA = BCM_PCM_BUS_BASE + ((ULONG)&PcmReg->Fifo - (ULONG)PcmReg);
                ULONG gpioClr0BA = BCM_GPIO_BUS_BASE + ((ULONG)GpioReg->Clear - (ULONG)GpioReg);

                for (int channel = 0; channel < NumChannels; ++channel)
                {
                    int currCbIdx = 0;

                    for (int step = 0; step < PulseNumSteps; ++step)
                    {
                        PBCM_DMA_CB cb0VA = CtrlDataVA.GetChannelCbVA(channel, currCbIdx);

                        cb0VA->TI = BCM_DMA_TI_NO_WIDE_BURSTS | BCM_DMA_TI_WAIT_RESP;
                        cb0VA->SOURCE_AD = CtrlDataVA.GetChannelStepBA(channel, step);
                        cb0VA->DEST_AD = gpioClr0BA;
                        cb0VA->TXFR_LEN = 4;
                        cb0VA->STRIDE = 0;
                        cb0VA->NEXTCONBK = CtrlDataVA.GetChannelCbBA(channel, currCbIdx + 1);

                        ++currCbIdx;

                        PBCM_DMA_CB cb1VA = CtrlDataVA.GetChannelCbVA(channel, currCbIdx);
                        cb1VA->TI = BCM_DMA_TI_NO_WIDE_BURSTS | BCM_DMA_TI_WAIT_RESP | BCM_DMA_TI_DEST_DREQ | BCM_DMA_IT_PER_MAP(BCM_PCM_DMA_DREQ);
                        // Any dummy data is used and its value will have no effect
                        // We use the FIFO just to control pulse timing
                        cb1VA->SOURCE_AD = CtrlDataVA.GetChannelStepBA(channel, 0);
                        cb1VA->DEST_AD = pcmFifoBA;
                        cb1VA->TXFR_LEN = 4;
                        cb1VA->STRIDE = 0;
                        cb1VA->NEXTCONBK = CtrlDataVA.GetChannelCbBA(channel, currCbIdx + 1);

                        ++currCbIdx;
                    }

                    // Make the CBs circular linked list by linking last CB to first CB
                    PBCM_DMA_CB cbVA = CtrlDataVA.GetChannelCbVA(channel, currCbIdx - 1);
                    cbVA->NEXTCONBK = CtrlDataVA.GetChannelCbBA(channel, 0);
                }

                return true;
            }

            void FinalizePrephirals()
            {
                LogInfo("Finalizing prephirals");

                WRITE_REGISTER_ULONG(&PcmReg->ControlAndStatus, 0);
                MicroDelay(100);

                (void)StopPcmClock();

                LogInfo("PCM controller and clock stopped");

                //
                // Stop the DMA engines
                //
                for (int ch = 0; ch < BCM_DMA_NUM_CHANNELS; ++ch)
                {
                    PBCM_DMA_CH_REGISTERS dmaReg = ChannelDmaReg[ch];

                    if (dmaReg == nullptr)
                        continue;

                    WRITE_REGISTER_ULONG(&dmaReg->ControlAndStatus, BCM_DMA_CS_ABORT);
                    MicroDelay(100);

                    WRITE_REGISTER_ULONG(&dmaReg->ControlAndStatus, READ_REGISTER_ULONG(&dmaReg->ControlAndStatus) & ~BCM_DMA_CS_ACTIVE);
                    WRITE_REGISTER_ULONG(&dmaReg->ControlBlockAddr, 0);
                    WRITE_REGISTER_ULONG(&dmaReg->ControlAndStatus, READ_REGISTER_ULONG(&dmaReg->ControlAndStatus) | BCM_DMA_CS_RESET);

                    MicroDelay(100);
                }

                LogInfo("DMA engine stopped");

                for (int i = 0; i < NumChannels; ++i)
                {
                    GpioFuncSelect(ChannelGpioPin[i], BCM_GPIO_FSEL_Output);
                    GpioPinWrite(ChannelGpioPin[i], 0);
                }
            }

        private:

            ULONG ControlDataAlignedLength;

            ControlData CtrlDataVA;
            double PulseFreqHz;
            int NumChannels;
            int PulsePeriodUs;
            int PulseNumSteps;
            // PulseStepTimeUs is the pulse width increment granularity, again in microseconds.
            // Setting it too low will likely cause problems as the DMA controller will use too much
            // memory bandwidth. 10us is a good value, though you might be ok setting it as low as 2us.
            // The pace at which the DMA transfer kicks-in in microseconds, the lower the higher precision
            int PulseStepTimeUs;
            int PulseNumDmaCbs;
            int ChannelGpioPin[BCM_GPIO_BANK0_NUM_PINS];
            int GpioPinChannel[BCM_GPIO_BANK0_NUM_PINS];
            PBCM_DMA_CH_REGISTERS ChannelDmaReg[BCM_DMA_NUM_CHANNELS];
        };
    }
}