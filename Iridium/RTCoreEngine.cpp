#include <Windows.h>
#include <Psapi.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <cstdint>
#include <versionhelpers.h>  // 用于 IsWindowsVersionOrGreater
#include "Define.h"
#include "FunctionCall.h"
#include "ntapiCaller.h"

#pragma comment(lib, "ntdll.lib")

struct RTCORE64_MEMORY_READ {
    BYTE Pad0[8];
    DWORD64 Address;
    BYTE Pad1[8];
    DWORD ReadSize;
    DWORD Value;
    BYTE Pad3[16];
};

DWORD64 Ci_Base = 0;
DWORD gCiOffset = 0;

static_assert(sizeof(RTCORE64_MEMORY_READ) == 48, "sizeof RTCORE64_MEMORY_READ must be 48 bytes");

// IOCTL 控制码
#define RTCORE64_MEMORY_READ_CODE 0x80002048
#define RTCORE64_MEMORY_WRITE_CODE 0x8000204c



// 获取内核模块基址
DWORD64 GetKernelModuleBase(const char* moduleName) {
    DWORD bytesNeeded;
    if (!EnumDeviceDrivers(nullptr, 0, &bytesNeeded)) {
        return 0;
    }

    std::vector<LPVOID> driverAddresses(bytesNeeded / sizeof(DWORD64));
    if (!EnumDeviceDrivers(driverAddresses.data(), bytesNeeded, &bytesNeeded)) {
        return 0;
    }

    char baseName[MAX_PATH];
    for (LPVOID address : driverAddresses) {
        if (GetDeviceDriverBaseNameA(reinterpret_cast<LPVOID>(address), baseName, sizeof(baseName))) {
            if (_stricmp(moduleName, baseName) == 0) {
                return (DWORD64)address;
            }
        }
    }

    return 0;
}

/*
DWORD Find_g_CiOptions_RVA(HMODULE hCiModule) {
    if (!hCiModule) return 0;

    BYTE* pCiInitialize = reinterpret_cast<BYTE*>(
        GetProcAddress(hCiModule, "CiInitialize"));
    if (!pCiInitialize) return 0;


    const BYTE callSignature[] = {
        0x4C, 0x8B, 0xCB,   // mov r9, rbx
        0x4C, 0x8B, 0xC7,   // mov r8, rdi
        0x48, 0x8B, 0xD6,   // mov rdx, rsi
        0x8B, 0xCD,         // mov ecx, ebp
        0xE8                // call CipInitialize
    };
    const int callSigSize = sizeof(callSignature);

    for (BYTE* p = pCiInitialize; p < pCiInitialize + 0x220; p++) {
        if (memcmp(p, callSignature, callSigSize) == 0) {
            DWORD callOffset = *reinterpret_cast<DWORD*>(p + callSigSize);


            BYTE* pCipInitialize = p + callSigSize + 4 + callOffset;

            BYTE* pTargetInstr = pCipInitialize + 0x1b; //注意要根据版本号适配


            if (pTargetInstr[0] == 0x89 && pTargetInstr[1] == 0x0D) {
                DWORD relOffset = *reinterpret_cast<DWORD*>(pTargetInstr + 2);
                BYTE* pCiOptions = pTargetInstr + 6 + relOffset;
                return static_cast<DWORD>(pCiOptions - reinterpret_cast<BYTE*>(hCiModule));
            }
        }
    }
    return 0;
}

*/

