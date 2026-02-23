#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(suppress : 4996)
#include "FunctionCall.h"
#include "Define.h"
#include <ShlObj.h>
#include <CommCtrl.h>
#include <Softpub.h>
#include <stdio.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <winver.h>
#pragma comment(lib,"comctl32.lib")
#include <WinTrust.h>
#pragma comment(lib,"wintrust.lib")
#include <wincrypt.h>
#pragma comment(lib,"Version.lib")


/*
Copyright (C) XuanLaboratory - 2024 All Rights Reserved
文件操作
*/

#define fn  "FileIO.cpp"

//常规文件IO

LPCSTR File_ReadFileFull(LPCSTR FileName) {
	dbgout("internal_File_ReadFileFull : Path=%s ", FileName);
	HANDLE hFile = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (Config->DebugMessage) printf("CreateFile failed - 0x%x\n", GetLastError());
		return NULL;
	}
	DWORD len = GetFileSize(hFile, 0);
	char* Buffer = (char*)malloc(len+1);
	RtlZeroMemory(Buffer, strlen(Buffer));
	DWORD read = 0;
	BOOL Status = ReadFile(hFile, Buffer, len, &read, 0);
	CloseHandle(hFile);
	if (Status) {
		LPCSTR data = Buffer;
		if (Config->DebugMessage) printf("OK\n");
		return data;
	}
	else {
		if (Config->DebugMessage) printf("ReadFile failed - 0x%x", GetLastError());
		return NULL;
	}
}

BOOL File_WriteFile(LPCSTR FileName,LPCSTR Info) {
	dbgout("internal_File_WriteFile : Path=%s ", FileName);
	HANDLE hFile = CreateFileA(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (Config->DebugMessage) printf("CreateFile failed - 0x%x\n", GetLastError());
		return false;
	}

	DWORD WriteLen = 0;
	DWORD Datalen = lstrlenA(Info);
	BOOL Stat = WriteFile(hFile, Info ,Datalen , &WriteLen, 0);
	CloseHandle(hFile);
	if (Config->DebugMessage) printf("WriteFile ret - 0x%x", GetLastError());
	return Stat;
}

BOOL File_WriteFileAttach(LPCSTR FileName, LPCSTR Info) {
	dbgout("internal_File_WriteFileAttach : Path=%s ", FileName);
	HANDLE hFile = CreateFileA(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		hFile = CreateFileA(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile== INVALID_HANDLE_VALUE) {
			if (Config->DebugMessage) printf("CreateFile failed - 0x%x\n", GetLastError());
			return false;
		}
	}
	DWORD WriteLen = 0;
	DWORD Datalen = lstrlenA(Info);
	SetFilePointer(hFile, 0, 0, FILE_END);
	BOOL Stat = WriteFile(hFile, Info, Datalen, &WriteLen, 0);
	CloseHandle(hFile);
	if (Config->DebugMessage) printf("WriteFile ret - 0x%x\n", GetLastError());
	return Stat;
}

LPCSTR ReadConfigString(LPCSTR Path, LPCSTR Section, LPCSTR Key) {
	dbgout("internal_ReadConfigString : P=%s S=%s K=%s ", Path, Section, Key);
	char* Buffer = (char*)malloc(1024);
	GetPrivateProfileStringA(Section, Key, "<N/A>", Buffer, 1024, Path);
	if (Config->DebugMessage) printf("ret=0x%x\n", GetLastError());
	return Buffer;
}

BOOL ReadConfigBoolean(LPCSTR Path, LPCSTR Section, LPCSTR Key) {
	dbgout("internal_ReadConfigBoolean P=%s S=%s K=%s ", Path, Section, Key);
	char* Buffer = (char*)malloc(5);
	GetPrivateProfileStringA(Section, Key, "false", Buffer, 5, Path);
	if (Config->DebugMessage) printf("ret=0x%x\n", GetLastError());
	if (strcmp(Buffer, "true") == 0) return TRUE;
	return FALSE;
}

DWORD ReadConfigDword(LPCSTR Path, LPCSTR Section, LPCSTR Key) {
	dbgout("internal_ReadConfigDword was called P=%s S=%s V=%s\n", Path, Section, Key);
	return GetPrivateProfileIntA(Section, Key, 0, Path);
}

