#include <Windows.h>
#include <iostream>
#include <string>
#include "FunctionCall.h"
#include "Console.h"
#include "Define.h"
#include "Structures.h"

typedef enum IRIDIUM_OPERATIONS {
    FileIo,
    ProcessHandleOpt
};

typedef enum IRIDIUM_FILE_OPERATIONS {
    FileOperationUnlock,
    FileOperationDelete,
    FileOperationDeleteForce,
    FileOperationRead,
    FileOperationWrite
};

typedef enum IRIDIUM_PROCESS_HANDLE_OPERATIONS {
    HandleModifyAccess
};

typedef struct _IRIDIUM_OPERATION_S {
    ULONG MajorFuncId;
    ULONG MinorFuncId;
    ULONG p1;
    ULONG p2;
    BYTE buf[4096+128];
    ULONG Datalen;
    NTSTATUS Stat;
    char sha256[64]; //结构体的SHA256值
}IRIDIUM_OPERATION_S, * PIRIDIUM_OPERATION_S;

HANDLE AllocateDriver() {
    return CreateFileA("\\\\.\\Iridium", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}


int ConvertToNtPath(LPCWSTR original, LPWSTR buffer, int size) {
    // 增强参数检查
    if (!original || !buffer || size <= 0) {
        return -1;
    }

    // 检查原始路径长度
    size_t originalLen = wcslen(original);
    if (originalLen == 0) {
        return -2;
    }

    // 增强路径格式验证
    if (originalLen < 3 ||
        !iswalpha(original[0]) ||
        original[1] != L':' ||
        original[2] != L'\\') {
        return -2;
    }

    // 构建NT路径 - 使用动态大小计算
    int requiredSize = (int)originalLen + 5; // \??\ + null terminator

    if (requiredSize > size) {
        return requiredSize;
    }

    // 直接构建到目标缓冲区
    int result = swprintf_s(buffer, size, L"\\??\\%s", original);

    if (result == -1) {
        dbgout("ConvertToNtPath: String formatting failed\n");
        return -3;
    }

    return 0;
}

BOOL SendPackageToDriver(ULONG Major, ULONG Minor, ULONG p1, ULONG p2, BYTE* buf, ULONG datalen, NTSTATUS* Stat) {
    IRIDIUM_OPERATION_S ido = { 0 };
    ido.MajorFuncId = Major;
    ido.MinorFuncId = Minor;
    ido.p1 = p1;
    ido.p2 = p2;

    // 安全地复制数据到缓冲区
    if (buf != NULL && datalen > 0) {
        ULONG copySize = (datalen > sizeof(ido.buf)) ? sizeof(ido.buf) : datalen;
        memcpy_s(ido.buf, sizeof(ido.buf), buf, copySize);
        ido.Datalen = copySize;
    }
    else {
        ido.Datalen = 0;
    }

    // 计算SHA256（排除sha256字段本身）
    LPCSTR sha256Result = CalculateSHA256Hex(&ido, (DWORD)offsetof(IRIDIUM_OPERATION_S, sha256));
    if (sha256Result != NULL) {
        memcpy_s(ido.sha256, sizeof(ido.sha256), sha256Result, 64);
    }

    HANDLE h = AllocateDriver();
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DWORD bytesReturned = 0;
    BOOL r = DeviceIoControl(
        h,
        IRIDIUM_COMMON_OPERATION,
        &ido,
        sizeof(ido),
        NULL,
        0,
        &bytesReturned,
        NULL
    );

    if (Stat != NULL) {
        *Stat = ido.Stat;
    }

    CloseHandle(h);
    return r;
}

BOOL DeleteFileForce(LPCWSTR path) {
    dbgout("DeleteFileForce() was called : ");
    if (path == NULL) {
        if (Config->DebugMessage)printf("Invalid path.\n");
        return FALSE;
    }

    if (wcslen(path) == 0) {
        if (Config->DebugMessage)printf("Invalid path.\n");
        return FALSE;
    }

    const int bufferSize = 4096;
    LPWSTR ntPathBuffer = (LPWSTR)malloc(sizeof(WCHAR) * bufferSize);
    if (ntPathBuffer == NULL) {
        if (Config->DebugMessage)printf("Insufficient buffer.\n");
        return FALSE;
    }

    ZeroMemory(ntPathBuffer, sizeof(WCHAR) * bufferSize);

    int convertResult = ConvertToNtPath(path, ntPathBuffer, bufferSize - 1);
    if (convertResult != 0) {
        free(ntPathBuffer);
        if (Config->DebugMessage)printf("Convert to nt path failed.\n");
        return FALSE;
    }

    ULONG dataLength = (ULONG)((wcslen(ntPathBuffer) + 1) * sizeof(WCHAR));
    NTSTATUS stat = 0;

    BOOL result = SendPackageToDriver(
        FileIo,
        FileOperationDeleteForce,
        0,
        0,
        (BYTE*)ntPathBuffer,
        dataLength,
        &stat
    );

    if (Config->DebugMessage)printf("ret=0x%x ntstatus=0x%x\n", GetLastError(), stat);
    free(ntPathBuffer);
    return result;
}

BOOL DrvUnlockFile(LPCWSTR path) {
    dbgout("UnlockFileForce() was called : ");
    if (path == NULL) {
        if (Config->DebugMessage)printf("Invalid path.\n");
        return FALSE;
    }

    if (wcslen(path) == 0) {
        if (Config->DebugMessage)printf("Invalid path.\n");
        return FALSE;
    }

    const int bufferSize = 8192;
    LPWSTR ntPathBuffer = (LPWSTR)malloc(sizeof(WCHAR) * bufferSize);
    if (ntPathBuffer == NULL) {
        if (Config->DebugMessage)printf("Insufficient buffer.\n");
        return FALSE;
    }

    ZeroMemory(ntPathBuffer, sizeof(WCHAR) * bufferSize);

    if (!ConvertDosPathToNtPath(path, ntPathBuffer, bufferSize - 1)) {
        free(ntPathBuffer);
        if (Config->DebugMessage)printf("Convert to nt path failed.\n");
        return FALSE;
    }

    printf("%ws\n", ntPathBuffer);

    ULONG dataLength = (ULONG)((wcslen(ntPathBuffer) + 1) * sizeof(WCHAR));
    NTSTATUS stat = 0;

    BOOL result = SendPackageToDriver(
        FileIo,
        FileOperationUnlock,
        0,
        0,
        (BYTE*)ntPathBuffer,
        dataLength,
        &stat
    );

    if (Config->DebugMessage)printf("ret=0x%x ntstatus=0x%x\n", GetLastError(), stat);
    free(ntPathBuffer);
    return result;
}

BOOL Driver_ModifyHandleAccess(DWORD pid, HANDLE handle, DWORD rg) {
    HANDLE_INFO info = { 0 };
    info.process_id = pid;
    info.handle = (unsigned long long)handle;
    info.access = rg;

    NTSTATUS stat = 0;
    BOOL result = SendPackageToDriver(ProcessHandleOpt, HandleModifyAccess, 0, 0, (BYTE*)&info, sizeof(info), &stat);

    return result;
}
