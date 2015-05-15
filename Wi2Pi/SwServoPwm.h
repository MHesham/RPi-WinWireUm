#pragma once

#include "Wi2Pi.h"

namespace Wi2Pi
{
	enum ServoType
	{
		ServoDefault = 0,
		ServoHS311,
		ServoMicroSG90,
		ServoUnknown
	};

	const int ServoMinMaxRange[][3] = {
		{ ServoDefault, 1000, 2000 },
		{ ServoHS311, 620, 2270 },
		{ ServoMicroSG90, 640, 2300 },
		{ ServoUnknown, 0, 0},
	};

	const int ChannelGpioPin[] = {
		BCM_GPIO4,
		BCM_GPIO17,
		BCM_GPIO18,
		BCM_GPIO21,
		BCM_GPIO22,
		BCM_GPIO23,
		BCM_GPIO24,
		BCM_GPIO25
	};

	int GpioPinChannel[BCM_GPIO25 + 1];

	// PULSE_PERIOD_US is the pulse cycle time (period) per servo, in microseconds.
	// Typically servos expect it to be 20,000us (20ms). If you are using
	// 8 channels (gpios), this results in a 2.5ms timeslot per gpio channel. A
	// servo output is set high at the start of its 2.5ms timeslot, and set low
	// after the appropriate delay.
#define PULSE_PERIOD_US			20000

		// PULSE_STEP_TIME_US is the pulse width increment granularity, again in microseconds.
		// Setting it too low will likely cause problems as the DMA controller will use too much
		// memory bandwidth. 10us is a good value, though you might be ok setting it as low as 2us.
		// The pace at which the DMA transfer kicks-in in microseconds, the lower the higher precision
#define PULSE_STEP_TIME_US		10

#define PULSE_NUM_STEPS			(PULSE_PERIOD_US/PULSE_STEP_TIME_US)

		// 1 CB for setting GPIO CLR or SET register, the other wait for PWM fifo and do dummy write
#define STEP_NUM_CBS			2

		// PULSE_NUM_DMA_CBS is number of DMA Control Blocks for 1 Pulse
#define PULSE_NUM_DMA_CBS		(PULSE_NUM_STEPS * STEP_NUM_CBS)

#define NUM_CHANNELS			(sizeof(ChannelGpioPin) / sizeof(ChannelGpioPin[0]))

		// CHANNEL_STEP_TIME_US is timeslot per channel (delay between setting pulse information)
		// With this delay it will arrive at the same channel after PERIOD_TIME.
#define CHANNEL_STEP_TIME_US    (PULSE_PERIOD_US/NUM_CHANNELS)

		// CHANNEL_NUM_STEPS is the maximum number of PULSE_STEP_TIME_US that fit into one gpio
		// channels timeslot. (eg. 250 for a 2500us timeslot with 10us PULSE_WIDTH_INCREMENT)
#define CHANNEL_NUM_STEPS		(CHANNEL_STEP_TIME_US/PULSE_STEP_TIME_US)

		// Min and max channel width settings (used only for controlling user input)
#define CHANNEL_WIDTH_MIN		0
#define CHANNEL_WIDTH_MAX		(CHANNEL_NUM_STEPS - 1)

	typedef struct _SwServoPwmControlData
	{
		__declspec(align(32)) ULONG Step[PULSE_NUM_STEPS];
		BCM_DMA_CB CB[PULSE_NUM_DMA_CBS];
	} SwServoPwmControlData, *PSwServoPwmControlData;


	class SwServoPwm
	{
	public:
		SwServoPwm() :
			CtrlDataPA(NULL),
			CtrlDataVA(nullptr)
		{}

		~SwServoPwm()
		{
			FinalizePrephirals();
		}