BOOL WriteConfigString(LPCSTR Path, LPCSTR Section, LPCSTR Key, LPCSTR Data) {
	DWORD len = strlen(Data);
	dbgout("internal_WriteConfigString was called. P=%s S=%s K=%s V=%s\n", Path, Section, Key, Data);
	return WritePrivateProfileStringA(Section, Key, Data, Path);
}

BOOL WriteConfigBoolean(LPCSTR Path, LPCSTR Section, LPCSTR Key, BOOL b) {
	dbgout("internal_WriteConfigBoolean was called P=%s S=%s K=%s V=%d\n", Path, Section, Key, b);
	char* Buffer = (char*)malloc(6);
	if (!Buffer) {
		return FALSE;
	}
	memset(Buffer, 0, 6);
	if (b) {
		strcpy_s(Buffer, 6, "true");
	}
	else {
		strcpy_s(Buffer, 6, "false");
	}
	BOOL result = WritePrivateProfileStringA(Section, Key, Buffer, Path);
	free(Buffer);
	return result;
}

BOOL WriteConfigInt(LPCSTR Path, LPCSTR Section, LPCSTR Key, DWORD v) {
	dbgout("internal_WriteConfigInt was called.P=%s S=%s K=%s V=%d\n", Path, Section, Key, v);
	char* Buffer = (char*)malloc(sizeof(DWORD));
	sprintf(Buffer, "%d", v);
	return WritePrivateProfileStringA(Section, Key, Buffer, Path);
}

//其他功能

BOOL FileExistsStatus(LPCSTR path) {
	if (path == nullptr) return false;

	HANDLE hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//GetLastError = 32 / 33 是被占用了,文件仍存在
	if (hFile == INVALID_HANDLE_VALUE) {
		if (32 <= GetLastError() && GetLastError() <= 33) {
			return true;
		}
		else {
			return false;
		}
	}
	CloseHandle(hFile);
	return true;
}

char* CreateFileSelectDlg(const char* Title) {
	LPITEMIDLIST pil = NULL;
	INITCOMMONCONTROLSEX InitCtrls = { 0 };
	char szBuf[4096] = { 0 };
	BROWSEINFOA bi = { 0 };
	bi.hwndOwner = NULL;
	bi.iImage = 0;
	bi.lParam = NULL;
	bi.lpfn = NULL;
	bi.lpszTitle = Title;
	bi.pszDisplayName = szBuf;
	bi.ulFlags = BIF_BROWSEINCLUDEFILES;

	InitCommonControlsEx(&InitCtrls);
	pil = SHBrowseForFolderA(&bi);
	if (NULL != pil){
		SHGetPathFromIDListA(pil, szBuf);
		return szBuf;
	}
	return nullptr;
}

