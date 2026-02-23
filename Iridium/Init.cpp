#include <Windows.h>
#include <Psapi.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")
#include "FunctionCall.h"
#include "Define.h"
#include <iostream>

char* raname = 0;
HFONT UniversalFont;

PIRIDIUM_CONFIG Config;
//支持的命令
LPCWSTR h_arg[] = { L"--nonetwork",L"--debug",L"--forcetop",L"--log",L"--recovery",L"--emer" ,L"--auto" ,L"--nocfg",L"--int_uiaccess_token_granted" };

//常用参数
LPCWSTR u_yn[] = { L"yes",L"no" };
LPCWSTR u_nf[] = { L"on",L"off" };
LPCWSTR u_tf[] = { L"true",L"false" };

BOOL AutoCfg = 0;
LONGLONG HeartBeat = GetTickCount64();
BOOL DoAutoConfig();

DWORD __i_cmdproc(LPCWSTR arg) {
	if (lstrcmpW(arg, L"--debug") == 0) {
		Config->DebugMessage = 1;
		Config->InternalDebugger = 1;
		return 1;
	}

	if (lstrcmpW(arg, L"--int_uiaccess_token_granted") == 0) {
		Config->UiAccess = 1;
		return 1;
	}

	if (lstrcmpW(arg, L"--forcetop") == 0) {
		Config->ForceOnTop = 1;
		return 1;
	}

	if (lstrcmpW(arg, L"--auto") == 0) {
		AutoCfg = 1;
		DoAutoConfig();
		return 1;
	}
	return 0;
}

DWORD __procCmdline() {
	dbgout("__procCmdline() was called.\n");
	LPCWSTR cmd = GetCommandLineW();
	int arg = 0;
	LPWSTR* argd = CommandLineToArgvW(cmd, &arg);
	if (arg == NULL) return GetLastError();

	for (int i = 1; i < arg; i++) { //第一个命令行是程序运行目录,忽略之
		for (int a = 0; a < ARRAYSIZE(h_arg); a++) {
			if (lstrcmp(h_arg[a], argd[i]) == 0) {
				__i_cmdproc(argd[i]);
				break;
			}

			if (lstrcmp(h_arg[a], argd[i]) != 0 && (a == ARRAYSIZE(h_arg) - 1)) {
				printA("无效的参数", 2);
				printf("%ws,已将其忽略.\n", argd[i]);
			}
		}
	}

	return 0;
}

BOOL __ApiInit();

DWORD __preInit() {
	printA("预先初始化...\n", 1);

	Config = (PIRIDIUM_CONFIG)VirtualAlloc(0, sizeof(IRIDIUM_CONFIG), MEM_COMMIT, PAGE_READWRITE);
	if (Config == nullptr) {
		printA("分配配置文件保存内存失败.\n", 3);
		Config = (PIRIDIUM_CONFIG)malloc(sizeof(IRIDIUM_CONFIG));
	}
	memset(Config, sizeof(IRIDIUM_CONFIG), 0);

	Config->DebugMessage = ReadConfigBoolean(Cfg, "General", "DebugMessage");
	if (Config->DebugMessage) printA("已启用调试消息打印.\n", 1);

	dbgout("Current config data base:0x%p\n", Config);

	//初始化ntapi
	if (!__ApiInit()) {
		printA("NTAPI初始化失败!\n", 2);
	}
	
	//字体初始化
	UniversalFont = CreateFontW(-12, -6, 0, 0, 100, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"微软雅黑"); //通用字体
	if (!UniversalFont) {
		dbgout("Failed to create new font,using default font.\n");
		MessageBoxA(GetConsoleWindow(), "错误:未能创建字体!\n\n使用默认字体", "错误", MB_OK);
		UniversalFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	}


	//随机名称初始化
	Config->RandomName = ReadConfigBoolean(Cfg, "General", "RandomName") || !FileExistsStatus(Cfg);
	if (Config->RandomName) {
		raname = RandomTextGenerate(20);
		SetConsoleTitleA(raname);
	}
	else {
		raname = (char*)"Iridium";
	}

	//定位将来要使用的函数
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	if (!hNtdll) {
		hNtdll = LoadLibraryA("ntdll.dll");
		if (!hNtdll) return IR_INIT_MODULE_FAILURE;
	}

	dbgout("Initializing unpublic api...\n");

	NtSetInformationThread fNtSetInformationThread = (NtSetInformationThread)GetProcAddress(hNtdll, "NtSetInformationThread");
	if (fNtSetInformationThread) dbgout("Found NtSetInformationThread(),addr=0x%p\n", fNtSetInformationThread);

	dbgout("Initializing watchdog thread...\n");
	__WatchDogInit();

	if (!fNtSetInformationThread(GetCurrentThread(), ThreadHideFromDebugger, NULL, 0)) {
		dbgout("Successfully hide main thread from debugger via NtSetInformationThread.\n");
	}

	return IR_INIT_SUCCESSFULLY;
}


BOOL reCfg(LPCSTR path);

