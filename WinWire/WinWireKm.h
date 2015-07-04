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

#define WINWIREKM_SYMBOLIC_LINK_NAME    L"\\DosDevices\\WinWireKm"
#define WINWIREKM_USER_PATH             L"\\\\.\\WinWireKm"

//
// Define an Interface Guid so that app can find the device and talk to it.
//
DEFINE_GUID(GUID_DEVINTERFACE_WinWireKm,
    0x6519e5e4, 0xed23, 0x4229, 0xb5, 0x6a, 0x59, 0xb6, 0xaf, 0xa4, 0x82, 0x3b);
// {6519E5E4-ED23-4229-B56A-59B6AFA4823B}

#define FILE_DEVICE_WINWIREKM 0xbee

#define IOCTL_WINWIREKM_MMAP        CTL_CODE(FILE_DEVICE_WINWIREKM, 0x100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINWIREKM_ARROW       CTL_CODE(FILE_DEVICE_WINWIREKM, 0x200, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _WINWIREKM_MMAP_INPUT_BUFFER
{
    PVOID PhysicalAddress;
    ULONG Length;
} WINWIREKM_MMAP_INPUT_BUFFER, *PWINWIREKM_MMAP_INPUT_BUFFER;

typedef struct _WINWIREKM_MMAP_OUTPUT_BUFFER
{
    PVOID PhysicalAddress;
    PVOID UserAddress;
    ULONG Length;
} WINWIREKM_MMAP_OUTPUT_BUFFER, *PWINWIREKM_MMAP_OUTPUT_BUFFER;

typedef struct _WINWIREKM_ARROW_OUTPUT_BUFFER
{
    LARGE_INTEGER HitTime;
    LARGE_INTEGER ThrowBackTime;
} WINWIREKM_ARROW_OUTPUT_BUFFER, *PWINWIREKM_ARROW_OUTPUT_BUFFER;