BOOL CheckIfExecutable(const char* Path) { // -1=失败 0=否 1=是
	HANDLE hFile = CreateFileA(Path, FILE_READ_ACCESS, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (!hFile) return false;
	char mzHeader[2] = { 0 };
	DWORD bytesRead = 0;
	BOOL result = ReadFile(hFile, mzHeader, 2, &bytesRead, nullptr);
	if (!result || bytesRead != 2 || mzHeader[0] != 'M' || mzHeader[1] != 'Z') { //检验MZ头
		CloseHandle(hFile);
		return false;
	}
	
	DWORD peHeaderOffset = 0;
	SetFilePointer(hFile, 0x3C, nullptr, FILE_BEGIN); //检验PE头
	result = ReadFile(hFile, &peHeaderOffset, sizeof(peHeaderOffset), &bytesRead, nullptr);
	if (!result || bytesRead != sizeof(peHeaderOffset)) {
		CloseHandle(hFile);
		return false;
	}

	SetFilePointer(hFile, peHeaderOffset, nullptr, FILE_BEGIN);
	char peHeader[4] = { 0 };
	result = ReadFile(hFile, peHeader, 4, &bytesRead, nullptr);
	CloseHandle(hFile);

	return (result && bytesRead == 4 && peHeader[0] == 'P' && peHeader[1] == 'E' && peHeader[2] == '\0' && peHeader[3] == '\0');
}

char* GetFilePublisher(LPCSTR Path) {
	struct TRANSLATION { WORD langID; WORD charset; } Translation;
	Translation.langID = 0x0409; //默认
	Translation.charset = 1252;

	DWORD size = 0, ptr = 0;
	size = GetFileVersionInfoSizeA(Path, &ptr);
	BYTE* buffer = (BYTE*)malloc(size);	
	if (buffer == nullptr) return nullptr;
	BOOL ret = 0;
	PVOID d = 0;
	UINT rl = 0;
	VS_FIXEDFILEINFO FileVersion = { 0 };
	ret = GetFileVersionInfoA(Path, 0, size, buffer);
	if (!ret) return nullptr;
	ret = VerQueryValueA(buffer, "\\", &d, &rl);
	if (!ret) return nullptr;
	memcpy(&FileVersion, d, sizeof(VS_FIXEDFILEINFO));
	ret = VerQueryValueA(buffer, "\\VarFileInfo\\Translation", &d, &rl);
	if (!ret && !(rl >= 4))	return nullptr;
	memcpy(&Translation, d, sizeof(TRANSLATION));
	char* buf = (char*)malloc(512);
	sprintf(buf, "\\StringFileInfo\\%04X%04X\\CompanyName", Translation.langID, Translation.charset);

	ret = VerQueryValueA(buffer, buf, &d, &rl);
	if (!ret) return nullptr;
	memset(buf, 0, 512);
	sprintf(buf, "%s", d);
	return buf;
}

BOOL VerifyDigitalSignature(LPCSTR lpPath) {
	dbgout("internal_VerifyDigitalSignature : ");
	// 将ANSI路径转换为宽字符路径
	wchar_t wszPath[MAX_PATH] = { 0 };
	if (0 == MultiByteToWideChar(
		CP_ACP,                   // ANSI代码页
		0,                        // 无特殊标志
		lpPath,                   // 输入ANSI路径
		-1,                       // 自动计算长度
		wszPath,                  // 输出宽字符缓冲区
		MAX_PATH                  // 缓冲区大小
	)) {
		if (Config->DebugMessage) printf("MultiByteToWideChar failed - 0x%x\n", GetLastError());
		return false; // 转换失败
	}

	// 配置文件信任信息
	WINTRUST_FILE_INFO fileInfo = { 0 };
	fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
	fileInfo.pcwszFilePath = wszPath; // 使用转换后的宽字符路径
	fileInfo.hFile = NULL;
	fileInfo.pgKnownSubject = NULL;

	// 配置信任验证参数
	WINTRUST_DATA trustData = { 0 };
	trustData.cbStruct = sizeof(WINTRUST_DATA);
	trustData.dwUIChoice = WTD_UI_NONE;       // 不显示UI
	trustData.fdwRevocationChecks = WTD_REVOKE_NONE; // 不检查吊销列表
	trustData.dwUnionChoice = WTD_CHOICE_FILE; // 验证文件类型
	trustData.pFile = &fileInfo;

	// 指定验证策略（通用签名验证）
	GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	// 执行验证
	LONG lStatus = WinVerifyTrust(
		NULL,          // 不指定窗口句柄
		&policyGUID,   // 验证策略
		&trustData     // 信任数据
	);

	if (Config->DebugMessage) printf("ret-0x%x\n", GetLastError());
	// 清理可能的句柄（此处不需要）
	//trustData.dwStateAction = WTD_STATEACTION_CLOSE;
	//WinVerifyTrust(NULL, &policyGUID, &trustData);
	return (lStatus == ERROR_SUCCESS);
}


LPVOID MapViewOfProgram() {
	dbgout("internal_MapViewOfProgram : ");
	char selfPath[MAX_PATH];
	GetModuleFileNameA(nullptr, selfPath, MAX_PATH);
	HANDLE hFile = CreateFileA(selfPath, GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) { if (Config->DebugMessage) printf("CreateFile failed - 0x%x", GetLastError()); return 0; }

	// 将文件映射到内存
	HANDLE hMapping = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!hMapping) {
		CloseHandle(hFile);
		if (Config->DebugMessage) printf("CreateFileMapping failed - 0x%x\n", GetLastError());
		return 0;
	}

	LPVOID pData = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (!pData) {
		CloseHandle(hMapping);
		CloseHandle(hFile);
		if (Config->DebugMessage) printf("MapViewOfFile failed - 0x%x\n", GetLastError());
		return 0;
	}

	DWORD fileSize = GetFileSize(hFile, nullptr);
	LPVOID buf = malloc(fileSize + 1);
	RtlZeroMemory(buf, fileSize + 1);
	memcpy(buf, pData, fileSize);
	UnmapViewOfFile(pData);
	if (Config->DebugMessage) printf("ret=0x%x\n", GetLastError());
	CloseHandle(hMapping);
	CloseHandle(hFile);

	return buf;
}

