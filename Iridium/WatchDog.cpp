#include <Windows.h>
#include <iostream>
#include "FunctionCall.h"
#include "Define.h"
#include "Structures.h"

LONGLONG OldThreadTick = 0;
DWORD Repeat = 0;

VOID thread_main_montior() {
	while (1) {
		if (OldThreadTick == HeartBeat) {
			Repeat++;
		}
		else {
			OldThreadTick = HeartBeat;
			Repeat = 0;
		}

		if (Repeat > 10) {
			MessageBoxA(GetConsoleWindow(), "主线程似乎长时间未响应?", "警告", MB_ICONWARNING | IDOK);
			Repeat = 0;
		}
		if (DebuggerCheckEnabled) {
			if (CheckDebugger()) {
				MessageBoxA(MainWindow, "Iridium检测到存在调试器附加在了本程序上.\n如果你正在执行调试,可以将配置文件中DebuggerCheck值设为false,\n并关闭驱动保护功能.", "提示", MB_ICONINFORMATION | MB_OK);
				DebuggerCheckEnabled = 0;
			}
		}

		Sleep(1000);
	}
}

DWORD __WatchDogInit() {
	OldThreadTick = HeartBeat;
	DWORD tid = 0;
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_main_montior, 0, 0, &tid);
	dbgout("Created watch dog thread,id=%d\n", tid);
	return 0;
}
