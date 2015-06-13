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
UCHAR
READ_REGISTER_NOFENCE_UCHAR (
    _In_ _Notliteral_ volatile UCHAR *Register
    )
{

    return ReadUCharNoFence(Register);
}

__forceinline
USHORT
READ_REGISTER_NOFENCE_USHORT (
    _In_ _Notliteral_ volatile USHORT *Register
    )
{

    return ReadUShortNoFence(Register);
}

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
READ_REGISTER_NOFENCE_BUFFER_UCHAR (
    _In_reads_(Count) _Notliteral_ volatile UCHAR *Register,
    _Out_writes_all_(Count) PUCHAR Buffer,
    _In_ ULONG Count
    )
{

    volatile UCHAR *registerBuffer =  Register;
    PUCHAR readBuffer = Buffer;
    ULONG readCount;

    for (readCount = Count; readCount--; readBuffer++, registerBuffer++) {
        *readBuffer = ReadUCharNoFence(registerBuffer);
    }


    return;
}

__forceinline
VOID
READ_REGISTER_NOFENCE_BUFFER_USHORT (
    _In_reads_(Count) _Notliteral_ volatile USHORT *Register,
    _Out_writes_all_(Count) PUSHORT Buffer,
    _In_ ULONG Count
    )
{
    volatile USHORT *registerBuffer =  Register;
    PUSHORT readBuffer = Buffer;
    ULONG readCount;

    for (readCount = Count; readCount--; readBuffer++, registerBuffer++) {
        *readBuffer = ReadUShortNoFence(registerBuffer);
    }

    return;
}

__forceinline
VOID
READ_REGISTER_NOFENCE_BUFFER_ULONG (
    _In_reads_(Count) _Notliteral_ volatile ULONG *Register,
    _Out_writes_all_(Count) PULONG Buffer,
    _In_ ULONG Count
    )
{
    volatile ULONG *registerBuffer =  Register;
    PULONG readBuffer = Buffer;
    ULONG readCount;

    for (readCount = Count; readCount--; readBuffer++, registerBuffer++) {
        *readBuffer = ReadULongNoFence(registerBuffer);
    }
    return;
}

__forceinline
VOID
WRITE_REGISTER_NOFENCE_UCHAR (
    _In_ _Notliteral_ volatile UCHAR *Register,
    _In_ UCHAR Value
    )
{

    WriteUCharNoFence(Register, Value);

    return;
}

