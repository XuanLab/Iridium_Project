#include <Windows.h>
#include <iostream>
#include <Shlwapi.h>
#pragma comment (lib,"Shlwapi.lib")
#include "FunctionCall.h"
#include "Define.h"

HMODULE hUtils = 0;

typedef struct _IRIDIUM_MODULE_VER_INFO {
	DWORD MajorVer;
	DWORD MinorVer;
	DWORD Build;
	const char* description_ptr;
	const char* weblnk_ptr;
}IRIDIUM_MODULE_VER_INFO, * PIRIDIUM_MODULE_VER_INFO;

DWORD __InitUtils() {
	if (FileExistsStatus(".\\IridiumUtils.dll") && CheckIfExecutable(".\\IridiumUtils.dll")) {
		printA("已找到IridiumUtils,正在初始化...\n", 1);
		hUtils = LoadLibraryA(".\\IridiumUtils.dll");
		if (hUtils) {
			dbgout("Loaded Utils library.\n");
			FARPROC fp = 0;
			


		}
		else {
			printAE("加载IridiumUtils失败! ", 2);
			return GetLastError();
		}
	}
	else {
		return -1;
	}
}

DWORD __Utils_Free() {

	if (!hUtils) {
		dbgout("No utils library loaded,skipping...\n");
		return 2;
	}

	if (FreeLibrary(hUtils)) {
		dbgout("Free utils library successfully!\n");
		return 0;
	}
	else {
		dbgout("Failed to free utils library!\n");
		return 1;
	}
}

DWORD __Utils_AcquireResource(DWORD index, LPVOID Buffer) {
	if (index < 0 || !hUtils || Buffer == nullptr) return 1;

}

typedef IRIDIUM_MODULE_VER_INFO(NTAPI* F__IridiumModuleVersionInfo)(void);

DWORD __ThirdPartyModuleLoad(HMODULE dll) {
	
	return 0;
}

DWORD __ThirdPartyModulePreLoadCheck(LPCSTR p) {
	char buf[768] = { 0 };

	sprintf_s(buf, "是否要加载模块:%s?\n确认之后,我们将于该模块进行信息交换.\n警告:我们无法保证第三方模块的安全性,Iridium开发者并不对使用第三方模块产生的任何后果负责!!!", PathFindFileNameA(p));

	if (MessageBoxA(GetConsoleWindow(), buf, "模块管理器", MB_ICONWARNING | MB_YESNO) != IDYES) {
		dbgout("User canceled operation.\n");
	}
	HMODULE h = 0;
	FARPROC f1, f2, f3, f4;
	f1 = GetProcAddress(h, "__IridiumModuleVersionInfo");
	if (!f1) dbgout("__IridiumModuleVersionInfo not found.\n");

	f2 = GetProcAddress(h, "__PostFunctionAddress");
	if (!f2) dbgout("__PostFunctionAddress not found.\n");

	f3 = GetProcAddress(h, "__PostClientVersionInfo");
	if (!f3) dbgout("__PostClientVersionInfo not found.\n");

	f4 = GetProcAddress(h, "__IridiumModuleAuthorInfo");
	if (!f4) dbgout("__IridiumModuleAuthorInfo not found.\n");

	if (!f1 || !f2 || !f3 || !f4) {
		printA("无法识别文件", 2);
		printf("%s,无法在此文件中找到特定的函数.\n", p);
		FreeLibrary(h);
		return 2;
	}

	dbgout("Loaded 3rd party module %s exchanging information...\n", PathFindFileNameA(p));

	F__IridiumModuleVersionInfo __IMVI = (F__IridiumModuleVersionInfo)f1;
	IRIDIUM_MODULE_VER_INFO vi = __IMVI();

	return __ThirdPartyModuleLoad(h);
}


DWORD __ThirdPartyModulesInit() {
	dbgout("__ThirdPartyModulesInit() was called.\n");

	dbgout("Scanning modules in .\\ir_plugins...\n");
	
	LPCSTR buf = nullptr;
	DWORD f = 0;
	f = EnumFiles(".\\ir_plugins", "dll", &buf, TRUE);
	if (!f) {
		dbgout(".\\ir_plugins is empty or doesn't exists.\n");
		return 0;
	}
	dbgout("Found %d iridium modules.\n", f);
	printA("正在加载第三方插件...\n", 1);

	const char* current = buf;
	for (DWORD i = 0; i < f; i++) {
		__ThirdPartyModulePreLoadCheck(current);
		current += strlen(current) + 1; // 移动到下一个路径
	}

	free((void*)buf);
}

