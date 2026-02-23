#include <ntifs.h>
#include "Define.h"
#include "fxCall.h"

NTSTATUS DeleteFile(PUNICODE_STRING pwzFileName)
{
    PEPROCESS pCurEprocess = NULL;
    KAPC_STATE kapc = { 0 };
    OBJECT_ATTRIBUTES fileOb;
    HANDLE hFile = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK iosta;
    PDEVICE_OBJECT DeviceObject = NULL;
    PVOID pHandleFileObject = NULL;


    // 判断中断等级不大于0
    if (KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (pwzFileName->Buffer == NULL || pwzFileName->Length <= 0)
    {
        return STATUS_INTERNAL_ERROR;
    }

    __try
    {
        // 读取当前进程的EProcess
        pCurEprocess = IoGetCurrentProcess();

        // 附加进程
        KeStackAttachProcess(pCurEprocess, &kapc);

        // 初始化结构
        InitializeObjectAttributes(&fileOb, pwzFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        // 文件系统筛选器驱动程序 仅向指定设备对象下面的筛选器和文件系统发送创建请求。
        status = IoCreateFileSpecifyDeviceObjectHint(&hFile,
            SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_READ_DATA,
            &fileOb,
            &iosta,
            NULL,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            0,
            0,
            CreateFileTypeNone,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK,
            DeviceObject);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        // 在对象句柄上提供访问验证，如果可以授予访问权限，则返回指向对象的正文的相应指针。
        status = ObReferenceObjectByHandle(hFile, 0, 0, 0, &pHandleFileObject, 0);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        // 镜像节对象设置为0
        ((PFILE_OBJECT)(pHandleFileObject))->SectionObjectPointer->ImageSectionObject = 0;

        // 删除权限打开
        ((PFILE_OBJECT)(pHandleFileObject))->DeleteAccess = 1;

        // 调用删除文件API
        status = ZwDeleteFile(&fileOb);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    _finally
    {
        if (pHandleFileObject != NULL)
        {
            ObDereferenceObject(pHandleFileObject);
            pHandleFileObject = NULL;
        }
        KeUnstackDetachProcess(&kapc);

        if (hFile != NULL || hFile != (PVOID)-1)
        {
            ZwClose(hFile);
            hFile = (PVOID)-1;
        }
    }
    return status;
}

NTSTATUS UnlockFile(PUNICODE_STRING pwzFileName) {

    if (KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        return STATUS_INTERNAL_ERROR;
    }

    DbgPrint("HELLO\n");

    //return 0;

    NTSTATUS Status = 0;
    PVOID Buffer = 0;
    ULONG BufferSize = 0x20000;

    ULONG64 qwHandleCount = 0;
    ULONG64 loopcnt = 0;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO* handletable = 0;
    //HANDLE_INFO HandleInfo[1024] = { 0 };

    ULONG ulProcessID = 0;
    CLIENT_ID cid = { 0 };
    HANDLE hHandle = 0;
    OBJECT_ATTRIBUTES oa = { 0 };
    NTSTATUS ns = 0;
    HANDLE hProcess = 0, hDupObj = 0;
    PVOID BasicInfo = 0, pNameInfo = 0;

    ULONG rtl = 0;

    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, 'ir');
    memset(Buffer, 0, BufferSize);
    // SystemHandleInformation
    Status = ZwQuerySystemInformation(16, Buffer, BufferSize, 0);
    while (Status == STATUS_INFO_LENGTH_MISMATCH){
        ExFreePoolWithTag(Buffer, 'ir');
        BufferSize = BufferSize * 2;
        Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, 'ir');
        memset(Buffer, 0, BufferSize);
        Status = ZwQuerySystemInformation(16, Buffer, BufferSize, 0);
        DbgPrint("[IRIDIUM] Trying to allocate buffer,size=%lld\n", BufferSize);
    }

    if (!NT_SUCCESS(Status)){
        ExFreePoolWithTag(Buffer, 'ir');
        DbgPrint("[IRIDIUM] Failed to allocate buffer for handle table.\n");
        return Status;
    }

    qwHandleCount = ((SYSTEM_HANDLE_INFORMATION*)Buffer)->NumberOfHandles;

    handletable = (SYSTEM_HANDLE_TABLE_ENTRY_INFO*)((SYSTEM_HANDLE_INFORMATION*)Buffer)->Handles;

    for (loopcnt = 0; loopcnt < qwHandleCount; loopcnt++) {
        ulProcessID = (ULONG)handletable[loopcnt].UniqueProcessId;
        cid.UniqueProcess = (HANDLE)ulProcessID;
        cid.UniqueThread = (HANDLE)0;
        hHandle = (HANDLE)handletable[loopcnt].HandleValue;

        InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);

        ns = ZwOpenProcess(&hProcess, PROCESS_DUP_HANDLE, &oa, &cid);

        if (!NT_SUCCESS(ns)){
            continue;
        }

        ns = ZwDuplicateObject(hProcess, hHandle, NtCurrentProcess(), &hDupObj, PROCESS_ALL_ACCESS, 0, DUPLICATE_SAME_ACCESS);
        if (!NT_SUCCESS(ns)){
            continue;
        }

        ZwQueryObject(hDupObj, ObjectBasicInformation, &BasicInfo, sizeof(OBJECT_BASIC_INFORMATION), NULL);

        pNameInfo = ExAllocatePool(PagedPool, 2048);
        RtlZeroMemory(pNameInfo, 2048);

        ns = ZwQueryObject(hDupObj, ObjectNameInformation, pNameInfo, 2048, &rtl);

        DbgPrint("obj=%ws\n", ((POBJECT_NAME_INFORMATION)pNameInfo)->Name.Buffer);

        if (RtlCompareUnicodeString(&((POBJECT_NAME_INFORMATION)pNameInfo)->Name, pwzFileName, TRUE) == 0) {
            PEPROCESS proc = 0;
            if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)handletable[loopcnt].UniqueProcessId, &proc))) {
                DbgPrint("Process %d is using file %ws,closing...\n", handletable[loopcnt].UniqueProcessId, pwzFileName->Buffer);
                Status = ForceCloseHandle(proc, handletable[loopcnt].HandleValue);
                ObDereferenceObject(proc);
            }
        }

        ExFreePool(pNameInfo);
        ZwClose(hDupObj);
        ZwClose(hProcess);
    }

    ExFreePoolWithTag(Buffer, 'ir');
}