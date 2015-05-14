#pragma once

#include "MMap.h"

namespace Wi2Pi
{
#define PWM_DEFAULT_FREQ_HZ		390.625
#define PWM_MAX_FREQ_HZ			3906.25

	typedef struct _ControlData
	{
		PBYTE Base;
		ULONG Length;
		ULONG *Step;	// [PulseNumSteps];
		ULONG StepLength;
		BCM_DMA_CB *CB; // [PulseNumDmaCbs];
		ULONG CbLength;
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

			ULONG controlDataNonAlignedLength = ULONG(sizeof(ULONG) * PulseNumSteps) + ULONG(sizeof(BCM_DMA_CB) * PulseNumDmaCbs);

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

			DumpPwmRegisters();
			DumpCmRegisters();
			DumpDmaRegisters();

			return true;
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
			PBCM_DMA_CB channelCB0 = CtrlDataVA.CB + channel * PulseNumSteps * STEP_NUM_CBS;
			PULONG channelStep = CtrlDataVA.Step + channel * PulseNumSteps;

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
			CtrlDataPA(NULL),
			NumChannels(0),
			PulsePeriodUs(0),
			PulseStepTimeUs(0),
			PulseNumSteps(0),
			PulseNumDmaCbs(0),
			PulseFreqHz(PWM_DEFAULT_FREQ_HZ)
		{
			ZeroMemory((PVOID)GpioPinChannel, sizeof(GpioPinChannel));
			ZeroMemory((PVOID)ChannelGpioPin, sizeof(ChannelGpioPin));
		}

		ULONG GetControlDataStepBA(int stepIdx)
		{
			int stepOffset = (PBYTE)(CtrlDataVA.Step + stepIdx) - (PBYTE)CtrlDataVA.Base;
			return BCM_CPU_TO_BUS_DRAM_ADDR((ULONG)CtrlDataPA + stepOffset);
		}

		ULONG GetControlDataCbBA(int cbIdx)
		{
			int cbOffset = (PBYTE)(CtrlDataVA.CB + cbIdx) - (PBYTE)CtrlDataVA.Base;
			ULONG ba = BCM_CPU_TO_BUS_DRAM_ADDR((ULONG)CtrlDataPA + cbOffset);
			_ASSERT(!(ba % 32) && "CB BA should be 256-bit (32-byte)aligned");

			return ba;
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

			// Turn off PWM controller
			WRITE_REGISTER_ULONG(&PwmReg->Control, 0);
			MicroDelay(10);

			if (!StopPwmClock(CmReg))
				return false;

			// Set PWM clock source
			WRITE_REGISTER_ULONG(&CmReg->PwmControl, BCM_PWMCLK_PSSWD | BCM_PWMCLK_CTL_SRC_PLLD);
			MicroDelay(100);

			// Divisor = 500 = 1MHz Clock
			WRITE_REGISTER_ULONG(&CmReg->PwmDivisor, BCM_PWMCLK_PSSWD | BCM_PWMCLK_DIV_INT(50));
			MicroDelay(100);

			// Enabl PWM clock and wait for busy flag to go high
			WRITE_REGISTER_ULONG(&CmReg->PwmControl, BCM_PWMCLK_PSSWD | BCM_PWMCLK_CTL_ENABLE | BCM_PWMCLK_CTL_SRC_PLLD);
			MicroDelay(100);

			while (!(READ_REGISTER_ULONG(&CmReg->PwmControl) & BCM_PWMCLK_CTL_BUSY));

			LogInfo("PWM clock configured");

			WRITE_REGISTER_ULONG(&PwmReg->Ch1Range, PulseStepTimeUs * 10);
			MicroDelay(10);

			// Enable DMA and set PANIC and DREQ threshold to 15
			WRITE_REGISTER_ULONG(&PwmReg->DmaConfig, BCM_PWM_REG_DMAC_ENAB | BCM_PWM_REG_DMAC_DREQ_15 | BCM_PWM_REG_DMAC_PANIC_15);
			MicroDelay(10);

			// Clear CH1 FIFO
			WRITE_REGISTER_ULONG(&PwmReg->Control, BCM_PWM_REG_CTL_CLRF1);
			MicroDelay(10);

			// Use PWM mode, PWM algorithm, FIFO for CH1 transmission and enable PWM on CH1
			WRITE_REGISTER_ULONG(&PwmReg->Control, BCM_PWM_REG_CTL_USEF1 | BCM_PWM_REG_CTL_PWEN1);
			MicroDelay(10);

			LogInfo("PWM controller configured");

			// Initialize the DMA
			WRITE_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus, BCM_DMA_CS_ABORT);
			MicroDelay(100);

			WRITE_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus, BCM_DMA_CS_RESET);
			MicroDelay(10);