BYTE* ReadExternalCiOptionsOffsetHotFix(LPCSTR path,DWORD* size) {
    dbgout("ReadExternalCiOptionsOffsetHotFix() was called,path=%s\n", path);
    int len = 0;
    LPCSTR ver = ReadConfigString(path, "IridiumHotFix", "Version");

    if (!StrCompare(GetVersionProc().String, ver)) {
        dbgout("Hotfix %s isn't suitable for current windows version!\n", path);
        return 0;
    }

    LPCSTR hotfix_type = ReadConfigString(path, "IridiumHotFix", "hotfix_type");

    if (!StrCompare("hotfix.cioptions.offset", hotfix_type)) {
        dbgout("Hotfix %s isn't suitable for current status!\n", path);
        return 0;
    }

    LPCSTR sha = ReadConfigString(path, "IridiumHotFix", "DataSummary");

    WriteConfigString(path, "IridiumHotFix", "DataSummary", "00000000"); //暂时重置方便计算sha值

    LPCSTR file_data = File_ReadFileFull(path);

    WriteConfigString(path, "IridiumHotFix", "DataSummary", sha);

    char cd_sha1[16] = { 0 }, cd_sha2[16] = { 0 };
    memcpy(cd_sha1, sha, 8); //强制截断字符串

    LPCSTR sha2 = CalculateSHA256Hex(file_data, strlen(file_data));
    memcpy(cd_sha2, sha2, 8);

    dbgout("File %s offered ds=%s,calcuated ds=%s\n", path, cd_sha1, cd_sha2);

    if (!StrCompare(cd_sha1, cd_sha2)) {
        dbgout("Invalid hotfix file %s,ignoring...\n", path);
        return 0;
    }

    DWORD val = ReadConfigDword(path, "IridiumHotFix", "value");
    LPCSTR Cip = ReadConfigString(path, "IridiumHotFix", "CipInitializeSignature");

    char msgbuf[256] = { 0 };
    sprintf_s(msgbuf, "在%s中找到了适合的CiOptions补丁,要使用该补丁中的数据吗?\nCipInitialize签名:%s", path, Cip);
    if (MessageBoxA(GetConsoleWindow(), msgbuf, "提示", MB_ICONINFORMATION | MB_YESNO) == IDYES) {
        *size = len;
        return HexStringToBytesDynamic(Cip, &len);
    }   

    return 0;
}


DWORD GetCiOptionsOffset() {

    // 加载 ci.dll
    HMODULE hCiModule = LoadLibraryExW(L"CI.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!hCiModule) {
        return 0;
    }

    // 获取 CiInitialize 函数地址
    BYTE* pCiInitialize = reinterpret_cast<BYTE*>(GetProcAddress(hCiModule, "CiInitialize"));
    if (!pCiInitialize) {
        FreeLibrary(hCiModule);
        dbgout("Failed to find CiInitialize.\n");
        return 0;
    }

    // 根据操作系统版本选择不同的特征码
    BYTE* pCallInstr = nullptr;
    DWORD callSigSize = 0;

    VERSION_INFO vi = GetVersionProc();

    BYTE* hotfixSignature = 0;
    DWORD hotfixCallsize = 0;
    LPCSTR buffer = 0;
    DWORD ext_cnt = EnumFiles(".\\", "ext", &buffer, 0);
    BOOL usingExternal = 0;
    if (ext_cnt > 0) {
        dbgout("Found %d external data file.\n", ext_cnt);

        const char* current = buffer;
        for (DWORD i = 0; i < ext_cnt; i++) {
            
            hotfixSignature = ReadExternalCiOptionsOffsetHotFix(current, &hotfixCallsize);
            if (hotfixSignature && hotfixCallsize) {
                usingExternal = 1;
            }
            current += strlen(current) + 1; // 移动到下一个路径
        }

        
    }

    dbgout("Looking for g_CiOptions address...\n");

    BYTE* universalSignature = 0;

    if (usingExternal) {
        universalSignature = hotfixSignature;
        callSigSize = hotfixCallsize;
    }
    else {
        if (vi.dwMajor == 10 && vi.dwBuild < 22000) {
            // Windows 10 的特征码
            BYTE win10Signature[] = {
                0x4C, 0x8B, 0xCB,   // mov r9, rbx
                0x4C, 0x8B, 0xC7,   // mov r8, rdi
                0x48, 0x8B, 0xD6,   // mov rdx, rsi
                0x8B, 0xCD,         // mov ecx, ebp
                0xE8                // call CipInitialize
            };
            callSigSize = sizeof(win10Signature);
            universalSignature = win10Signature;

        }
        else {
            // Windows 11 的特征码
            BYTE win11Signature[] = {
                0x48, 0x8B, 0xD6,       // mov rdx, rsi
                0x8B, 0xCD,             // mov ecx, ebp
                0xE8                    // call CipInitialize
            };
            callSigSize = sizeof(win11Signature);
            universalSignature = win11Signature;
        }
    }

    // 在 CiInitialize 中搜索特征码
    for (BYTE* p = pCiInitialize; p < pCiInitialize + 0x220; p++) {
        if (memcmp(p, universalSignature, callSigSize) == 0) {
            pCallInstr = p;
            break;
        }
    }


    if (!pCallInstr) {
        FreeLibrary(hCiModule);
        dbgout("CipInitialize call signature not found.\n");
        return 0;
    }

    // 解析 CipInitialize 地址
    // call 指令后的 4 字节是相对偏移
    DWORD callOffset = *reinterpret_cast<DWORD*>(pCallInstr + callSigSize);
    BYTE* pCipInitialize = pCallInstr + callSigSize + 4 + callOffset;

    // 在 CipInitialize 中搜索 g_CiOptions 写入指令
    // 目标指令: mov cs:g_CiOptions, ecx (操作码 89 0D)
    BYTE* pTargetInstr = nullptr;

    // 根据操作系统版本选择不同的搜索范围
    DWORD searchRange = (vi.dwMajor == 10 && vi.dwBuild < 22000) ? 0x50 : 0x100;

    // 在 CipInitialize 函数开头搜索目标指令
    for (BYTE* p = pCipInitialize; p < pCipInitialize + searchRange; p++) {
        if (p[0] == 0x89 && p[1] == 0x0D) {
            pTargetInstr = p;
            break;
        }
    }

    if (!pTargetInstr) {
        FreeLibrary(hCiModule);
        dbgout("g_CiOptions signature not found.\n");
        return 0;
    }

    // 解析 g_CiOptions 地址
    // 89 0D 后的 4 字节是相对偏移
    DWORD relOffset = *reinterpret_cast<DWORD*>(pTargetInstr + 2);
    BYTE* pCiOptions = pTargetInstr + 6 + relOffset;  // 6 = 2(操作码) + 4(偏移)

    // 计算 RVA (相对虚拟地址)
    DWORD rva = static_cast<DWORD>(pCiOptions - reinterpret_cast<BYTE*>(hCiModule));

    FreeLibrary(hCiModule);
    dbgout("g_CiOptions RVA: CI.DLL!+0x%x\n", rva);

    if (WriteConfigInt(Cfg, "System", "CI.dll:g_CiOptions", rva)) {
        dbgout("Cached g_CiOptions.\n");
    }

    return rva;
}

