#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(suppress : 4996)
#include <Windows.h>
#include <stdio.h>
#include <NTSecAPI.h>
#include <Wincrypt.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#include "Console.h"

LPCSTR StrConnect(LPCSTR a, LPCSTR b) {
	if (a == nullptr || b == nullptr) {
		return NULL;
	}
    char* Buffer = (char*)malloc(strlen(a) + strlen(b) + 1);
    if (Buffer == nullptr) return 0;
    RtlZeroMemory(Buffer, strlen(Buffer));
	strcpy(Buffer, a);
	strcat(Buffer, b);
	return Buffer;
}

BOOL StrCompare(LPCSTR a, LPCSTR b) {
	if (a == nullptr || b == nullptr) {
		return NULL;
	}
	return strcmp(a, b) == 0;
}

void StrSep(const char* input, char delimiter, char*** output, int* count) {
    const char* start = input;
    int index = 0;

    // 第一次遍历，计算令牌数量
    while (*input) {
        if (*input == delimiter) {
            (*count)++;
        }
        input++;
    }
    (*count)++; // 最后一个令牌

    // 动态分配 tokens 数组
    *output = (char**)malloc((*count) * sizeof(char*));

    // 重置指针，开始分割字符串
    input = start;
    index = 0;

    while (*input) {
        if (*input == delimiter) {
            int length = input - start;
            (*output)[index] = (char*)malloc((length + 1) * sizeof(char));
            strncpy((*output)[index], start, length);
            (*output)[index][length] = '\0';
            index++;
            start = input + 1;
        }
        input++;
    }

    // 处理最后一个令牌
    if (start != input) {
        int length = input - start;
        (*output)[index] = (char*)malloc((length + 1) * sizeof(char));
        strncpy((*output)[index], start, length);
        (*output)[index][length] = '\0';
    }
}

wchar_t* charToWchar(const char* c) {
    size_t len = strlen(c) + 1;
    wchar_t* wc = new wchar_t[len];
    mbstowcs(wc, c, len);
    return wc;
}

char* TranslateGetLastErrorMsg(DWORD code) {
	LPVOID lpMsgBuf=nullptr;
	if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_CHINESE_SIMPLIFIED, SUBLANG_CHINESE_SIMPLIFIED), (char*)&lpMsgBuf, 0, 0) <= 0) {
		return nullptr;
	}
	char* Buffer = (char*)malloc(strlen((char*)lpMsgBuf));
	memset(Buffer, 0, strlen(Buffer));
	memcpy(Buffer, lpMsgBuf, strlen((char*)lpMsgBuf));
	return Buffer;
}

BOOL __RandomSeedInit(ULONGLONG* Ptr) {
    if (Ptr == nullptr) return false;
    BOOL result = 1;
    BYTE random[256];
    result &= RtlGenRandom(random, sizeof(random));
    memcpy(Ptr, random, sizeof(ULONGLONG));
    return result;
}

char* RandomTextGenerate(size_t length) {
    if (length == 0) return nullptr;

    // 分配内存（包含终止符）
    char* str = new char[length + 1];
    if (!str) return nullptr;
    str[length] = '\0';  // 设置字符串终止符

    HCRYPTPROV hCryptProv = 0;

    // 获取加密服务提供程序
    if (!CryptAcquireContextW(
        &hCryptProv,
        nullptr,
        nullptr,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT | CRYPT_SILENT
    )) {
        delete[] str;
        return nullptr;
    }

    // 逐个生成随机字符
    for (size_t i = 0; i < length; ++i) {
        BYTE randomByte;

        // 生成随机字节
        if (!CryptGenRandom(hCryptProv, sizeof(randomByte), &randomByte)) {
            CryptReleaseContext(hCryptProv, 0);
                delete[] str;
                return nullptr;
        }

        // 映射到英文字母：0-25=A-Z, 26-51=a-z
        randomByte %= 52;  // 52个字母（26大写 + 26小写）

            if (randomByte < 26) {
                str[i] = 'A' + static_cast<char>(randomByte);
            }
            else {
                str[i] = 'a' + static_cast<char>(randomByte - 26);
            }
    }

    // 清理资源
    CryptReleaseContext(hCryptProv, 0);
    return str;
}