__forceinline
VOID
WRITE_REGISTER_NOFENCE_USHORT (
    _In_ _Notliteral_ volatile USHORT *Register,
    _In_ USHORT Value
    )
{

    WriteUShortNoFence(Register, Value);

    return;
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
VOID
WRITE_REGISTER_NOFENCE_BUFFER_UCHAR (
    _Out_writes_(Count) _Notliteral_ volatile UCHAR *Register,
    _In_reads_(Count) PUCHAR Buffer,
    _In_ ULONG Count
    )
{

    volatile UCHAR *registerBuffer = Register;
    PUCHAR writeBuffer = Buffer;
    ULONG writeCount;

    for (writeCount = Count; writeCount--; writeBuffer++, registerBuffer++) {
        WriteUCharNoFence(registerBuffer, *writeBuffer);
    }

    return;
}

__forceinline
VOID
WRITE_REGISTER_NOFENCE_BUFFER_USHORT (
    _Out_writes_(Count) _Notliteral_ volatile USHORT *Register,
    _In_reads_(Count) PUSHORT Buffer,
    _In_ ULONG Count
    )
{

    volatile USHORT *registerBuffer = Register;
    PUSHORT writeBuffer = Buffer;
    ULONG writeCount;

    for (writeCount = Count; writeCount--; writeBuffer++, registerBuffer++) {
        WriteUShortNoFence(registerBuffer, *writeBuffer);
    }

    return;
}

__forceinline
VOID
WRITE_REGISTER_NOFENCE_BUFFER_ULONG (
    _Out_writes_(Count) _Notliteral_ volatile ULONG *Register,
    _In_reads_(Count) PULONG Buffer,
    _In_ ULONG Count
    )
{

    volatile ULONG *registerBuffer = Register;
    PULONG writeBuffer = Buffer;
    ULONG writeCount;

    for (writeCount = Count; writeCount--; writeBuffer++, registerBuffer++) {
        WriteULongNoFence(registerBuffer, *writeBuffer);
    }

    return;
}

__forceinline
VOID
REGISTER_FENCE (
    VOID
    )
{

    _DataSynchronizationBarrier();
}

__forceinline
UCHAR
READ_REGISTER_UCHAR (
    _In_ _Notliteral_ volatile UCHAR *Register
    )
{
    UCHAR Value;

    _DataSynchronizationBarrier();
    Value = READ_REGISTER_NOFENCE_UCHAR(Register);

    return Value;
}

__forceinline
USHORT
READ_REGISTER_USHORT (
    _In_ _Notliteral_ volatile USHORT *Register
    )
{
    USHORT Value;

    _DataSynchronizationBarrier();
    Value = READ_REGISTER_NOFENCE_USHORT(Register);

    return Value;
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
READ_REGISTER_BUFFER_UCHAR (
    _In_reads_(Count) _Notliteral_ volatile UCHAR *Register,
    _Out_writes_all_(Count) PUCHAR Buffer,
    _In_ ULONG Count
    )
{
    _DataSynchronizationBarrier();
    READ_REGISTER_NOFENCE_BUFFER_UCHAR(Register, Buffer, Count);

    return;
}

__forceinline
VOID
READ_REGISTER_BUFFER_USHORT (
    _In_reads_(Count) _Notliteral_ volatile USHORT *Register,
    _Out_writes_all_(Count) PUSHORT Buffer,
    _In_ ULONG Count
    )
{

    _DataSynchronizationBarrier();
    READ_REGISTER_NOFENCE_BUFFER_USHORT(Register, Buffer, Count);

    return;
}

__forceinline
VOID
READ_REGISTER_BUFFER_ULONG (
    _In_reads_(Count) _Notliteral_ volatile ULONG *Register,
    _Out_writes_all_(Count) PULONG Buffer,
    _In_ ULONG Count
    )
{

    _DataSynchronizationBarrier();
    READ_REGISTER_NOFENCE_BUFFER_ULONG(Register, Buffer, Count);

    return;
}

__forceinline
VOID
WRITE_REGISTER_UCHAR (
    _In_ _Notliteral_ volatile UCHAR *Register,
    _In_ UCHAR Value
    )
{

    _DataSynchronizationBarrier();
    WRITE_REGISTER_NOFENCE_UCHAR(Register, Value);

    return;
}

__forceinline
VOID
WRITE_REGISTER_USHORT (
    _In_ _Notliteral_ volatile USHORT *Register,
    _In_ USHORT Value
    )
{

    _DataSynchronizationBarrier();
    WRITE_REGISTER_NOFENCE_USHORT(Register, Value);

    return;
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

__forceinline
VOID
WRITE_REGISTER_BUFFER_UCHAR (
    _Out_writes_(Count) _Notliteral_ volatile UCHAR *Register,
    _In_reads_(Count) PUCHAR Buffer,
    _In_ ULONG Count
    )
{

    _DataSynchronizationBarrier();
    WRITE_REGISTER_NOFENCE_BUFFER_UCHAR(Register, Buffer, Count);

    return;
}

__forceinline
VOID
WRITE_REGISTER_BUFFER_USHORT (
    _Out_writes_(Count) _Notliteral_ volatile USHORT *Register,
    _In_reads_(Count) PUSHORT Buffer,
    _In_ ULONG Count
    )
{

    _DataSynchronizationBarrier();
    WRITE_REGISTER_NOFENCE_BUFFER_USHORT(Register, Buffer, Count);

    return;
}

__forceinline
VOID
WRITE_REGISTER_BUFFER_ULONG (
    _Out_writes_(Count) _Notliteral_ volatile ULONG *Register,
    _In_reads_(Count) PULONG Buffer,
    _In_ ULONG Count
    )
{

    _DataSynchronizationBarrier();
    WRITE_REGISTER_NOFENCE_BUFFER_ULONG(Register, Buffer, Count);

    return;
}

#ifdef __cplusplus
}
#endif

#endif // defined(_ARM_)
#endif // _REGACCESS_H_
