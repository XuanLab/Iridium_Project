#include <Windows.h>
#include <iostream>
#include "FunctionCall.h"
#include "Define.h"

DWORD CreateProcessNew(LPCSTR path, LPCSTR cmdline) {
	STARTUPINFOA si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	
	BOOL b = CreateProcessA(path, (LPSTR)cmdline, 0, 0, 0, CREATE_NEW_CONSOLE, 0, 0, &si, &pi);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return pi.dwProcessId;
}

BOOL SetPrivilege(HANDLE hProcess, LPCWSTR privilege) {
    dbgout("SetPrivilege() was called. handle=0x%x,acquire=%ws :", hProcess, privilege);
    HANDLE hToken;
    LUID sedebugnameValue;
    TOKEN_PRIVILEGES tkp;

    // 打开当前进程的访问令牌
    if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    // 查找调试权限的LUID值
    if (!LookupPrivilegeValueW(NULL, privilege, &sedebugnameValue)) {
        CloseHandle(hToken);
        return false;
    }

    // 设置权限结构
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = sedebugnameValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // 调整令牌权限
    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL)) {
        CloseHandle(hToken);
        return false;
    }

    // 检查是否成功
    if (GetLastError() != ERROR_SUCCESS) {
        CloseHandle(hToken);
        return false;
    }

    // 关闭句柄
    CloseHandle(hToken);
    printf("OK\n");
    return true;
}

BOOL RtlAdjustCurrentPrivilege(DWORD Privilege) {
    dbgout("RtlAdjustCurrentPrivilege() was called,requested privilege:%d\n", Privilege);
    HMODULE hNtdll = LoadLibraryA("ntdll.dll");
    if (!hNtdll) {
        dbgout("RtlAdjustCurrentPrivilege() failed with code 0x%x.\n", GetLastError());
        return 0;
    }

    _RtlAdjustPrivilege RtlAdjustPrivilege = (_RtlAdjustPrivilege)GetProcAddress(hNtdll, "RtlAdjustPrivilege");
    BOOLEAN bEnabled = 0;
    NTSTATUS Stat = RtlAdjustPrivilege(Privilege, 1, 0, &bEnabled);
    NTSTATUS Stat2 = RtlAdjustPrivilege(Privilege, 1, 1, &bEnabled);
    return bEnabled;
}

DWORD InjectDllToProcess(DWORD Pid = GetCurrentProcessId()) {
    dbgout("InjectDllToProcess() was called,target prcoess id=0x%x\n", Pid);

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, Pid);
    if (hProc == INVALID_HANDLE_VALUE && Config->LoadDriver) {
        dbgout("Failed to open 0x%x with code 0x%x,trying to use driver to raise handle privilege...\n", Pid, GetLastError());
    }

    return 0;
}
