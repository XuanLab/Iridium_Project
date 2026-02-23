#include <ntifs.h>
#include "fxCall.h"
#include "Define.h"

OFFSET_DESCRIPTOR offset_desc = { 0 };

NTSTATUS __InitOffsets(VOID);

NTSTATUS __EnvironmentCheck() {

    DbgPrint(("[IRIDIUM] Nt base address:0x%p.\n"), GetNtBaseAddress());

    return STATUS_SUCCESS;
}

NTSTATUS __DrvInit(PDRIVER_OBJECT DriverObject) {
    NTSTATUS Status = STATUS_FAILED_DRIVER_ENTRY; //初始状态
    RtlInitUnicodeString(&DeviceName, L"\\Device\\Iridium"); //初始化设备
    RtlInitUnicodeString(&SymbolLink, L"\\??\\Iridium"); //初始化符号链接

    IoDeleteSymbolicLink(&SymbolLink); //首先删除一次符号链接

    Status = IoCreateDevice(DriverObject, 64, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DriverObject->DeviceObject); //创建设备
    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice(DriverObject->DeviceObject);
        DbgPrint(("[IRIDIUM] Device create failed with status:0x%x.\n"), Status);
        return Status;
    }
    DbgPrint("[IRIDIUM] Device created.\n");

    Status = IoCreateSymbolicLink(&SymbolLink, &DeviceName); //创建符号链接
    if (!NT_SUCCESS(Status)) {
        IoDeleteSymbolicLink(&SymbolLink);
        IoDeleteDevice(DriverObject->DeviceObject);
        DbgPrint(("[IRIDIUM] Symbol link create failed with status:0x%x.\n"), Status);
        return Status;
    }
    DbgPrint("[IRIDIUM] Symbol link created.\n");

    Status = __EnvironmentCheck();
    if (!NT_SUCCESS(Status)) {

    }

    Status = __InitOffsets();
    if (!NT_SUCCESS(Status)) {
        
    }

    return Status;
} //Init Driver Basically

NTSTATUS __InitOffsets(VOID){

    DbgPrint("[IRIDIUM] Trying to obtain offsets.\n");

    //EPROCESS.UniqueProcessId START

    PEPROCESS curProc = PsGetCurrentProcess();
    BYTE* start = (BYTE*)curProc;
    ULONG curPid = (ULONG)PsGetCurrentProcessId();
    ULONG offset = 0;

    KIRQL oldIrql = KeRaiseIrqlToDpcLevel();

    for (int i = 0; i < 0x1000; i += sizeof(ULONG)) {
        __try {
            ULONG Value = *(ULONG*)(start + i);
            if (Value == curPid) {
                offset_desc.EPROCESS_UniqueProcessId = i;
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // 跳过不可读的内存区域
            continue;
        }
    }

    KeLowerIrql(oldIrql);


    //EPROCESS.UniqueProcessId END




    return STATUS_SUCCESS;
}


NTSTATUS __InitSSDTHooks() {
    
    
    return STATUS_SUCCESS;
}