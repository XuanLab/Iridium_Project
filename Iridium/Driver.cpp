#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include <Shlwapi.h>
#include <Psapi.h>

#pragma comment (lib,"shlwapi.lib")
#include "FunctionCall.h"
#include "Define.h"

#define drv2 "IridiumKMDHelper"
#define sym "\\\\.\\Iridium"

DWORD __CfgDriver();
BOOL iridium_kmd = 0;

typedef struct INTERNAL_DRV_DATA_SECTION { //Iridium_Driver识别标志
    char recognizer[48] = "IRIDIUM_KERNEL_MODE_DRIVER_DATA_SECTION_1384012";
    DWORD filesize;
    char dic[64] = { 0 };
} DRV_HEADER;

const char END_MARKER[48] = "IRIDIUM_KERNEL_MODE_DRIVER_DATA_SECTION_ENDLAB";

typedef struct INTERNAL_DRV_DATA_SECTION2 { //RTCore64识别标志
    char recognizer[48] = "RTCOR64_KERNEL_MODE_DRIVER_DATA_SECTION_1384012";
    DWORD filesize;
    char dic[64] = { 0 };
} DRV_HEADER2;

const char END_MARKER2[48] = "RTCOR64_KERNEL_MODE_DRIVER_DATA_SECTION_ENDLAB";

LPCSTR cur_irDrv, cur_irhelpDrv;
LPCSTR drv_m_name;

