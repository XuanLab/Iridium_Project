#include <ntifs.h>
#include "Define.h"

PEPROCESS LookupProcess(HANDLE Pid){
    PEPROCESS eprocess = NULL;
    if (NT_SUCCESS(PsLookupProcessByProcessId(Pid, &eprocess))) {
        return eprocess;
    }
    else {
        return NULL;
    }
}


NTSTATUS ForceCloseHandle(PEPROCESS Process, ULONG64 HandleValue){
    HANDLE h = 0;
    KAPC_STATE ks = { 0 };
    OBJECT_HANDLE_FLAG_INFORMATION ohfi = { 0 };

    if (Process == NULL){
        return STATUS_INVALID_PARAMETER;
    }
    // 验证进程是否可读写
    if (!MmIsAddressValid(Process)){
        return STATUS_INVALID_ADDRESS;
    }

    KeStackAttachProcess(Process, &ks);    // 附加到进程
    h = (HANDLE)HandleValue;
    ohfi.Inherit = 0;
    ohfi.ProtectFromClose = 0;

    ObSetHandleAttributes(h, &ohfi, KernelMode);    // 设置句柄为可关闭状态
    ZwClose(h);
    KeUnstackDetachProcess(&ks);    // 脱离附加进程

    return STATUS_SUCCESS;
}