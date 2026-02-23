#include <ntifs.h>
#include <bcrypt.h>
#pragma comment (lib,"cng.lib")

/*
注意 在项目->连接器->输入中添加了C:\Program Files (x86)\Windows Kits\10\Lib\10.0.19041.0\km\x64\cng.lib库
*/

// 计算数据的SHA256哈希值
NTSTATUS ComputeSHA256(
    _In_reads_bytes_(len) PVOID data,  // 输入数据指针
    _In_ ULONG len,                    // 数据长度
    _Out_writes_bytes_(32) PUCHAR buf   // 输出缓冲区（32字节）
)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BCRYPT_ALG_HANDLE hAlgorithm = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    PUCHAR hashObject = NULL;
    ULONG hashObjectSize = 0;
    ULONG resultSize = 0;

    do {
        // 验证输入参数
        if (!data || len == 0 || !buf) {
            status = STATUS_INVALID_PARAMETER;
            KdPrint(("Invalid parameters!\n"));
            break;
        }

        // 打开SHA256算法提供者
        status = BCryptOpenAlgorithmProvider(
            &hAlgorithm,
            BCRYPT_SHA256_ALGORITHM,
            NULL,
            BCRYPT_PROV_DISPATCH);
        if (!NT_SUCCESS(status)) {
            KdPrint(("BCryptOpenAlgorithmProvider failed: 0x%X\n", status));
            break;
        }

        // 获取哈希对象大小
        status = BCryptGetProperty(
            hAlgorithm,
            BCRYPT_OBJECT_LENGTH,
            (PUCHAR)&hashObjectSize,
            sizeof(ULONG),
            &resultSize,
            0);
        if (!NT_SUCCESS(status) || resultSize != sizeof(ULONG)) {
            KdPrint(("BCryptGetProperty(OBJECT_LENGTH) failed: 0x%X\n", status));
            break;
        }

        // 分配哈希对象内存
        hashObject = (PUCHAR)ExAllocatePool2(
            POOL_FLAG_NON_PAGED ,
            hashObjectSize,
            'HshC');
        if (!hashObject) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            KdPrint(("Memory allocation failed\n"));
            break;
        }

        // 创建哈希对象
        status = BCryptCreateHash(
            hAlgorithm,
            &hHash,
            hashObject,
            hashObjectSize,
            NULL,
            0,
            0);
        if (!NT_SUCCESS(status)) {
            KdPrint(("BCryptCreateHash failed: 0x%X\n", status));
            break;
        }

        // 计算哈希值
        status = BCryptHashData(
            hHash,
            (PUCHAR)data,
            len,
            0);
        if (!NT_SUCCESS(status)) {
            KdPrint(("BCryptHashData failed: 0x%X\n", status));
            break;
        }

        // 获取最终哈希结果
        status = BCryptFinishHash(
            hHash,
            buf,
            32,
            0);
        if (!NT_SUCCESS(status)) {
            KdPrint(("BCryptFinishHash failed: 0x%X\n", status));
            break;
        }

        status = STATUS_SUCCESS;
    } while (0);

    // 资源清理
    if (hHash) {
        BCryptDestroyHash(hHash);
    }

    if (hashObject) {
        ExFreePoolWithTag(hashObject, 'HshC');
    }

    if (hAlgorithm) {
        BCryptCloseAlgorithmProvider(hAlgorithm, 0);
    }

    return status;
}

// 将32字节哈希值转换为64字符十六进制字符串（加1字节结尾符）
NTSTATUS Sha256ToHexString(
    _In_reads_bytes_(32) const PUCHAR hash,   // 32字节哈希输入
    _Out_writes_bytes_(65) PUCHAR hexString    // 65字节输出缓冲区
)
{
    if (!hash || !hexString) {
        return STATUS_INVALID_PARAMETER;
    }

    // 十六进制字符表
    static const CHAR hexDigits[] = "0123456789abcdef";

    // 将每个字节转换为两个十六进制字符
    for (ULONG i = 0; i < 32; i++) {
        hexString[i * 2] = hexDigits[(hash[i] >> 4) & 0x0F];
        hexString[i * 2 + 1] = hexDigits[hash[i] & 0x0F];
    }

    // 添加字符串结束符
    hexString[64] = '\0';

    return STATUS_SUCCESS;
}

NTSTATUS VerifyPackageSha256(PVOID data, ULONG datalen, PVOID contsha) {
    NTSTATUS Stat = 0;

    if (!MmIsAddressValid(data) || !MmIsAddressValid(contsha)) return 0;

    PUCHAR binHash = (PUCHAR)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        32,  // SHA256输出是32字节
        'HshT');

    if (binHash == nullptr) {
        Stat = STATUS_INSUFFICIENT_RESOURCES;
        return Stat;
    }
    RtlZeroMemory(binHash, 32);

    Stat = ComputeSHA256(
        data,  // 数据起始地址
        datalen,  // 计算到sha256字段之前的长度
        binHash);  // 输出缓冲区（32字节）

    if (!NT_SUCCESS(Stat)) {
        ExFreePool(binHash);
        return Stat;
    }

    UCHAR hexString[65] = { 0 };

    Stat = Sha256ToHexString(binHash, hexString);
    ExFreePool(binHash);

    if (!NT_SUCCESS(Stat)) {
        return Stat;
    }

    // 比较计算出的哈希和提供的哈希（比较64个字符）
    if (memcmp(hexString, contsha, 64) != 0) {
        return STATUS_FAIL_CHECK;
    }
    else {
        return -1001;
    }
}

NTSTATUS  UnicodeStringToCharArray(PUNICODE_STRING dst, char* src){
    NTSTATUS stat = 0;
    ANSI_STRING string;
    if (dst->Length > 260){
        return STATUS_INVALID_BUFFER_SIZE;
    }

    stat = RtlUnicodeStringToAnsiString(&string, dst, TRUE);
    strcpy(src, string.Buffer);
    RtlFreeAnsiString(&string);

    return stat;
}