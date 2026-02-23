#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(suppress : 4996)
#include <Windows.h>
#include <VersionHelpers.h>
#include <stdio.h>
#include "Define.h"
#include "FunctionCall.h"
#include "Structures.h"

typedef void(__stdcall* RtlGetNtVersionNumbersCall)(DWORD*, DWORD*, DWORD*);

VERSION_INFO GetVersionProc(void) {
    BYTE* sharedUserData = (BYTE*)0x7FFE0000;
    ULONG uMajorVer, uMinorVer, uBuildNum;
    memcpy(&uMajorVer, (sharedUserData + 0x26c), sizeof(ULONG));
    memcpy(&uMinorVer, (sharedUserData + 0x270), sizeof(ULONG));
    memcpy(&uBuildNum, (sharedUserData + 0x260), sizeof(ULONG));
    VERSION_INFO Ver = { 0 };
    char* Buffer = (char*)malloc(128);
    sprintf(Buffer, "%d-%d-%d", *(ULONG*)(sharedUserData + 0x26c), *(ULONG*)(sharedUserData + 0x270), *(ULONG*)(sharedUserData + 0x260));
    Ver.dwMajor = uMajorVer;
    Ver.dwMinor = uMinorVer;
    Ver.dwBuild = uBuildNum;
    if (uBuildNum >= 22000) Ver.dwMajor = 11;
    Ver.String = Buffer;
    return Ver;
}

char* GetSystemTime() {
    SYSTEMTIME SysTime;
    GetLocalTime(&SysTime);
    char* Buffer = (char*)malloc(128);
    sprintf(Buffer, "%02d:%02d:%02d.%03d", SysTime.wHour, SysTime.wMinute, SysTime.wSecond, SysTime.wMilliseconds);
    return Buffer;
}

char* GetSystemTimeFull() {
    SYSTEMTIME SysTime;
    GetLocalTime(&SysTime);
    char* Buffer = (char*)malloc(256);
    sprintf(Buffer, "%04d-%02d-%02d %02d:%02d:%02d.%03d", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute, SysTime.wSecond, SysTime.wMilliseconds);
    return Buffer;
}

BOOL EnableNoSignedDriverLoad() {
    BOOL Stat = true;
    Stat = Stat & (DWORD)ShellExecuteW(0, L"runas", L"BCDEdit.exe", L"/debug on", NULL, SW_SHOWNORMAL);
    Stat = Stat & (DWORD)ShellExecuteW(0, L"runas", L"BCDEdit.exe", L"/set testsigning on", NULL, SW_SHOWNORMAL);
    Stat = Stat & (DWORD)ShellExecuteW(0, L"runas", L"BCDEdit.exe", L"/set nointegritychecks on", NULL, SW_SHOWNORMAL);
    return Stat;
}

BOOL DisableNoSignedDriverLoad() {
    BOOL Stat = true;
    Stat = Stat & (DWORD)ShellExecuteW(0, L"runas", L"BCDEdit.exe", L"/debug off", NULL, SW_SHOWNORMAL);
    Stat = Stat & (DWORD)ShellExecuteW(0, L"runas", L"BCDEdit.exe", L"/set testsigning off", NULL, SW_SHOWNORMAL);
    Stat = Stat & (DWORD)ShellExecuteW(0, L"runas", L"BCDEdit.exe", L"/set nointegritychecks off", NULL, SW_SHOWNORMAL);
    return Stat;
}

LUID RtlConvertUlongToLuid(
    _In_ ULONG Ulong
)
{
    LUID tempLuid;

    tempLuid.LowPart = Ulong;
    tempLuid.HighPart = 0;

    return tempLuid;
}


BOOL IsRunAsAdmin() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }
    TOKEN_ELEVATION eve;
    DWORD len = 0;
    if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == false) {
        return false;
    }
    CloseHandle(hToken);
    if (len == sizeof(eve)) {
        return eve.TokenIsElevated;
    }
    return false;
}

//注意:Nt系列函数必须在__ApiInit()成功后才能调用 否则调用常规api


NTSTATUS NtDeviceIoControlFile(HANDLE hDev, HANDLE Event, PVOID apc, PVOID apcc, PIO_STATUS_BLOCK io, ULONG code, PVOID bin, ULONG sbin, PVOID bout, ULONG sbout) {
    DWORD ret = -1;
    DWORD s = 0;
    if (!pNtDeviceIoControlFile) {
        s = DeviceIoControl(hDev, code, bin, sbin, bout, sbout, &ret, 0);
    }
    else {
        s = pNtDeviceIoControlFile(hDev, Event, apc, apcc, io, code, bin, sbin, bout, sbout);
    }

    dbgout("external_NtDeviceIoControl type=%d status=0x%x\n", ret == -1, s);

    return s;
}


NTSTATUS NtCreateFile(PHANDLE h, ACCESS_MASK acc, POBJECT_ATTRIBUTES obj, PIO_STATUS_BLOCK io, PLARGE_INTEGER allocsize, ULONG fileatt, ULONG shareacc, ULONG createdispo, ULONG createopt, PVOID buf, ULONG buflen) {
    DWORD p1, p2, p3;
    HANDLE h2 = 0;

    if (h == nullptr) return 1;

    if (!pNtCreateFile) {
        h2 = CreateFileW(obj->ObjectName->Buffer, acc, shareacc, 0, createdispo, createopt, 0);
        memcpy(h, &h2, sizeof(HANDLE));
        CloseHandle(h2);
    }

    DWORD s = pNtCreateFile(h, acc, obj, io, allocsize, fileatt, shareacc, createdispo, createopt, buf, buflen);

    dbgout("external_NtCreateFile type=%d status=0x%x\n", (DWORD)h2 == -1, s);

    return s;
}