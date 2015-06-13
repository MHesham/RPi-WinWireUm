//
// BCM PWM Controller
//

#pragma once

#include "RPi2\bcm.h"
#include "RPi2\bcmgpio.h"

//
// BCM PWM controller registers.
// BCM : Broadcom PWM Controller
//

#define BCM_PWM_BASE_OFFSET			0x0020C000
#define BCM_PWM_CPU_BASE			(BCM_CPU_PERIPH_BASE + BCM_PWM_BASE_OFFSET)
#define BCM_PWM_BUS_BASE			(BCM_BUS_PERIPH_BASE + BCM_PWM_BASE_OFFSET)
#define BCM_PWM_REG_LEN				0x28

#define BCM_PWM_DMA_DREQ			5 // Datasheet-61

#define BCM_PWM_CH1_GPIO_PIN		BCM_GPIO18
#define BCM_PWM_CH1_GPIO_ALTF		BCM_GPIO_FSEL_Alt5

//
// PWM Control (CTL)
//

#define BCM_PWM_REG_CTL_PWEN1   (1<<0)
#define BCM_PWM_REG_CTL_MODE1   (1<<1)
#define BCM_PWM_REG_CTL_RPTL1   (1<<2)
#define BCM_PWM_REG_CTL_SBIT1   (1<<3)
#define BCM_PWM_REG_CTL_POLA1   (1<<4)
#define BCM_PWM_REG_CTL_USEF1   (1<<5)
#define BCM_PWM_REG_CTL_CLRF1   (1<<6)
#define BCM_PWM_REG_CTL_MSEN1   (1<<7)
#define BCM_PWM_REG_CTL_PWEN2   (1<<8)
#define BCM_PWM_REG_CTL_MODE2   (1<<9)
#define BCM_PWM_REG_CTL_RPTL2   (1<<10)
#define BCM_PWM_REG_CTL_SBIT2   (1<<11)
#define BCM_PWM_REG_CTL_POLA2   (1<<12)
#define BCM_PWM_REG_CTL_USEF2   (1<<13)
#define BCM_PWM_REG_CTL_MSEN2   (1<<15)

//
// PWM Status (STA)
//

#define BCM_PWM_REG_STA_FULL1   (1<<0)
#define BCM_PWM_REG_STA_EMPT1   (1<<1)
#define BCM_PWM_REG_STA_WERR1   (1<<2)
#define BCM_PWM_REG_STA_RERR1   (1<<3)
#define BCM_PWM_REG_STA_GAPO1   (1<<4)
#define BCM_PWM_REG_STA_GAPO2   (1<<5)
#define BCM_PWM_REG_STA_GAPO3   (1<<6)
#define BCM_PWM_REG_STA_GAPO4   (1<<7)
#define BCM_PWM_REG_STA_BERR    (1<<8)
#define BCM_PWM_REG_STA_STA1    (1<<9)
#define BCM_PWM_REG_STA_STA2    (1<<10)
#define BCM_PWM_REG_STA_STA3    (1<<11)
#define BCM_PWM_REG_STA_STA4    (1<<12)

//
// PWM DMA Configuration (DMAC)
//

#define BCM_PWM_REG_DMAC_DREQ_SHIFT     0
#define BCM_PWM_REG_DMAC_DREQ_MASK      (0xFF<<BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_SHIFT    8
#define BCM_PWM_REG_DMAC_PANIC_MASK     (0xFF<<BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_ENAB           (1<<31)

#define BCM_PWM_REG_DMAC_DREQ_0         (0 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_1         (1 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_2         (2 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_3         (3 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_4         (4 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_5         (5 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_6         (6 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_7         (7 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_8         (8 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_9         (9 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_10        (10 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_11        (11 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_12        (12 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_13        (13 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_14        (14 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_15        (15 << BCM_PWM_REG_DMAC_DREQ_SHIFT)
#define BCM_PWM_REG_DMAC_DREQ_16        (16 << BCM_PWM_REG_DMAC_DREQ_SHIFT)

#define BCM_PWM_REG_DMAC_PANIC_0         (0 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_1         (1 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_2         (2 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_3         (3 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_4         (4 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_5         (5 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_6         (6 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_7         (7 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_8         (8 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_9         (9 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_10        (10 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_11        (11 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_12        (12 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_13        (13 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_14        (14 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_15        (15 << BCM_PWM_REG_DMAC_PANIC_SHIFT)
#define BCM_PWM_REG_DMAC_PANIC_16        (16 << BCM_PWM_REG_DMAC_PANIC_SHIFT)


namespace WinWire {
	namespace RPi2
	{
#include <pshpack4.h>
		typedef struct _BCM_PWM_REGISTERS
		{
			// using the chatty register names because
			// the short register names are not self explanatory
			ULONG Control;             // CTL
			ULONG Status;              // STA
			ULONG DmaConfig;		   // DMAC
			ULONG Reserved0;		   // 
			ULONG Ch1Range;			   // RNG1
			ULONG Ch1Data;             // DAT1
			ULONG FifoInput;		   // FIF1
			ULONG Reserved1;           //
			ULONG Ch2Range;            // RNG2
			ULONG Ch2Data;             // DAT2
		} BCM_PWM_REGISTERS, *PBCM_PWM_REGISTERS;
#include <poppack.h>

		static PBCM_PWM_REGISTERS PwmReg;

		void DumpPwmRegisters()
		{
			LogInfo(
				"\nDumping PWM Registers\n"
				"    Control =    0x%08x\n"
				"    Status =     0x%08x\n"
				"    DMA Config = 0x%08x\n"
				"    CH1 Range =  0x%08x\n"
				"    CH1 Data =   0x%08x\n"
				"    FIFO Input = 0x%08x\n"
				"    CH2 Range =  0x%08x\n"
				"    CH2 Data =   0x%08x\n",
				READ_REGISTER_ULONG(&PwmReg->Control),
				READ_REGISTER_ULONG(&PwmReg->Status),
				READ_REGISTER_ULONG(&PwmReg->DmaConfig),
				READ_REGISTER_ULONG(&PwmReg->Ch1Range),
				READ_REGISTER_ULONG(&PwmReg->Ch1Data),
				READ_REGISTER_ULONG(&PwmReg->FifoInput),
				READ_REGISTER_ULONG(&PwmReg->Ch2Range),
				READ_REGISTER_ULONG(&PwmReg->Ch2Data));
		}
	}
}