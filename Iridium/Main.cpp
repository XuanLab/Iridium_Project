#include <Windows.h>
#include <iostream>
#include <string>
#include "FunctionCall.h"
#include "Console.h"
#include "Define.h"
#include <cstdint>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#include <shellscalingapi.h>
#pragma comment(lib, "shcore.lib")

HINSTANCE hInstance = 0;
ULONGLONG ProgramBootTick = 0;
int CrashCount = 0;

extern "C" uint64_t g_BaseCycles = 0;
DWORD DebuggerCheckEnabled = 1;

void _BuildInfo() {
		std::string compiler =
	#if defined(_MSC_VER)
			"MSVC " + std::to_string(_MSC_VER);
	#elif defined(__GNUC__)
			"GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
	#else
			"Unknown Compiler";
	#endif
        printf(" ______           Iridium was built by %s complier,build date-time:%s-%s\n/\\__  _\\          This is a GPLv3 Opened Source Software\n\\/_/\\ \\/   _ __   You have the right to operate,study and modify this software.\n   \\ \\ \\  /\\`'__\\ Modified versions must remain open source and indicate the original project.\n    \\_\\ \\_\\ \\ \\/  For intact license, please visit https://webres.xuan.asia/license.html \n   \\/_____/\\/_/   For user manual for iridium please visit https://iridium.xuan.asia/usermanual.html \n\n", compiler.c_str(), __DATE__, __TIME__);
}

