#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(suppress : 4996)
#include <windows.h>
#include <WinInet.h>
#include <Shlwapi.h>
#include <iostream>
#include "FunctionCall.h"
#include "Define.h"
#pragma comment(lib, "winhttp.lib")
#pragma comment (lib,"shlwapi.lib")
#pragma comment( lib, "wininet.lib") 


DWORD DownloadFile(LPCSTR Url, LPCSTR savePath) {

	if (!g_Settings_Network) {
		printA("配置文件要求禁用网络链接,下载不可用.\n", 3);
		return 0;
	}

	byte Temp[2048] = { 0 };
	ULONG Number = 1;

	savePath = ResolveTempShortPath(savePath);

	HANDLE hTmp = CreateFileA(savePath, FILE_ALL_ACCESS, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hTmp == INVALID_HANDLE_VALUE) return 0;

	HINTERNET hSession = InternetOpenA("RookIE/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hSession != NULL)
	{
		HINTERNET handle2 = InternetOpenUrlA(hSession, Url, NULL, 0, INTERNET_FLAG_DONT_CACHE, 0);
		if (handle2 != NULL)
		{
			while (Number > 0) {
				InternetReadFile(handle2, Temp, sizeof(Temp) - 1, &Number);
				WriteFile(hTmp, Temp, Number, &Number, 0);
			}
			CloseHandle(hTmp);

			dbgout("Downloaded file from %s\n", Url);

			InternetCloseHandle(handle2);
			handle2 = NULL;
		}
		else {
			dbgout("Open remote server url failed.\n");
			CloseHandle(hTmp);
			return 0;
		}
		InternetCloseHandle(hSession);
		hSession = NULL;
	}
	else {
		dbgout("Create session failed.\n");
		CloseHandle(hTmp);
		return 0;
	}
	//CloseHandle(hTmp);
	return 1;
}