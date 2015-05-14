//
// BCM DMA Controller
//

#pragma once

#include "bcm.h"

#define BCM_DMA_BASE_OFFSET			0x00007000
#define BCM_DMA_CPU_BASE			(BCM_CPU_PERIPH_BASE + BCM_DMA_BASE_OFFSET)
#define BCM_DMA_BUS_BASE			(BCM_BUS_PERIPH_BASE + BCM_DMA_BASE_OFFSET)
#define BCM_DMA_NUM_CHANNELS		15 // We don't count channel 15
#define BCM_DMA_CHANNEL_REG_LEN		0x100
#define BCM_DMA_REG_LEN				((BCM_DMA_NUM_CHANNELS + 1) * BCM_DMA_CHANNEL_REG_LEN) // +1 to include INT_STATUS and ENABLE registers

#define BCM_DMA_REG_CB_LEN			6
#define BCM_DMA_REG_PAD_LEN			55

//
// DMA Control and Status (CS)
//
#define BCM_DMA_CS_ACTIVE						(1<<0)
#define BCM_DMA_CS_END							(1<<1)
#define BCM_DMA_CS_INT							(1<<2)
#define BCM_DMA_CS_PANIC_PRIORITY(X)			((X)<<20)
#define BCM_DMA_CS_PRIORITY(X)					((X)<<16)
#define BCM_DMA_WAIT_FOR_OUTSTADNDING_WRITES	(1<<28)
#define BCM_DMA_CS_ABORT						(1<<30)
#define BCM_DMA_CS_RESET						(1<<31)

//
// DMA Debug (DEBUG)
//
#define BCM_DMA_DEBUG_FIFO_ERROR	(1<<1)
#define BCM_DMA_DEBUG_READ_ERROR	(1<<2)

#define BCM_DMA_TI_WAIT_RESP		(1<<3)
#define BCM_DMA_TI_DEST_DREQ        (1<<6)
#define BCM_DMA_IT_PER_MAP(x)		((x)<<16)
#define BCM_DMA_TI_NO_WIDE_BURSTS	(1<<26)

namespace Wi2Pi
{
#include <pshpack4.h>
	typedef struct _BCM_DMA_CH_REGISTERS
	{
		ULONG ControlAndStatus;
		ULONG ControlBlockAddr;
		ULONG CB[BCM_DMA_REG_CB_LEN];
		ULONG Debug;
		ULONG Pad[BCM_DMA_REG_PAD_LEN];
	} BCM_DMA_CH_REGISTERS, *PBCM_DMA_CH_REGISTERS;

	typedef struct _BCM_DMA_REGISTERS
	{
		ULONG Ch0ControlAndStatus;
		ULONG Ch0ControlBlockAddr;
		ULONG CH0CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch0Debug;
		ULONG CH0Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch1ControlAndStatus;
		ULONG Ch1ControlBlockAddr;
		ULONG CH1CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch1Debug;
		ULONG CH1Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch2ControlAndStatus;
		ULONG Ch2ControlBlockAddr;
		ULONG CH2CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch2Debug;
		ULONG CH2Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch3ControlAndStatus;
		ULONG Ch3ControlBlockAddr;
		ULONG CH3CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch3Debug;
		ULONG CH3Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch4ControlAndStatus;
		ULONG Ch4ControlBlockAddr;
		ULONG CH4CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch4Debug;
		ULONG CH4Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch5ControlAndStatus;
		ULONG Ch5ControlBlockAddr;
		ULONG CH5CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch5Debug;
		ULONG CH5Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch6ControlAndStatus;
		ULONG Ch6ControlBlockAddr;
		ULONG CH6CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch6Debug;
		ULONG CH6Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch7ControlAndStatus;
		ULONG Ch7ControlBlockAddr;
		ULONG CH7CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch7Debug;
		ULONG CH7Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch8ControlAndStatus;
		ULONG Ch8ControlBlockAddr;
		ULONG CH8CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch8Debug;
		ULONG CH8Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch9ControlAndStatus;
		ULONG Ch9ControlBlockAddr;
		ULONG CH9CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch9Debug;
		ULONG CH9Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch10ControlAndStatus;
		ULONG Ch10ControlBlockAddr;
		ULONG CH10CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch10Debug;
		ULONG CH10Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch11ControlAndStatus;
		ULONG Ch11ControlBlockAddr;
		ULONG CH11CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch11Debug;
		ULONG CH11Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch12ControlAndStatus;
		ULONG Ch12ControlBlockAddr;
		ULONG CH12CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch12Debug;
		ULONG CH12Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch13ControlAndStatus;
		ULONG Ch13ControlBlockAddr;
		ULONG CH13CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch13Debug;
		ULONG CH13Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Ch14ControlAndStatus;
		ULONG Ch14ControlBlockAddr;
		ULONG CH14CB[BCM_DMA_REG_CB_LEN];
		ULONG Ch14Debug;
		ULONG CH14Pad[BCM_DMA_REG_PAD_LEN];
		ULONG Pad0[56];
		ULONG InterruptStatus;
		ULONG Pad1[3];
		ULONG Enable;

	} BCM_DMA_REGISTERS, *PBCM_DMA_REGISTERS;

#include <poppack.h>
	//
	// DMA Control Block (needs to be 256bit aligned)
	//

	typedef struct _BCM_DMA_CB
	{
		__declspec(align(32)) ULONG TI;
		ULONG SOURCE_AD;
		ULONG DEST_AD;
		ULONG TXFR_LEN;
		ULONG STRIDE;
		ULONG NEXTCONBK;
		ULONG RSVD0;
		ULONG RSVD1;
	} BCM_DMA_CB, *PBCM_DMA_CB;

	static PBCM_DMA_REGISTERS DmaReg;

	void DumpDmaRegisters()
	{
		LogInfo(
			"\nDumping DMA Registers\n"
			"    Enable =                   0x%08x\n"
			"    CH14 Control And Status =  0x%08x\n"
			"    CH14 Control Block Addr =  0x%08x\n"
			"    CH14 Debug =               0x%08x\n",
			READ_REGISTER_ULONG(&DmaReg->Enable),
			READ_REGISTER_ULONG(&DmaReg->Ch14ControlAndStatus),
			READ_REGISTER_ULONG(&DmaReg->Ch14ControlBlockAddr),
			READ_REGISTER_ULONG(&DmaReg->Ch14Debug));
	}
}