// 读取内核内存
DWORD ReadKernelDWORD(HANDLE hDevice, DWORD64 address) {
    RTCORE64_MEMORY_READ readOp = { 0 };
    readOp.Address = address;
    readOp.ReadSize = sizeof(DWORD);

    DWORD bytesReturned;

    DWORD r = 0;
    if (!DeviceIoControl(hDevice, RTCORE64_MEMORY_READ_CODE,
        &readOp, sizeof(readOp),
        &readOp, sizeof(readOp),
        &r, nullptr)) {
        dbgout("Read Kernel Memory by RTCore64 failed.\n");
        return 0;
    }

   
    return readOp.Value;
}


// 写入内核内存
bool WriteKernelDWORD(HANDLE hDevice, DWORD64 address, DWORD value) {
    RTCORE64_MEMORY_READ writeOp = { 0 };
    writeOp.Address = address;
    writeOp.ReadSize = sizeof(DWORD);
    writeOp.Value = value;

    DWORD r = 0;
    if (!DeviceIoControl(hDevice, RTCORE64_MEMORY_WRITE_CODE,
        &writeOp, sizeof(writeOp),
        &writeOp, sizeof(writeOp),
        &r, nullptr)) {
        dbgout("Write Kernel Memory by RTCore64 failed.\n");
        return 0;
    }
    return true;
}

#define drv "IridiumKMDHelper"

DWORD64 addr = 0;
DWORD v = 0;