DWORD __ExtractDriver(LPCSTR TargetPath) {
    dbgout("__ExtractDriver() was called.\n");

    // XOR 密钥（必须与嵌入端完全一致）
    static const BYTE xorKey[] = "IRIDIUM_DRIVER_ENCRYPT_KEY";
    static const DWORD xorKeyLen = sizeof(xorKey) - 1;

    LPCSTR pri_irDrv = ReadConfigString(Cfg, "CachedInfo", "LastIridiumDrvName");
    LPCSTR pri_irhelpDrv = ReadConfigString(Cfg, "CachedInfo", "LastIridiumHelperDrvName");

    dbgout("Trying to delete old driver:%s,ret=0x%x\n", pri_irDrv, DeleteFileA(pri_irDrv));
    dbgout("Trying to delete old driver:%s,ret=0x%x\n", pri_irhelpDrv, DeleteFileA(pri_irhelpDrv));

    cur_irDrv = StrConnect(TargetPath, StrConnect(RandomTextGenerate(16), ".sys"));
    dbgout("Generated random file name for main driver,name=%s\n", cur_irDrv);
    cur_irhelpDrv = StrConnect(TargetPath, StrConnect(RandomTextGenerate(16), ".sys"));
    dbgout("Generated random file name for helper driver,name=%s\n", cur_irhelpDrv);

    WriteConfigString(Cfg, "CachedInfo", "LastIridiumDrvName", cur_irDrv);
    WriteConfigString(Cfg, "CachedInfo", "LastIridiumHelperDrvName", cur_irhelpDrv);

    char selfPath[MAX_PATH];
    GetModuleFileNameA(nullptr, selfPath, MAX_PATH);
    HANDLE hFile = CreateFileA(selfPath, GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return 0;

    // 将文件映射到内存
    HANDLE hMapping = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hMapping) {
        CloseHandle(hFile);
        return 0;
    }

    LPVOID pData = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!pData) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    DWORD fileSize = GetFileSize(hFile, nullptr);
    const char* fileEnd = static_cast<const char*>(pData) + fileSize;

    // 从文件末尾开始查找第二个结束标记
    const char* endMarker2Pos = fileEnd - sizeof(END_MARKER2);
    while (endMarker2Pos > static_cast<const char*>(pData)) {
        if (memcmp(endMarker2Pos, END_MARKER2, sizeof(END_MARKER2)) == 0) break;
        endMarker2Pos--;
    }

    if (endMarker2Pos <= static_cast<const char*>(pData)) {
        printA("导出自带的内核驱动程序失败:找不到文件标记\n", 3);
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    // 读取第二个头结构
    const DRV_HEADER2* header2 = reinterpret_cast<const DRV_HEADER2*>(
        endMarker2Pos - sizeof(DRV_HEADER2));

    // 验证第二个头结构识别码
    if (memcmp(header2->recognizer, "RTCOR64_KERNEL_MODE_DRIVER_DATA_SECTION_1384012",
        sizeof(header2->recognizer)) != 0) {
        printA("导出自带的内核驱动程序失败:头错误\n", 3);
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    // 计算第二个驱动程序数据位置
    const BYTE* drvData2 = reinterpret_cast<const BYTE*>(header2) - header2->filesize;

    // 验证第二个驱动程序的完整性（使用加密后数据的SHA256）
    LPCSTR sha256Calc2 = CalculateSHA256Hex(drvData2, header2->filesize);
    if (memcmp(header2->dic, sha256Calc2, 64) != 0) {
        printA("导出自带的内核驱动程序失败:SHA256值不匹配\n", 3);
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    // 查找第一个结束标记
    const char* endMarker1Pos = reinterpret_cast<const char*>(drvData2) - sizeof(END_MARKER);
    while (endMarker1Pos > static_cast<const char*>(pData)) {
        if (memcmp(endMarker1Pos, END_MARKER, sizeof(END_MARKER)) == 0) break;
        endMarker1Pos--;
    }

    if (endMarker1Pos <= static_cast<const char*>(pData)) {
        printA("导出自带的内核驱动程序失败:找不到文件标记\n", 3);
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    // 读取第一个头结构
    const DRV_HEADER* header1 = reinterpret_cast<const DRV_HEADER*>(
        endMarker1Pos - sizeof(DRV_HEADER));

    // 验证第一个头结构识别码
    if (memcmp(header1->recognizer, "IRIDIUM_KERNEL_MODE_DRIVER_DATA_SECTION_1384012",
        sizeof(header1->recognizer)) != 0) {
        printA("导出自带的内核驱动程序失败:头错误\n", 3);
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    // 计算第一个驱动程序数据位置
    const BYTE* drvData1 = reinterpret_cast<const BYTE*>(header1) - header1->filesize;

    // 验证第一个驱动程序的完整性（使用加密后数据的SHA256）
    LPCSTR sha256Calc1 = CalculateSHA256Hex(drvData1, header1->filesize);
    if (memcmp(header1->dic, sha256Calc1, 64) != 0) {
        printA("导出自带的内核驱动程序失败:SHA256值不匹配1\n", 3);
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    // ---------- 提取第一个驱动程序（先解密）----------
    HANDLE hTarget1 = CreateFileA(cur_irDrv, GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hTarget1 == INVALID_HANDLE_VALUE) {
        printA("导出自带的内核驱动程序失败:无法创建Iridium驱动程序\n", 3);
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }
    dbgout("Created Iridium Driver file %s,handle=0x%x\n", cur_irDrv, hTarget1);

    StopService(drv2);
    UnInstallService(drv2);

    printA("正在解密Iridium驱动程序...\n", 1);

    // 分配缓冲区用于解密
    BYTE* decryptedData1 = new BYTE[header1->filesize];
    memcpy(decryptedData1, drvData1, header1->filesize);
    for (DWORD i = 0; i < header1->filesize; ++i) {
        decryptedData1[i] ^= xorKey[i % xorKeyLen];
    }

    DWORD bytesWritten;
    if (!WriteFile(hTarget1, decryptedData1, header1->filesize, &bytesWritten, nullptr) ||
        bytesWritten != header1->filesize) {
        printA("导出自带的内核驱动程序失败:写入文件Iridium驱动程序\n", 3);
        CloseHandle(hTarget1);
        DeleteFileA(cur_irDrv);
        delete[] decryptedData1;
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }
    CloseHandle(hTarget1);
    delete[] decryptedData1;

    // ---------- 提取第二个驱动程序 ----------
    HANDLE hTarget2 = CreateFileA(cur_irhelpDrv, GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hTarget2 == INVALID_HANDLE_VALUE) {
        printA("导出自带的内核驱动程序失败:无法创建目标文件Iridium辅助驱动\n", 3);
        DeleteFileA(cur_irDrv);
        DeleteFileA(cur_irhelpDrv);
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    dbgout("Created Iridium Helper Driver file %s,handle=0x%x\n", cur_irhelpDrv, hTarget2);

    printA("正在解密Iridium辅助驱动程序...\n", 1);

    BYTE* decryptedData2 = new BYTE[header2->filesize];
    memcpy(decryptedData2, drvData2, header2->filesize);
    for (DWORD i = 0; i < header2->filesize; ++i) {
        decryptedData2[i] ^= xorKey[i % xorKeyLen];
    }

    if (!WriteFile(hTarget2, decryptedData2, header2->filesize, &bytesWritten, nullptr) ||
        bytesWritten != header2->filesize) {
        printA("导出自带的内核驱动程序失败:写入Iridium辅助驱动\n", 3);
        CloseHandle(hTarget2);
        DeleteFileA(cur_irDrv);
        DeleteFileA(cur_irhelpDrv);
        delete[] decryptedData2;
        UnmapViewOfFile(pData);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }
    CloseHandle(hTarget2);
    delete[] decryptedData2;

    // 清理资源
    UnmapViewOfFile(pData);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    dbgout("Embedded driver released and decrypted successfully.\n");
    return 1;
}

DWORD __DriverInit() {
    dbgout("__DriverInit() was called.\n");
    printA("正在安装内核模式驱动程序...\n", 1);
    //先卸载服务 防止之前异常退出后没有卸载驱动

    HANDLE hDriver = CreateFileA(sym, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDriver != INVALID_HANDLE_VALUE) {
        DeviceIoControl(hDriver, IRIDIUM_DRV_UNLOAD, 0, 0, 0, 0, 0, 0);
        CloseHandle(hDriver);
        Sleep(200);
    }

    LPCSTR pri_irName = ReadConfigString(Cfg, "CachedInfo", "LastIridiumSvcName");

    StopService(pri_irName);
    UnInstallService(pri_irName);

    if (!__ExtractDriver(".\\")) { //导出
        printA("数据区没有找到可用的驱动程序,无法进入内核模式.\n", 3);
        return 0;
    }

    Sleep(200);

    drv_m_name = RandomTextGenerate(16);

    WriteConfigString(Cfg, "CachedInfo", "LastIridiumSvcName", drv_m_name);

	//安装服务
	if (!InstallService(drv_m_name, "Iridium Kernel Mode Driver", cur_irDrv, "Iridium内核模式驱动", SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START, SERVICE_ERROR_CRITICAL)) {
		printAE("安装驱动服务失败", 3);
		return GetLastError();
	}

    //启动服务 

    /*
            if (!LaunchService(drv)) {
            
        }
    
    */

    if (GetServiceStatus(drv_m_name) != SERVICE_RUNNING) {
        dbgout("StartService first failed.\n");
        BOOL enabled = 0;
        DWORD c = __RTCoreInit(cur_irDrv, cur_irhelpDrv);
        if (c) {
            printA("Iridium_DriverHelper初始化失败,若未关闭驱动强制签名Iridium驱动将加载失败! (代码:", 2);
            printf("%d)\n", c);
        }
        else {
            enabled = 1;
        }

        if (!LaunchService(drv_m_name)) {
            printAE("启动驱动服务失败", 3);
            UnInstallService(drv_m_name);
            return GetLastError();
        }

        if (enabled) __RTCoreUninit(cur_irhelpDrv);
    }

	//检查符号链接
	hDriver = CreateFileA(sym, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDriver == INVALID_HANDLE_VALUE) {
		printA("无法检测到驱动程序的符号链接,驱动可能没有正确初始化!正在卸载驱动...\n", 2);
		CloseHandle(hDriver);
		StopService(drv_m_name);
		UnInstallService(drv_m_name);
		return IR_DRIVER_SYMBOLLINK_ERROR;
	}

	CloseHandle(hDriver);

	return __CfgDriver();
}


DWORD __CfgDriver() {
    dbgout("__CfgDriver() was called.\n");
    printA("正在配置驱动程序...\n", 1);
	HANDLE hDriver = CreateFileA(sym, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDriver == INVALID_HANDLE_VALUE) {
        printAE("无法打开驱动程序,初始化失败.", 3);
        StopService(drv_m_name);
        UnInstallService(drv_m_name);
        return IR_DRIVER_DRIVER_DIED;
    }

    IRIDIUM_INIT init = { 0 };
    init.ProcId = GetCurrentProcessId();
    init.Protect = Config->DriverProtect;
    init.ProtectEx = Config->DriverProtectEx;

    DWORD dataLen = (DWORD)offsetof(IRIDIUM_INIT, sha256);
    LPCSTR hashResult = CalculateSHA256Hex(&init, dataLen);

    dbgout("Init package for driver SHA256:%s\n", hashResult);

    if (hashResult != NULL) {
        memcpy(init.sha256, hashResult, 64);
    }
    else {
        printAE("准备驱动初始化结构体失败", 3);
        CloseHandle(hDriver);
        return 0;
    }

    BOOL bRet = DeviceIoControl(hDriver, IRIDIUM_INIT_INST, &init, sizeof(init), 0, 0, 0, 0);
    if (!bRet) {
        printAE("向驱动程序发送初始化参数失败", 3);
        CloseHandle(hDriver);
        return 0;
    }


    //初始化驱动完成后移除控制台的关闭按钮
    HMENU MainMenu = GetSystemMenu(GetConsoleWindow(), false);
    RemoveMenu(MainMenu, SC_CLOSE, MF_BYCOMMAND);
    DrawMenuBar(GetConsoleWindow());

    CloseHandle(hDriver);

    iridium_kmd = 1;
	return IR_DRIVER_SUCCESSFULLY;
}
