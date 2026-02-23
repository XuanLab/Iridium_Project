#pragma once
#include <Windows.h>
#include "Structures.h"


extern "C" NTSTATUS NtClose(HANDLE Handle);
extern "C" void Int3Breakpoint();
extern "C" void AnyJump(DWORD64 addr);
extern "C" BOOL CheckDebugger();

typedef NTSTATUS(NTAPI* FNtDeviceIoControlFile)(
    HANDLE           FileHandle,
    HANDLE           Event,
    PVOID            ApcRoutine,
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG            IoControlCode,
    PVOID            InputBuffer,
    ULONG            InputBufferLength,
    PVOID            OutputBuffer,
    ULONG            OutputBufferLength
    );

typedef NTSTATUS(NTAPI*  FNtCreateFile)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
);