DWORD __Init() {
	printA("正在初始化...\n", 1);
	dbgout("__Init() was called.\n");
	
	//读取配置文件
	if (!FileExistsStatus(Cfg) && !AutoCfg) {
		if (MessageBoxA(GetConsoleWindow(), "未能在当前目录中找到配置文件Iridium.ini,要重新配置吗?\n若不重新配置,程序将自动执行配置操作.", "配置文件", MB_YESNO | MB_ICONWARNING) == IDYES) {
			if (!reCfg(Cfg)) {
				MessageBoxA(GetConsoleWindow(), "保存配置文件失败!", "错误", MB_OK | MB_ICONERROR);
				return IR_INIT_CFG_NOT_FOUND;
			}
		}
		else {
			DoAutoConfig();
			AutoCfg = 1;
		}
	}

	Config->DriverProtectEx = Config->DriverProtectEx || ReadConfigBoolean(Cfg, "Driver", "ProtectEx");
	Config->DriverProtect = Config->DriverProtect || ReadConfigBoolean(Cfg, "Driver", "Protect");
	Config->DisableCapture = Config->DisableCapture || ReadConfigBoolean(Cfg, "Window", "DisableCapture");

	Config->IridiumDlgHeight = ReadConfigDword(Cfg, "Window", "WindowHeight");
	Config->IridiumDlgWidth = ReadConfigDword(Cfg, "Window", "WindowWidth");

	Config->InternalDebugger = Config->InternalDebugger || ReadConfigBoolean(Cfg, "General", "InternalDebugger");
	Config->ForceOnTop = Config->ForceOnTop || ReadConfigBoolean(Cfg, "Window", "ForceOnTop");

	if (!Config->IridiumDlgHeight) Config->IridiumDlgHeight = 850;
	if (!Config->IridiumDlgWidth)Config->IridiumDlgWidth = 1300;

	if (Config->ForceOnTop && !Config->UiAccess) {
		DWORD err = PrepareForUIAccess();
		if (err) {
			MessageBoxA(GetConsoleWindow(), "操作执行失败,强制窗口置顶配置已禁用.", "提示", MB_ICONERROR | MB_OK);
		}
		else {
			ExitProcess(err);
		}
	}

	Config->LoadDriver = Config->LoadDriver || ReadConfigBoolean(Cfg, "Driver", "LoadDriver");
	if (Config->LoadDriver) {
		__DriverInit();
	}
	
	Config->Network = Config->Network || ReadConfigBoolean(Cfg, "General", "AllowNetwork");
	if (!Config->Network) {
		printA("配置文件要求禁用网络链接,更新等功能将不可用.\n", 1);
	}

	if (Config->InternalDebugger) {
		__AssembleIIDGUI();
	}

	__InitUtils();

	if (!ReadConfigBoolean(Cfg, "General", "Load3rdPartyModule")) {
		__ThirdPartyModulesInit();
	}

	return IR_INIT_SUCCESSFULLY;
}


BOOL reCfg(LPCSTR path) {
	dbgout("__reCfg() was called.\n");
	BOOL r = 1;
	r |= WriteConfigBoolean(path, "Window", "DisableCapture", 0);

	r |= WriteConfigBoolean(path, "General", "RandomName", 1);
	r |= WriteConfigBoolean(path, "General", "DebugMessage", 1);
	r |= WriteConfigBoolean(path, "General", "InternalDebugger", 0);
	r |= WriteConfigBoolean(path, "General", "AllowNetwork", 1);
	r |= WriteConfigBoolean(path, "General", "DebuggerCheck", 1);

	r |= WriteConfigBoolean(path, "Driver", "LoadDriver", 1);
	r |= WriteConfigBoolean(path, "Driver", "Protect", 1);
	r |= WriteConfigBoolean(path, "Driver", "ProtectEx", 0);

	r |= WriteConfigString(path, "System", "OSVersion", GetVersionProc().String);
	r |= WriteConfigBoolean(path, "System", "CacheOffsets", 1);
	r |= WriteConfigInt(path, "System", "CI.dll:g_CiOptions", 0);


	//r|=WriteConfigBoolean(path,"")
	return r;
}

BOOL DoAutoConfig() {
	Config->DebugMessage = 1;
	Config->DriverProtect = 1;
	Config->LoadDriver = 1;
	Config->Network = 1;
	Config->RandomName = 1;
	Config->UiAccess = 1;

	printA("自动配置执行完成.\n", 1);
	return 1;
}


//NtAPI
FNtDeviceIoControlFile pNtDeviceIoControlFile;
FNtCreateFile pNtCreateFile;

BOOL __ApiInit() {
	dbgout("__ApiInit() was called.\n");
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	if (!hNtdll) return 0;
	pNtDeviceIoControlFile = (FNtDeviceIoControlFile)GetProcAddress(hNtdll, "NtDeviceIoControlFile");
	pNtCreateFile = (FNtCreateFile)GetProcAddress(hNtdll, "NtCreateFile");

	return 1;
}