			// Clear the read-only INT and END status flags by setting them
			WRITE_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus, BCM_DMA_CS_INT | BCM_DMA_CS_END);

			WRITE_REGISTER_ULONG(&DmaReg->Ch14ControlBlockAddr, GetControlDataCbBA(0));

			// Clear interesting debug flags
			WRITE_REGISTER_ULONG(&DmaReg->Ch14Debug, BCM_DMA_DEBUG_FIFO_ERROR | BCM_DMA_DEBUG_READ_ERROR);

			WRITE_REGISTER_ULONG(
				&DmaReg->Ch14ControlAndStatus,
				BCM_DMA_CS_ACTIVE | BCM_DMA_WAIT_FOR_OUTSTADNDING_WRITES | BCM_DMA_CS_PANIC_PRIORITY(0x8) | BCM_DMA_CS_PRIORITY(0x8));

			LogInfo("DMA controller configured");

			return true;
		}

		bool CreateAndInitDmaControlBlocks()
		{
			LogInfo("Creating and initializing DMA control blocks");

			auto res = MMap::Inst().AllocMap(ControlDataAlignedLength);
			CtrlDataPA = res.PhysicalAddress;
			CtrlDataVA.Base = (PBYTE)res.UserAddress;

			if (CtrlDataVA.Base == NULL)
			{
				LogError("MMap::AllocMap failed");
				return false;
			}

			CtrlDataVA.Length = ControlDataAlignedLength;
			CtrlDataVA.Step = (ULONG*)CtrlDataVA.Base;
			_ASSERT((ULONG)CtrlDataVA.Step % 32 == 0);
			CtrlDataVA.StepLength = PulseNumSteps;
			CtrlDataVA.CB = (PBCM_DMA_CB)(CtrlDataVA.Step + CtrlDataVA.StepLength);
			_ASSERT((ULONG)CtrlDataVA.CB % 32 == 0);
			CtrlDataVA.CbLength = PulseNumDmaCbs;

			LogInfo("Allocated %d byte ControlData block @VA:0x%08x @PA:0x%08x", CtrlDataVA.Length, CtrlDataVA.Base, CtrlDataPA);

			ZeroMemory(CtrlDataVA.Base, ControlDataAlignedLength);
			for (int channel = 0; channel < NumChannels; ++channel)
			{
				for (int step = 0; step < PulseNumSteps; ++step)
				{
					CtrlDataVA.Step[(channel * PulseNumSteps) + step] = 1 << ChannelGpioPin[channel];
				}
			}

			LogInfo("Touched all allocated DMA control blocks physical memory");

			ULONG pwmFifoBA = BCM_PWM_BUS_BASE + ((ULONG)&PwmReg->FifoInput - (ULONG)PwmReg);
			ULONG gpioClr0BA = BCM_GPIO_BUS_BASE + ((ULONG)GpioReg->Clear - (ULONG)GpioReg);

			int currCbIdx = 0;
			ULONG firstCbBA = 0;

			for (int step = 0; step < PulseNumSteps; ++step)
			{
				PBCM_DMA_CB cb0VA = CtrlDataVA.CB + currCbIdx;

				cb0VA->TI = BCM_DMA_TI_NO_WIDE_BURSTS | BCM_DMA_TI_WAIT_RESP;
				cb0VA->SOURCE_AD = GetControlDataStepBA(step);
				cb0VA->DEST_AD = gpioClr0BA;
				cb0VA->TXFR_LEN = 4;
				cb0VA->STRIDE = 0;
				cb0VA->NEXTCONBK = GetControlDataCbBA(currCbIdx + 1);

				if (firstCbBA == 0)
				{
					ULONG firstCbOffset = (ULONG)cb0VA - (ULONG)CtrlDataVA.Base;
					firstCbBA = BCM_CPU_TO_BUS_DRAM_ADDR((ULONG)CtrlDataPA + firstCbOffset);
				}

				++currCbIdx;

				PBCM_DMA_CB cb1VA = CtrlDataVA.CB + currCbIdx;
				cb1VA->TI = BCM_DMA_TI_NO_WIDE_BURSTS | BCM_DMA_TI_WAIT_RESP | BCM_DMA_TI_DEST_DREQ | BCM_DMA_IT_PER_MAP(BCM_PWM_DMA_DREQ);
				// Any dummy data is used and its value will have no effect
				// We use the FIFO just to control pulse timing
				cb1VA->SOURCE_AD = GetControlDataStepBA(0);
				cb1VA->DEST_AD = pwmFifoBA;
				cb1VA->TXFR_LEN = 4;
				cb1VA->STRIDE = 0;
				cb1VA->NEXTCONBK = GetControlDataCbBA(currCbIdx + 1);

				++currCbIdx;
			}

			// Make the CBs circular linked list by linking last CB to first CB
			PBCM_DMA_CB cbVA = CtrlDataVA.CB + currCbIdx - 1;
			cbVA->NEXTCONBK = GetControlDataCbBA(0);

			_ASSERT(cbVA->NEXTCONBK == firstCbBA && "Last CB should be linked to first CB");

			return true;
		}

		void FinalizePrephirals()
		{
			LogInfo("Finalizing prephirals");

			WRITE_REGISTER_ULONG(&PwmReg->Control, 0);
			MicroDelay(100);

			(void)StopPwmClock(CmReg);

			LogInfo("PWM controller and clock stopped");

			//
			// Stop the DMA engine
			//
			WRITE_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus, BCM_DMA_CS_ABORT);
			MicroDelay(100);

			WRITE_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus, READ_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus) & ~BCM_DMA_CS_ACTIVE);
			WRITE_REGISTER_ULONG(&DmaReg->Ch14ControlBlockAddr, 0);
			WRITE_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus, READ_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus) | BCM_DMA_CS_RESET);

			MicroDelay(100);

			LogInfo("DMA engine stopped");

			for (int i = 0; i < NumChannels; ++i)
			{
				GpioFuncSelect(ChannelGpioPin[i], BCM_GPIO_FSEL_Output);
				GpioPinWrite(ChannelGpioPin[i], 0);
			}
		}

	private:

		ULONG ControlDataAlignedLength;
		ULONG CtrlDataPA;
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
	};
}