LPCSTR MySHA256() {
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
	LPCSTR sha = CalculateSHA256Hex(pData, fileSize);

	UnmapViewOfFile(pData);
	CloseHandle(hMapping);
	CloseHandle(hFile);

	return sha;
}


LPCSTR File_SHA256Hex(LPCSTR target) {
	HANDLE hFile = CreateFileA(target, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) return 0;
	DWORD dSize = GetFileSize(hFile, 0);
	if (!dSize) {
		CloseHandle(hFile);
		return 0;
	}

	LPVOID buf = malloc(dSize);
	if (!ReadFile(hFile, buf, dSize, 0, 0)) {
		free(buf);
		CloseHandle(hFile);
		return 0;
	}

	LPCSTR sha = CalculateSHA256Hex(buf, dSize);
	free(buf);
	CloseHandle(hFile);

	return sha;
}

using namespace std;

bool iequals(const std::string& a, const std::string& b) {
	if (a.length() != b.length()) return false;
	for (size_t i = 0; i < a.length(); ++i) {
		if (std::tolower(static_cast<unsigned char>(a[i])) !=
			std::tolower(static_cast<unsigned char>(b[i]))) {
			return false;
		}
	}
	return true;
}

// 递归收集文件路径
void CollectFiles(const char* dir, const char* extension, BOOL subFolder,
	std::vector<std::string>& fileList) {
	char searchPath[MAX_PATH];
	WIN32_FIND_DATAA findData;

	// 构建搜索路径
	sprintf_s(searchPath, MAX_PATH, "%s\\*", dir);

	HANDLE hFind = FindFirstFileA(searchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE) return;

	do {
		// 跳过 "." 和 ".." 目录
		if (strcmp(findData.cFileName, ".") == 0 ||
			strcmp(findData.cFileName, "..") == 0) {
			continue;
		}

		// 构建完整路径
		char fullPath[MAX_PATH];
		sprintf_s(fullPath, MAX_PATH, "%s\\%s", dir, findData.cFileName);

		// 处理目录
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (subFolder) {
				CollectFiles(fullPath, extension, subFolder, fileList);
			}
		}
		// 处理文件
		else {
			// 提取文件后缀
			const char* dot = strrchr(findData.cFileName, '.');
			if (dot) {
				// 检查后缀匹配
				if (iequals(dot + 1, extension)) {
					fileList.push_back(fullPath);
				}
			}
			// 处理无后缀文件
			else if (extension[0] == '\0') {
				fileList.push_back(fullPath);
			}
		}
	} while (FindNextFileA(hFind, &findData));

	//dbgout("EnumFiles() recursion completed.\n");
	FindClose(hFind);
}

DWORD EnumFiles(const char* dir, const char* extension, LPCSTR* buffer, BOOL subFolder) {
	dbgout("EnumFiles() was called.\n");
	std::vector<std::string> fileList;

	// 收集所有匹配的文件路径
	CollectFiles(dir, extension, subFolder, fileList);

	// 分配结果缓冲区
	*buffer = new CHAR[fileList.size() * MAX_PATH];
	CHAR* current = const_cast<CHAR*>(*buffer); // 去除const以便写入

	// 将文件路径复制到缓冲区
	for (const auto& path : fileList) {
		strcpy_s(current, MAX_PATH, path.c_str());
		current += strlen(current) + 1; // 移动到下一个位置
	}
	*current = '\0'; // 设置双重结束符

	return static_cast<DWORD>(fileList.size());
}
