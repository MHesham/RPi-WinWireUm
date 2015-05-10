#pragma once

#include "common.h"
#include "d2map.h"
#include "regaccess.h"
#include "bcmdma.h"
#include "bcmpwm.h"
#include "bcmcm.h"
#include "bcmgpio.h"

namespace Wi2Pi
{
	struct MMapResult
	{
		ULONG PhysicalAddress;
		PVOID UserAddress;
	};

	struct ArrowResult
	{
		LONGLONG ThrowTimeUs;
		LONGLONG ThrowBackTimeUs;
	};

	class MMap
	{
	public:
		MMap() :
			hD2Map(INVALID_HANDLE_VALUE)
		{}

		~MMap()
		{
			if (hD2Map != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hD2Map);
			}
		}

		MMapResult AllocMap(ULONG length)
		{
			LogVerbose("MMap::AllocMap physicalAddress=0x%08x length=%x", nullptr, length);

			return MMapInternal(nullptr, length);
		}

		MMapResult Map(PVOID physicalAddress, ULONG length)
		{
			LogVerbose("MMap::Map physicalAddress=0x%08x length=%x", physicalAddress, length);

			return MMapInternal(physicalAddress, length);
		}

		ArrowResult Arrow()
		{
			ArrowResult res;

			res.ThrowTimeUs = 0;
			res.ThrowBackTimeUs = 0;

			D2MAP_ARROW_OUTPUT_BUFFER dmapOutputBuffer;
			LARGE_INTEGER throwStartTick;
			LARGE_INTEGER throwBackEndTick;
			DWORD bytesReturned;

			(void)QueryPerformanceCounter(&throwStartTick);

			if (!DeviceIoControl(
				GetHandle(),
					IOCTL_D2MAP_ARROW,
					NULL,
					0,
					&dmapOutputBuffer,
					sizeof(dmapOutputBuffer),
					&bytesReturned,
					nullptr))
			{
				LogError("DeviceIoControl failed, Error=%d", GetLastError());
				return res;
			}

			(void)QueryPerformanceCounter(&throwBackEndTick);

			res.ThrowTimeUs = ((dmapOutputBuffer.HitTime.QuadPart - throwStartTick.QuadPart) * 1000000) / HpcFreq.QuadPart;
			res.ThrowBackTimeUs = ((throwBackEndTick.QuadPart - dmapOutputBuffer.ThrowBackTime.QuadPart) * 1000000) / HpcFreq.QuadPart;

			return res;
		}

	private:

		MMapResult MMapInternal(PVOID physicalAddress, ULONG length)
		{
			LogVerbose("MMap::MMapInternal physicalAddress=0x%08x length=%x", physicalAddress, length);

			MMapResult res;
			res.PhysicalAddress = 0;
			res.UserAddress = nullptr;

			D2MAP_MMAP_INPUT_BUFFER dmapInputBuffer;
			D2MAP_MMAP_OUTPUT_BUFFER dmapOutputBuffer;

			dmapInputBuffer.PhysicalAddress = physicalAddress;
			dmapInputBuffer.Length = length;

			DWORD bytesReturned;
			if (!DeviceIoControl(
				GetHandle(),
				IOCTL_D2MAP_MMAP,
				&dmapInputBuffer,
				sizeof(dmapInputBuffer),
				&dmapOutputBuffer,
				sizeof(dmapOutputBuffer),
				&bytesReturned,
				nullptr))
			{
				LogError("DeviceIoControl failed, Error=%d", GetLastError());
				return res;
			}

			res.PhysicalAddress = (ULONG)dmapOutputBuffer.PhysicalAddress;
			res.UserAddress = dmapOutputBuffer.UserAddress;

			return res;
		}

		HANDLE GetHandle()
		{
			if (hD2Map == INVALID_HANDLE_VALUE)
			{
				hD2Map = CreateFile(
					D2MAP_USER_PATH,
					GENERIC_WRITE | GENERIC_READ,
					0,      // exclusive
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

				if (hD2Map == INVALID_HANDLE_VALUE)
				{
					LogError("CreateFile failed, Error=%d", GetLastError());
					return NULL;
				}
			}

			return hD2Map;
		}

		HANDLE hD2Map;
	};

	static MMap MemMap;

	bool MapPrephirals()
	{
		PwmReg = (PBCM_PWM_REGISTERS)MemMap.Map((PVOID)BCM_PWM_CPU_BASE, BCM_PWM_REG_LEN).UserAddress;

		if (!PwmReg)
		{
			LogError("D2Map::MMap PWM registers failed");
			return false;
		}

		LogInfo("PWM Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", PwmReg, BCM_PWM_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_PWM_CPU_BASE));

		GpioReg = (PBCM_GPIO_REGISTERS)MemMap.Map((PVOID)BCM_GPIO_CPU_BASE, BCM_GPIO_REG_LEN).UserAddress;

		if (!GpioReg)
		{
			LogError("D2Map::MMap GPIO registers failed");
			return false;
		}

