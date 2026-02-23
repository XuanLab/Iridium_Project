#include <Windows.h>
#include <iostream>
#include <Psapi.h>
#include "FunctionCall.h"
#include "Define.h"

LPCSTR sha256[32] = { 0 };

DWORD __EnvironmentCheck() {
	printA("正在检查系统环境...\n", 1);
	dbgout("__EnvironmentCheck() was called.\n");

	//操作系统版本检测
	VERSION_INFO ver = GetVersionProc();

	dbgout("OS Version : Major=%d Minor=%d Build=%d\n", ver.dwMajor, ver.dwMinor, ver.dwBuild);

	if (ver.dwMajor < 10) {
		char msgbuf[256] = { 0 };
		printA("检测到不支持的操作系统!!!\n", 4);
		sprintf_s(msgbuf, "错误:您的操作系统不在支持范围内(需要Windows10或以上,当前系统为%d-%d.%d),请更新操作系统后重试.", ver.dwMajor, ver.dwMinor, ver.dwBuild);
		MessageBoxA(GetConsoleWindow(), msgbuf, "错误", MB_ICONERROR | MB_OK);
		printf("按任意键退出Iridium...\n");
		system("pause>nul");
		ExitProcess(IR_CHECK_VERSION_OUTDATED);
	}

	return IR_CHECK_SUCCESSFULLY;
}
