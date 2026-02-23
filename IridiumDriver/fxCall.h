#pragma once
#include <ntddk.h>

//Main.cpp
void IridiumUnload(PDRIVER_OBJECT DriverObject);

//Init.cpp
NTSTATUS __DrvInit(PDRIVER_OBJECT DriverObject);

//Routines.cpp
NTSTATUS DriverCreate(PDEVICE_OBJECT pdo, PIRP Irp);
NTSTATUS DriverClose(PDEVICE_OBJECT pdo, PIRP Irp);
NTSTATUS DriverDeviceControl(PDEVICE_OBJECT pdo, PIRP Irp);

//NtOperation.cpp
PVOID GetNtBaseAddress();
NTSTATUS KernelMapFile(UNICODE_STRING FileName, HANDLE* phFile, HANDLE* phSection, PVOID* ppBaseAddress);
ULONG64 GetAddressFromFunction(UNICODE_STRING DllFileName, PCHAR pszFunctionName);

//MessageFunction.cpp
NTSTATUS VerifyPackageSha256(PVOID data, ULONG datalen, PVOID contsha);
NTSTATUS UnicodeStringToCharArray(PUNICODE_STRING dst, char* src);

//FileIO.cpp
NTSTATUS DeleteFile(PUNICODE_STRING pwzFileName);
NTSTATUS UnlockFile(PUNICODE_STRING pwzFileName);

//Process.cpp
NTSTATUS ForceCloseHandle(PEPROCESS Process, ULONG64 HandleValue);
PEPROCESS LookupProcess(HANDLE Pid);

//Protect.cpp
NTSTATUS __ProtectClientByRegisterCallback();
void UnRegNotifyCallbacks(void);