DWORD GenerateRandomNumber(DWORD min, DWORD max) {
    // 验证参数有效性
    if (min > max) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return min - 1;
    }

    // 计算范围
    DWORD64 range = static_cast<DWORD64>(max) - min + 1;

    // 处理范围大小为1的情况
    if (range == 1) {
        return min;
    }

    HCRYPTPROV hCryptProv = 0;
    DWORD randomValue = 0;

    // 获取加密服务提供者句柄
    if (!CryptAcquireContextW(
        &hCryptProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        return min - 1; // 返回错误
    }

    int result = min - 1; // 预设错误返回值

    // 处理范围超过32位的情况（需要两次生成）
    if (range > 0xFFFFFFFF) {
        DWORD64 randomLarge = 0;
        if (CryptGenRandom(hCryptProv, sizeof(randomLarge), reinterpret_cast<BYTE*>(&randomLarge))) {
            result = min + static_cast<int>(randomLarge % range);
        }
    }
    // 处理常规范围
    else {
        DWORD range32 = static_cast<DWORD>(range);
        DWORD limit = 0xFFFFFFFF - (0xFFFFFFFF % range32);

        do {
            if (!CryptGenRandom(hCryptProv, sizeof(randomValue), reinterpret_cast<BYTE*>(&randomValue))) {
                break;
            }
        } while (randomValue >= limit);

        if (randomValue < limit) {
            result = min + static_cast<int>(randomValue % range32);
        }
    }

    // 释放加密提供者
    CryptReleaseContext(hCryptProv, 0);
    return result;
}

LPCSTR CalculateSHA256Hex(LPCVOID pData, DWORD dwDataLen) {
    static thread_local char szHexHash[65] = { 0 }; // 64字符+null终止符
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE rgbHash[32]; // SHA-256结果为32字节
    DWORD cbHash = sizeof(rgbHash);
    const char hexDigits[] = "0123456789abcdef";

    // 初始化输出缓冲区
    szHexHash[0] = '\0';

    // 获取加密服务提供程序
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return szHexHash;
    }

    // 创建SHA-256哈希对象
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return szHexHash;
    }

    // 添加数据到哈希
    if (!CryptHashData(hHash, (BYTE*)pData, dwDataLen, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return szHexHash;
    }

    // 获取哈希值
    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return szHexHash;
    }

    // 将二进制哈希转换为十六进制字符串
    for (DWORD i = 0; i < cbHash; i++) {
        szHexHash[i * 2] = hexDigits[rgbHash[i] >> 4];   // 高4位
        szHexHash[i * 2 + 1] = hexDigits[rgbHash[i] & 0xF];  // 低4位
    }
    szHexHash[64] = '\0'; // 确保null终止

    // 清理资源
    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);

    return szHexHash;
}

LPCSTR ResolveTempShortPath(LPCSTR t) {
    if (strstr(t, "$Temp$") != nullptr) {

        LPSTR fn = PathFindFileNameA(t);
        char buf[MAX_PATH] = { 0 };

        char buf2[260] = { 0 };
        GetTempPathA(260, buf2);

        PathCombineA(buf, buf2, fn);
        return buf;
    }

    if (strstr(t, "$CD$") != nullptr) {

    }
    return t;
}


LPCSTR StrRemove(PVOID addr, ULONG len, LPCSTR str) {
    if (addr == nullptr || len == 0 || str == nullptr || *str == '\0') {
        return reinterpret_cast<LPCSTR>(addr);
    }

    ULONG strLen = static_cast<ULONG>(strlen(str));
    if (strLen > len) {
        return reinterpret_cast<LPCSTR>(addr);
    }

    PBYTE data = reinterpret_cast<PBYTE>(addr);
    ULONG writeIndex = 0;

    for (ULONG readIndex = 0; readIndex < len; ) {
        // 检查剩余长度是否足够匹配str
        if (readIndex + strLen <= len) {
            // 使用memcmp比较内存块是否匹配str
            if (memcmp(data + readIndex, str, strLen) == 0) {
                // 匹配成功，跳过strLen字节（即移除str）
                readIndex += strLen;
                continue;
            }
        }

        // 不匹配或剩余长度不足，复制当前字节并移动指针
        if (writeIndex != readIndex) {
            data[writeIndex] = data[readIndex];
        }
        writeIndex++;
        readIndex++;
    }

    // 确保结果以null结尾（如果空间允许）
    if (writeIndex < len) {
        data[writeIndex] = '\0';
    }
    else if (len > 0) {
        // 无空间添加null终止符，尽可能处理（此处选择不添加）
        // 注意：这可能导致字符串未终止，调用方需注意长度
    }

    return reinterpret_cast<LPCSTR>(addr);
}