		bool Init()
		{
			LogInfo("SwServoPwm Configurations:\n"
				"    Pulse Period: %dus\n"
				"    Pulse Step Time: %dus\n"
				"    Pulse Num Steps: %d\n"
				"    Num Channels: %d\n"
				"    Channel Step Count: %d\n"
				"    Channel Total Steps Time: %dus\n"
				"    Channel Max Width: %d (%dus)\n"
				"    Num DMA CBs: %d\n"
				"    Control Data Byte Size Unaligned/Aligned: %d/%d \n",
				PULSE_PERIOD_US,
				PULSE_STEP_TIME_US,
				PULSE_NUM_STEPS,
				NUM_CHANNELS,
				CHANNEL_NUM_STEPS,
				CHANNEL_STEP_TIME_US,
				CHANNEL_WIDTH_MAX,
				CHANNEL_WIDTH_MAX * PULSE_STEP_TIME_US,
				PULSE_NUM_DMA_CBS,
				sizeof(SwServoPwmControlData),
				ControlDataLength);

			ZeroMemory((PVOID)GpioPinChannel, sizeof(GpioPinChannel));

			LogInfo("Channel to GPIO pin mapping:");
			for (int channel = 0; channel < NUM_CHANNELS; ++channel)
			{
				int pin = ChannelGpioPin[channel];
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

		void SetChannelWidth(int channel, int widthUS)
		{
			if (channel >= NUM_CHANNELS)
			{
				LogError("Invalid CH%d", channel);
				return;
			}

			int width = widthUS / PULSE_STEP_TIME_US;

			// Clamp width to the allowed range
			if (width < CHANNEL_WIDTH_MIN)
				width = CHANNEL_WIDTH_MIN;
			else if (width > CHANNEL_WIDTH_MAX)
				width = CHANNEL_WIDTH_MAX;

			ULONG gpioClr0BA = BCM_GPIO_BUS_BASE + ((ULONG)GpioReg->Clear - (ULONG)GpioReg);
			ULONG gpioSet0BA = BCM_GPIO_BUS_BASE + ((ULONG)GpioReg->Set - (ULONG)GpioReg);
			PBCM_DMA_CB channelCB0 = CtrlDataVA->CB + channel * CHANNEL_NUM_STEPS * STEP_NUM_CBS;
			PULONG channelStep = CtrlDataVA->Step + channel * CHANNEL_NUM_STEPS;

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

		void SetServoAngle(int channel, int angle, ServoType type)
		{
			if ((int)type >= (int)ServoUnknown)
			{
				LogError("Invalid servo type %d", type);
				return;
			}

			int width = MapRange(angle, 0, 180, ServoMinMaxRange[type][1], ServoMinMaxRange[type][2]);

			LogInfo("Set Servo @CH%d @GPIO%d -> %d' (%dus)", channel, ChannelGpioPin[channel], angle, width);
			SetChannelWidth(channel, width);
		}

	private:
		ULONG GetControlDataStepBA(int stepIdx)
		{
			int stepOffset = (PBYTE)(CtrlDataVA->Step + stepIdx) - (PBYTE)CtrlDataVA;
			return BCM_CPU_TO_BUS_DRAM_ADDR((ULONG)CtrlDataPA + stepOffset);
		}

		ULONG GetControlDataCbBA(int cbIdx)
		{
			int cbOffset = (PBYTE)(CtrlDataVA->CB + cbIdx) - (PBYTE)CtrlDataVA;
			ULONG ba = BCM_CPU_TO_BUS_DRAM_ADDR((ULONG)CtrlDataPA + cbOffset);
			_ASSERT(!(ba % 32) && "CB BA should be 256-bit (32-byte)aligned");

			return ba;
		}

		bool InitPrephirals()
		{
			LogInfo("Initializing prehirals");

			for (int i = 0; i < NUM_CHANNELS; ++i)
			{
				GpioFuncSelect(ChannelGpioPin[i], BCM_GPIO_FSEL_Output);
				GpioPinWrite(ChannelGpioPin[i], 0);
			}

			LogInfo("All Channels GPIO pins are set to output and asserted LOW");

			// Turn off PWM controller
			WRITE_REGISTER_ULONG(&PwmReg->Control, 0);
			MicroDelay(10);

			if (!StopPwmClock())
				return false;

			// Set PWM clock source
			WRITE_REGISTER_ULONG(&CmReg->PwmControl, BCM_CM_PWM_PSSWD | BCM_PWMCLK_CTL_SRC_PLLD);
			MicroDelay(100);

			// Divisor = 500 = 1MHz Clock
			WRITE_REGISTER_ULONG(&CmReg->PwmDivisor, BCM_CM_PWM_PSSWD | BCM_PWMCLK_DIV_INT(500));
			MicroDelay(100);

			// Enabl PWM clock and wait for busy flag to go high
			WRITE_REGISTER_ULONG(&CmReg->PwmControl, BCM_CM_PWM_PSSWD | BCM_PWMCLK_CTL_ENABLE | BCM_PWMCLK_CTL_SRC_PLLD);
			MicroDelay(100);

			while (!(READ_REGISTER_ULONG(&CmReg->PwmControl) & BCM_PWMCLK_CTL_BUSY));

			LogInfo("PWM clock configured");

			WRITE_REGISTER_ULONG(&PwmReg->Ch1Range, PULSE_STEP_TIME_US);
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

			auto res = MMap::Inst().AllocMap(ControlDataLength);
			CtrlDataPA = res.PhysicalAddress;
			CtrlDataVA = (PSwServoPwmControlData)res.UserAddress;

			if (CtrlDataVA == NULL)
			{
				LogError("D2MAP::AllocMap failed");
				return false;
			}

			LogInfo("Allocated %d byte SwServoPwmControlData block @VA:0x%08x @PA:0x%08x", ControlDataLength, CtrlDataVA, CtrlDataPA);

			ZeroMemory(CtrlDataVA, ControlDataLength);
			for (size_t channel = 0; channel < NUM_CHANNELS; ++channel)
			{
				for (size_t step = 0; step < CHANNEL_NUM_STEPS; ++step)
				{
					CtrlDataVA->Step[(channel * CHANNEL_NUM_STEPS) + step] = 1 << ChannelGpioPin[channel];
				}
			}

			LogInfo("Touched all allocated DMA control blocks physical memory");

			ULONG pwmFifoBA = BCM_PWM_BUS_BASE + ((ULONG)&PwmReg->FifoInput - (ULONG)PwmReg);
			ULONG gpioClr0BA = BCM_GPIO_BUS_BASE + ((ULONG)GpioReg->Clear - (ULONG)GpioReg);

			int currCbIdx = 0;
			ULONG firstCbBA = 0;

			for (int step = 0; step < PULSE_NUM_STEPS; ++step)
			{
				PBCM_DMA_CB cb0VA = CtrlDataVA->CB + currCbIdx;

				cb0VA->TI = BCM_DMA_TI_NO_WIDE_BURSTS | BCM_DMA_TI_WAIT_RESP;
				cb0VA->SOURCE_AD = GetControlDataStepBA(step);
				cb0VA->DEST_AD = gpioClr0BA;
				cb0VA->TXFR_LEN = 4;
				cb0VA->STRIDE = 0;
				cb0VA->NEXTCONBK = GetControlDataCbBA(currCbIdx + 1);

				if (firstCbBA == 0)
				{
					ULONG firstCbOffset = (ULONG)cb0VA - (ULONG)CtrlDataVA;
					firstCbBA = BCM_CPU_TO_BUS_DRAM_ADDR((ULONG)CtrlDataPA + firstCbOffset);
				}

				++currCbIdx;

				PBCM_DMA_CB cb1VA = CtrlDataVA->CB + currCbIdx;
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
			PBCM_DMA_CB cbVA = CtrlDataVA->CB + currCbIdx - 1;
			cbVA->NEXTCONBK = GetControlDataCbBA(0);

			_ASSERT(cbVA->NEXTCONBK == firstCbBA && "Last CB should be linked to first CB");

			return true;
		}

		void FinalizePrephirals()
		{
			LogInfo("Finalizing prephirals");

			WRITE_REGISTER_ULONG(&PwmReg->Control, 0);
			MicroDelay(100);

			(void)StopPwmClock();

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

			for (int i = 0; i < NUM_CHANNELS; ++i)
			{
				GpioFuncSelect(ChannelGpioPin[i], BCM_GPIO_FSEL_Output);
				GpioPinWrite(ChannelGpioPin[i], 0);
			}
		}

	private:
		const ULONG ControlDataLength = ROUNDUP(sizeof(SwServoPwmControlData), 4096);
		ULONG CtrlDataPA;
		PSwServoPwmControlData CtrlDataVA;
	};
}