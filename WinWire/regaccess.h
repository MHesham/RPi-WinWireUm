//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Register access definitions copied from wdm.h for usermode.
//

#ifndef _REGACCESS_H_
#define _REGACCESS_H_

#if defined(_ARM_)

//
// I/O space read and write macros.
//
//  The READ/WRITE_REGISTER_* calls manipulate I/O registers in MEMORY space.
//
//  N.B. This implementation assumes that the memory mapped registers
//       have been mapped using the OS concept of uncached memory
//       which is implemented using the ARMv7 strongly ordered memory
//       type.  In addition, the register access is bracketed by a
//       compiler barrier to ensure that the compiler does not
//       re-order the I/O accesses with other accesses and a data
//       synchronization barrier to ensure that any side effects of
//       the access have started (but not necessairly completed).
//
//  The READ/WRITE_PORT_* calls manipulate I/O registers in PORT
//  space.  The ARM architecture doesn't have a seperate I/O space.
//  These operations bugcheck so as to identify incorrect code.
//

#ifdef __cplusplus
extern "C" {
#endif

__forceinline
ULONG
READ_REGISTER_NOFENCE_ULONG (
    _In_ _Notliteral_ volatile ULONG *Register
    )
{

    return ReadULongNoFence(Register);
}

__forceinline
VOID
WRITE_REGISTER_NOFENCE_ULONG (
    _In_ _Notliteral_ volatile ULONG *Register,
    _In_ ULONG Value
    )
{

    WriteULongNoFence(Register, Value);

    return;
}

__forceinline
ULONG
READ_REGISTER_ULONG (
    _In_ _Notliteral_ volatile ULONG *Register
    )
{
    ULONG Value;

    _DataSynchronizationBarrier();
    Value = READ_REGISTER_NOFENCE_ULONG(Register);

    return Value;
}

__forceinline
VOID
WRITE_REGISTER_ULONG (
    _In_ _Notliteral_ volatile ULONG *Register,
    _In_ ULONG Value
    )
{

    _DataSynchronizationBarrier();
    WRITE_REGISTER_NOFENCE_ULONG(Register, Value);

    return;
}

#ifdef __cplusplus
}
#endif

#endif // defined(_ARM_)
#endif // _REGACCESS_H_