BOOL IsMemoryReadable(void* address, size_t size) {
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    SIZE_T offset = 0;
    BYTE* start = (BYTE*)address;
    BYTE* end = start + size;

    while (start < end) {
        if (VirtualQuery(start, &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE) {
            return false;
        }

        // 如果页面保护权限为可读，或者可读写，或者可执行读等，则视为可读
        if (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) {
            // 当前页面可读，计算下一个页面
            start += mbi.RegionSize;
        }
        else {
            return false;
        }
    }

    return true;
}


BOOL ConvertDosPathToNtPath(const wchar_t* dosPath, wchar_t* ntPath, DWORD ntPathSize) {
    wchar_t drive[4] = L" :";
    wchar_t device[MAX_PATH];
    DWORD deviceSize = MAX_PATH;

    // 提取驱动器号
    drive[0] = dosPath[0];

    // 查询驱动器对应的设备路径
    if (QueryDosDeviceW(drive, device, deviceSize) == 0) {
        return FALSE;
    }

    // 组合成完整NT路径
    swprintf(ntPath, ntPathSize, L"%s%s", device, dosPath + 2);
    return TRUE;
}

BOOL IsSystemProcess(LPCWSTR str) {
    LPCWSTR sysproc[] = { L"wininit.exe",L"dwm.exe",L"services.exe",L"svchost.exe",L"winlogon.exe",L"csrss.exe",L"lsass.exe",L"smss.exe" };
    for (int i = 0; i < ARRAYSIZE(sysproc); i++) {
        if (lstrcmpW(str, sysproc[i]) == 0) return TRUE;
    }
    return FALSE;
}

BOOL ConvertSystemPathToFullPath(const wchar_t* systemPath, wchar_t* fullPath, DWORD fullPathSize) {
    if (!systemPath || !fullPath) return FALSE;

    fullPath[0] = L'\0';

    // 如果是 \SystemRoot\ 开头的路径
    if (wcsncmp(systemPath, L"\\SystemRoot\\", 12) == 0) {
        // 获取 Windows 目录
        wchar_t windowsDir[MAX_PATH];
        if (GetWindowsDirectoryW(windowsDir, MAX_PATH) == 0) {
            return FALSE;
        }

        // 拼接完整路径
        swprintf_s(fullPath, fullPathSize, L"%s%s", windowsDir, systemPath + 11); // 跳过 "\SystemRoot"
        return TRUE;
    }
}

int HexStringToBytes(LPCSTR hexStr, BYTE* buffer, int bufferSize) {
    if (!hexStr || !buffer || bufferSize <= 0) {
        return 0;
    }

    int len = strlen(hexStr);
    int byteCount = 0;
    int i = 0;

    // 跳过 "0x" 或 "0X" 前缀
    if (len >= 2 && hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X')) {
        i = 2;
    }

    while (i < len && byteCount < bufferSize) {
        // 跳过空格
        if (hexStr[i] == ' ' || hexStr[i] == '\t') {
            i++;
            continue;
        }

        // 确保有足够的字符可以转换
        if (i + 1 >= len) {
            break;  // 奇数个十六进制字符，不完整的最后一个字节
        }

        char hexByte[3] = { hexStr[i], hexStr[i + 1], '\0' };

        // 将十六进制字符串转换为字节
        char* endPtr;
        long byteVal = strtol(hexByte, &endPtr, 16);

        if (endPtr == hexByte || *endPtr != '\0') {
            // 转换失败，跳过这个字节
            i += 2;
            continue;
        }

        buffer[byteCount] = (BYTE)(byteVal & 0xFF);
        byteCount++;
        i += 2;
    }

    return byteCount;
}

BYTE* HexStringToBytesDynamic(LPCSTR hexStr, int* outLength) {
    if (!hexStr || !outLength) return NULL;

    int len = strlen(hexStr);
    // 最大可能的字节数（每2个字符一个字节，但可能有空格）
    int maxBytes = (len + 1) / 2;

    BYTE* buffer = (BYTE*)malloc(maxBytes);
    if (!buffer) return NULL;

    int byteCount = HexStringToBytes(hexStr, buffer, maxBytes);

    *outLength = byteCount;
    return buffer;
}