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

#include "common.h"
#include "WinWireKm.h"

namespace WinWire
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

		~MMap()
		{
			if (hD2Map != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hD2Map);
			}
		}

		static MMap& Inst()
		{
			static MMap inst;
			return inst;
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

		MMap() :
			hD2Map(INVALID_HANDLE_VALUE)
		{}

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
}