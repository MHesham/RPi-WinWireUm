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

#include <winioctl.h>
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

    class FxKm
    {
    public:

        ~FxKm()
        {
            if (hWinWireKm != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hWinWireKm);
            }
        }

        static FxKm& Inst()
        {
            static FxKm inst;
            return inst;
        }

        MMapResult AllocMap(ULONG length)
        {
            LogVerbose("FxKm::AllocMap physicalAddress=0x%08x length=%x", nullptr, length);

            return MMapInternal(nullptr, length);
        }

        MMapResult Map(PVOID physicalAddress, ULONG length)
        {
            LogVerbose("FxKm::Map physicalAddress=0x%08x length=%x", physicalAddress, length);

            return MMapInternal(physicalAddress, length);
        }

        ArrowResult Arrow()
        {
            ArrowResult res;

            res.ThrowTimeUs = 0;
            res.ThrowBackTimeUs = 0;

            WINWIREKM_ARROW_OUTPUT_BUFFER ob;
            LARGE_INTEGER throwStartTick;
            LARGE_INTEGER throwBackEndTick;
            DWORD bytesReturned;

            (void)QueryPerformanceCounter(&throwStartTick);

            if (!DeviceIoControl(
                GetHandle(),
                IOCTL_WINWIREKM_ARROW,
                NULL,
                0,
                &ob,
                sizeof(ob),
                &bytesReturned,
                nullptr))
            {
                LogError("DeviceIoControl failed, Error=%d", GetLastError());
                return res;
            }

            (void)QueryPerformanceCounter(&throwBackEndTick);

            res.ThrowTimeUs = ((ob.HitTime.QuadPart - throwStartTick.QuadPart) * 1000000) / HpcFreq.QuadPart;
            res.ThrowBackTimeUs = ((throwBackEndTick.QuadPart - ob.ThrowBackTime.QuadPart) * 1000000) / HpcFreq.QuadPart;

            return res;
        }

    private:

        FxKm() :
            hWinWireKm(INVALID_HANDLE_VALUE)
        {}

        MMapResult MMapInternal(PVOID physicalAddress, ULONG length)
        {
            LogVerbose("FxKm::MMapInternal physicalAddress=0x%08x length=%x", physicalAddress, length);

            MMapResult res;
            res.PhysicalAddress = 0;
            res.UserAddress = nullptr;

            WINWIREKM_MMAP_INPUT_BUFFER ib;
            WINWIREKM_MMAP_OUTPUT_BUFFER ob;

            ib.PhysicalAddress = physicalAddress;
            ib.Length = length;

            DWORD bytesReturned;
            if (!DeviceIoControl(
                GetHandle(),
                IOCTL_WINWIREKM_MMAP,
                &ib,
                sizeof(ib),
                &ob,
                sizeof(ob),
                &bytesReturned,
                nullptr))
            {
                LogError("DeviceIoControl failed, Error=%d", GetLastError());
                return res;
            }

            res.PhysicalAddress = (ULONG)ob.PhysicalAddress;
            res.UserAddress = ob.UserAddress;

            return res;
        }

        HANDLE GetHandle()
        {
            if (hWinWireKm == INVALID_HANDLE_VALUE)
            {
                hWinWireKm = CreateFile(
                    WINWIREKM_USER_PATH,
                    GENERIC_WRITE | GENERIC_READ,
                    0,      // exclusive
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

                if (hWinWireKm == INVALID_HANDLE_VALUE)
                {
                    LogError("CreateFile failed, Error=%d", GetLastError());
                    return NULL;
                }
            }

            return hWinWireKm;
        }

        HANDLE hWinWireKm;
    };
}