DWORD __RTCoreInit(LPCSTR IrDrvPath, LPCSTR HelperDrvPath) {
    dbgout("__RTCoreInit() was called.\n");

    DWORD ciinit = 0;

    //简要判断一下偏移是否有效 正常人不会手痒改数据!!!

    if (ReadConfigBoolean(Cfg, "System", "CacheOffsets")) {
        LPCSTR ver = ReadConfigString(Cfg, "System", "OSVersion");
        if (!StrCompare(ver, GetVersionProc().String)) {
            dbgout("Invalid os version from the config,overwriting...\n");
            WriteConfigString(Cfg, "System", "OSVersion", GetVersionProc().String);
            writeci:
            ciinit = GetCiOptionsOffset();
            WriteConfigInt(Cfg, "System", "CI.dll:g_CiOptions", ciinit);
            if (!ciinit) {
                return 2;
            }
        }
        else {
            if (ReadConfigDword(Cfg, "System", "CI.dll:g_CiOptions") == 0) {
                goto writeci;
            }

            ciinit = ReadConfigDword(Cfg, "System", "CI.dll:g_CiOptions");
            dbgout("CI.dll:g_CiOptions is using config.\n");
        }

    }
    else {
        ciinit = GetCiOptionsOffset();
        if (!ciinit) {
            return 2;
        }


    }

    DWORD64 ciBase = GetKernelModuleBase("CI.dll");
    if (!ciBase) {
        return 3;
    }

    if (ciBase == 0) {
        dbgout("Failed to get g_CiOptions offset.\n");
        printA("获取g_CiOptions偏移失败!\n", 3);
        return 7;
    }
    Ci_Base = ciBase;
    gCiOffset = ciinit;
    addr = ciBase + ciinit;

    dbgout("g_CiOptions address is 0x%p\n", addr);

    if (!InstallService(drv, "Iridium Kernel Mode Driver Helper", cur_irhelpDrv, "Iridium内核模式驱动辅助服务", SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START, SERVICE_ERROR_CRITICAL)) {
        DeleteFileA(IrDrvPath);
        DeleteFileA(HelperDrvPath);
        return 4;
    }

    //启动服务

    if (!LaunchService(drv)) {
        UnInstallService(drv);
        return 5;
    }

    HANDLE hDrv = CreateFileA("\\\\.\\RTCore64", FILE_ALL_ACCESS, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hDrv == INVALID_HANDLE_VALUE) {
        printA("无法初始化Iridium_DriverHelper.\n", 3);
        StopService(drv);
        UnInstallService(drv);
        DeleteFileA(IrDrvPath);
        DeleteFileA(HelperDrvPath);
        return 6;

    }

    v = ReadKernelDWORD(hDrv, addr);

    if (v < 0 || v>17000) {
        dbgout("g_CiOptions value may not correct!!!\n");
        char buf[384] = { 0 };
        sprintf_s(buf, "警告:读取到的g_CiOptions值不在预期范围(0~17000)内,其值为%d.这可能是因为偏移量计算错误导致读取到了错误的地址.如果你不认为这是个错误,请点击是继续进行接下来的操作.", v);
        if (MessageBoxA(GetConsoleWindow(), buf, "警告", MB_ICONWARNING | MB_YESNO) == IDNO) {
            StopService(drv);
            UnInstallService(drv);
            DeleteFileA(IrDrvPath);
            DeleteFileA(HelperDrvPath);
            return 9;
        }
    }

    dbgout("g_CiOptions value is %d\n", v);

    if (WriteKernelDWORD(hDrv, addr, 0)) {
        dbgout("DSE Disabled.\n");
    }

    WriteConfigInt(Cfg, "System", "CI.dll:g_CiOptions", ciinit);

    CloseHandle(hDrv);
    return 0;

}


DWORD __RTCoreUninit(LPCSTR HelperDrvPath) {
    dbgout("__RTCoreUninit() was called.\n");
    HANDLE hDrv = CreateFileA("\\\\.\\RTCore64", FILE_ALL_ACCESS, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hDrv == INVALID_HANDLE_VALUE) {
        StopService(drv);
        UnInstallService(drv);
        return 0;

    }

    BOOL r = 1;
    r |= WriteKernelDWORD(hDrv, addr, v);

    if (r) {
        dbgout("DSE Enabled.\n");
    }

    CloseHandle(hDrv);
    r |= StopService(drv);
    r |= UnInstallService(drv);
    DeleteFileA(HelperDrvPath);
    return !r;
}