int main(HINSTANCE hInst) {
    ProgramBootTick = GetTickCount64();
    cls();
    Sleep(200); //等待前一个进程完全退出
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    HANDLE hIridiumMutex = CreateMutexA(NULL, FALSE, "IridiumMutexForMulti-ProcessChk");
    
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (MessageBoxA(GetConsoleWindow(), "当前系统中已经有Iridium进程正在运行.\n如果开启多个进程,可能会导致不可预料的问题.\n\n要继续吗?", "提示", MB_ICONQUESTION | MB_YESNO) == IDNO) {
            HWND irdlg = FindWindowA("Iridium", 0);
            ShowWindow(irdlg, 1);
            return 0; // 退出程序
        }
    }

	_BuildInfo();

    //设置异常处理程序
    SetupCrashReporting();

    CrashCount = ReadConfigDword(Cfg, "General", "CrashCount");

    if (CrashCount >= 5) {
        WriteConfigInt(Cfg, "General", "CrashCount", 0);
        if (MessageBoxA(GetConsoleWindow(), "我们注意到程序短时间内非正常退出多次,如果你认为这是Iridium的问题,请将转储文件上传至Iridium服务器.\n\n是否退出进程?", "警告", MB_ICONQUESTION | MB_YESNO) == IDYES) {
            ExitProcess(-1);
        }
    }

    if (!IsRunAsAdmin()) {
        if (MessageBoxA(GetConsoleWindow(), "检测到未以管理员身份运行本程序,驱动可能无法正常加载,部分功能将无法正常使用!\n\n点击确定以尝试提权至管理员", "警告", MB_ICONWARNING | MB_YESNO) == IDYES) {
            wchar_t szPath[MAX_PATH];
            if (GetModuleFileNameW(nullptr, szPath, ARRAYSIZE(szPath)) == 0) {
                return false;
            }

            // 以管理员身份重新启动进程
            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.lpParameters = GetCommandLineW();
            sei.lpVerb = L"runas"; // 请求UAC提权
            sei.lpFile = szPath;
            sei.hwnd = nullptr;
            sei.nShow = SW_NORMAL;

            if (!ShellExecuteExW(&sei)) {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_CANCELLED) {
                    MessageBoxA(GetConsoleWindow(), "你取消了提权操作,将在低权限模式下运行", "提示", MB_ICONINFORMATION | MB_OK);
                }
                
            }
            else {
                ExitProcess(0);
                return true;
            }

         
        }
    }

    hInstance = GetModuleHandleA(0);

    #ifdef IRIDIUM_KMODE_DRIVER_CONTAINED
    printf("Iridium Version:%s (With Kernel Mode Driver Embedded.) (Static Complied)\n", IRIDIUM_VERSION);
    #elif
    printf("Iridium Version:%s (No Kernel Mode Driver Embedded.) (Static Complied)\n", IRIDIUM_VERSION);
    #endif //IRIDIUM_KMODE_DRIVER_CONTAINED


    printf("SHA256:%s\n\n", MySHA256());

    DWORD r = 0;
    r = __preInit();
    if (r) {
        printA("预先初始化错误:代码", 3);
        printf("%x\n", r);
    }

    Config->DebuggerCheck = ReadConfigBoolean(Cfg, "General", "DebuggerCheck");
    DebuggerCheckEnabled = Config->DebuggerCheck;

    if (!DebuggerCheckEnabled) {
        printA("用户禁用了调试器检测,将不检查调试器存在情况.\n", 2);
        dbgout("Debugger check disabled.\n");
    }
    else {
        CheckDebugger();
        dbgout("First called CheckDebugger() g_BaseCycles=%lld\n", g_BaseCycles);

        if (CheckDebugger()) {
            //将DebuggerCheckEnabled设为false 避免重复提醒
            DebuggerCheckEnabled = 0;
            dbgout("Found debugger attached to current process!!!\n");
            printA("发现调试器附加在了本程序上!\n", 2);
            MessageBoxA(GetConsoleWindow(), "Iridium检测到存在调试器附加在了本程序上.\n如果你正在执行调试,可以将配置文件中DebuggerCheck值设为false,\n并关闭驱动保护功能.", "提示", MB_ICONINFORMATION | MB_OK);
        }
        else {
            dbgout("No debugger found on current process.\n");
        }
        dbgout("Second called CheckDebugger() g_BaseCycles=%lld\n", g_BaseCycles);
    }

    if (SetPrivilege(GetCurrentProcess(), SE_DEBUG_NAME)) {
        dbgout("Successfully granted debug privilege.\n");
    }

    //获取所有特权
    for (int i = 0; i <= 0x24; i++) {
        RtlAdjustCurrentPrivilege(i);
    }

    r = __procCmdline();
    if (r) {
        printA("获取命令行参数时遇到错误,将无法解析其参数.\n", 2);
    }

    r = __EnvironmentCheck();
    if (r) {
        printA("操作系统环检查境错误:代码", 3);
        printf("%x\n", r);
    }

    r = __Init();
    if (r) {
        printA("初始化错误:代码", 3);
        printf("%x\n", r);
        ExitProcess(r);
    }

    dbgout("Initializing framework...\n");

    __AssembleMainGui();

    dbgout("Initialization completed in %lldms\n", GetTickCount64() - ProgramBootTick);

    MSG MainWindowMessage;
    while (GetMessageW(&MainWindowMessage, NULL, 0, 0))        //从消息队列中获取消息
    {
        TranslateMessage(&MainWindowMessage);                 //将虚拟键消息转换为字符消息
        DispatchMessage(&MainWindowMessage);                  //分发到回调函数(过程函数)
        if (!IsWindow(MainWindow)) break;
    }
    printA("主窗口已被关闭,清理后退出...\n", 1);
    dbgout("Main window was destroyed,cleaning up and exit.\n");
    dbgout("Process alive duration %lldms\n", GetTickCount64() - ProgramBootTick);
    __GUI_Summary();

    //Clean-up


    //FILE
    __Utils_Free();
    WriteConfigInt(Cfg, "General", "CrashCount", 0);  //重置非正常退出计数

    //DRV
    if (Config->LoadDriver) {
        dbgout("Stopping Iridium kernel mode driver...\n");
        dbgout("Unlocking driver to unload it...\n");
        HANDLE hDrv = CreateFileA("\\\\.\\Iridium", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hDrv == INVALID_HANDLE_VALUE) {
            dbgout("Failed to open iridium driver,we can't stop it now.\n");
            printAE("与内核模式驱动通信失败", 3);
        }
        else {
            DeviceIoControl(hDrv, IRIDIUM_DRV_UNLOAD, 0, 0, 0, 0, 0, 0);
        }

        Sleep(20);

        if (GetServiceStatus(drv_m_name) == SERVICE_RUNNING) {
            r |= StopService(drv_m_name);
            r |= UnInstallService(drv_m_name);

            if (!r) {
                dbgout("Stop the kernel mode driver failed!\n");
                printA("未能停止Iridium内核模式驱动程序,下一次驱动程序初始化可能失败.\n", 2);
            }
        }
    }

    CloseHandle(hIridiumMutex);

    ExitProcess(0);
	return 0;
}