		LogInfo("GPIO Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", GpioReg, BCM_GPIO_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_GPIO_CPU_BASE));

		DmaReg = (PBCM_DMA_REGISTERS)MemMap.Map((PVOID)BCM_DMA_CPU_BASE, BCM_DMA_REG_LEN).UserAddress;

		if (!DmaReg)
		{
			LogError("D2Map::MMap DMA registers failed");
			return false;
		}

		LogInfo("DMA Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", DmaReg, BCM_DMA_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_DMA_CPU_BASE));

		CmReg = (PBCM_CM_REGISTERS)MemMap.Map((PVOID)BCM_CM_CPU_BASE, BCM_CM_REG_LEN).UserAddress;

		if (!CmReg)
		{
			LogError("D2Map::MMap CM registers failed");
			return false;
		}

		LogInfo("Clock Manager Direct Access Acquired @VA:0x%08x @PA:0x%08x @BA:0x%08x", CmReg, BCM_CM_CPU_BASE, BCM_CPU_TO_BUS_PERIPH_ADDR(BCM_CM_CPU_BASE));

		return true;
	}

	volatile bool GlobalShutdownFlag = false;
	HANDLE GlobalShutdownEvt;

	static bool Init()
	{
		GlobalShutdownEvt = CreateEvent(NULL, TRUE, FALSE, NULL);

		(void)QueryPerformanceCounter(&Wi2PiT0);
		(void)QueryPerformanceFrequency(&HpcFreq);

		HpcPerdiodNs = 1000000000.0 / HpcFreq.QuadPart;
		HpcMicroNumTicks = (double)HpcFreq.QuadPart / 1000000.0;

		LogInfo("Initializing Wi2Pi Library");

		if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
		{
			LogError("SetPriorityClass REALTIME_PRIORITY_CLASS failed, %d", GetLastError());
			return false;
		}

		LogInfo("HPC Info: Freq=%lldHz, Period=%fns 1MicroNumTicks=%f", HpcFreq.QuadPart, HpcPerdiodNs, HpcMicroNumTicks);

#ifdef DEBUG_TIMING

		LARGE_INTEGER t0, t1, t2, t3;
		QueryPerformanceCounter(&t0);
		QueryPerformanceCounter(&t1);
		QueryPerformanceCounter(&t2);
		QueryPerformanceCounter(&t3);
		LogInfo("t0=%lld\nt1=%lld\nt2=%lld\nt3=%lld\n",
			t0.QuadPart, t1.QuadPart, t2.QuadPart, t3.QuadPart);

		LogInfo("t1-t0=%lld~%lldns\nt2-t1=%lld~%lldns\nt3-t2=%lld~%lldns",
			t1.QuadPart - t0.QuadPart, ((t1.QuadPart - t0.QuadPart) * 1000000000) / HpcFreq.QuadPart,
			t2.QuadPart - t1.QuadPart, ((t2.QuadPart - t1.QuadPart) * 1000000000) / HpcFreq.QuadPart,
			t3.QuadPart - t2.QuadPart, ((t3.QuadPart - t2.QuadPart) * 1000000000) / HpcFreq.QuadPart);

		LogInfo("\nStart Testing HPC");
		MicroDelay(1);
		MicroDelay(10);
		MicroDelay(100);
		MicroDelay(1000);
		LogInfo("End Testing HPC\n");

		ArrowResult arrowTestRes;
		arrowTestRes.ThrowBackTimeUs = 0;
		arrowTestRes.ThrowTimeUs = 0;
		const int ArrowNumSamples = 100;

		for (int i = 0; i < ArrowNumSamples; ++i)
		{
			ArrowResult currRes = MemMap.Arrow();

			LogVerbose(
				"Sample%d: User->Kernel=%lldus, Kernel->User=%lldus",
				i,
				currRes.ThrowTimeUs,
				currRes.ThrowBackTimeUs);

			arrowTestRes.ThrowTimeUs += currRes.ThrowTimeUs;
			arrowTestRes.ThrowBackTimeUs += currRes.ThrowBackTimeUs;
		}

		arrowTestRes.ThrowTimeUs /= (LONGLONG)ArrowNumSamples;
		arrowTestRes.ThrowBackTimeUs /= (LONGLONG)ArrowNumSamples;

		LogInfo(
			"KMDF IOCTL Timing: User->Kernel=%lldus, Kernel->User=%lldus, User<->Kernel= %lldus over %d samples",
			arrowTestRes.ThrowTimeUs,
			arrowTestRes.ThrowBackTimeUs,
			arrowTestRes.ThrowTimeUs + arrowTestRes.ThrowBackTimeUs,
			ArrowNumSamples);

#endif

		if (!MapPrephirals())
		{
			LogError("Failed to map prephirals registers");
			return false;
		}

		return true;
	}

	void Deinit()
	{
		GlobalShutdownFlag = true;
		SetEvent(GlobalShutdownEvt);
	}
}