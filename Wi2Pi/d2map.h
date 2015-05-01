//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Module Name:
//
//    d2map.h
//
// Abstract:
//
//    Public interface to D2Map.
//
// Environment:
//
//    user and kernel
//

#define D2MAP_SYMBOLIC_LINK_NAME    L"\\DosDevices\\D2map"
#define D2MAP_USER_PATH             L"\\\\.\\D2map"

//
// Define an Interface Guid so that app can find the device and talk to it.
//
DEFINE_GUID(GUID_DEVINTERFACE_D2Map, 
    0x6519e5e4, 0xed23, 0x4229, 0xb5, 0x6a, 0x59, 0xb6, 0xaf, 0xa4, 0x82, 0x3b);
// {6519E5E4-ED23-4229-B56A-59B6AFA4823B}

#define FILE_DEVICE_DMAP 0xbee

#define IOCTL_D2MAP_MMAP        CTL_CODE(FILE_DEVICE_DMAP, 0x100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_D2MAP_ARROW       CTL_CODE(FILE_DEVICE_DMAP, 0x200, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _D2MAP_MMAP_INPUT_BUFFER
{
	PVOID PhysicalAddress;
	ULONG Length;
} D2MAP_MMAP_INPUT_BUFFER, *PD2MAP_MMAP_INPUT_BUFFER;

typedef struct _D2MAP_MMAP_OUTPUT_BUFFER
{
	PVOID PhysicalAddress;
	PVOID UserAddress;
	ULONG Length;
} D2MAP_MMAP_OUTPUT_BUFFER, *PD2MAP_MMAP_OUTPUT_BUFFER;

typedef struct _D2MAP_ARROW_OUTPUT_BUFFER
{
	LARGE_INTEGER HitTime;
	LARGE_INTEGER ThrowBackTime;
} D2MAP_ARROW_OUTPUT_BUFFER, *PD2MAP_ARROW_OUTPUT_BUFFER;