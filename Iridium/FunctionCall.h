#include <Windows.h>
#include "Structures.h"
#include "ntapiCaller.h"


//Init.cpp
DWORD __procCmdline();
DWORD __preInit();
DWORD __Init();

//Checker.cpp
DWORD __EnvironmentCheck();

//Service.cpp
BOOL InstallService(LPCSTR svcName, LPCSTR dispName, LPCSTR lpFilePath, LPCSTR lpDesc, DWORD svcType, DWORD svcLaunchMethod, DWORD svcErrorLevel); //安装指定的服务
BOOL UnInstallService(LPCSTR svcName); //删除指定的服务
BOOL LaunchService(LPCSTR svcName); //启动指定的服务
BOOL StopService(LPCSTR svcName); //停止指定的服务
BOOL SetServiceDescription(LPCSTR, LPCSTR); //设置指定服务描述
DWORD GetServiceStatus(LPCSTR svcName); //获取服务状态
BOOL UnloadDriverWithName(LPCWSTR drvFileName);

//DriverControl.cpp
DWORD __DriverInit();

//Registry.cpp
HKEY Registry_OpenKey(LPCSTR Path, HKEY HKey);
char* Registry_ReadStringKey(HKEY Hkey, LPCSTR Path, LPCSTR Subkey);
BOOL Registry_WriteStringKey(HKEY Hkey, LPCSTR Path, LPCSTR Subkey, LPCSTR value);
BOOL Registry_DeleteKey(HKEY HKey, LPCSTR Path, LPCSTR Subkey);
BOOL Registry_WriteDwordKey(HKEY Hkey, LPCSTR Path, LPCSTR Subkey, DWORD value);

//Console.cpp
void printC(const char* msg, int color); //输出彩字
void printA(const char* msg, int info); //输出日志
void printCr(const char* msg, int color); //输出彩字 (覆盖)
void printAr(const char* msg, int info); //输出日志 (覆盖)
void printAE(const char* msg, int color); //输出错误信息(自动换行
void dbgout(const char* format, ...);
void cls(); //清屏

//FileIO.cpp
LPCSTR File_ReadFileFull(LPCSTR); //读取文件全部数据
BOOL File_WriteFile(LPCSTR FileName, LPCSTR Info); //写入文件(直接覆盖)
BOOL File_WriteFileAttach(LPCSTR FileName, LPCSTR Info); //写入文件(追加模式)
BOOL FileExistsStatus(LPCSTR path);
char* CreateFileSelectDlg(const char* Title);
BOOL CheckIfExecutable(const char* Path);
char* GetFilePublisher(LPCSTR Path);
LPCSTR ReadConfigString(LPCSTR Path, LPCSTR Section, LPCSTR Key);
BOOL ReadConfigBoolean(LPCSTR Path, LPCSTR Section, LPCSTR Key);
DWORD ReadConfigDword(LPCSTR Path, LPCSTR Section, LPCSTR Key);
BOOL WriteConfigString(LPCSTR Path, LPCSTR Section, LPCSTR Key, LPCSTR Data);
BOOL WriteConfigBoolean(LPCSTR Path, LPCSTR Section, LPCSTR Key, BOOL b);
BOOL WriteConfigInt(LPCSTR Path, LPCSTR Section, LPCSTR Key, DWORD v);
BOOL VerifyDigitalSignature(LPCSTR lpPath);
LPVOID MapViewOfProgram();
LPCSTR MySHA256();
LPCSTR File_SHA256Hex(LPCSTR target);
DWORD EnumFiles(const char* dir, const char* extension, LPCSTR* buffer, BOOL subFolder);

//MessageFunction.cpp
LPCSTR StrConnect(LPCSTR a, LPCSTR b);
BOOL StrCompare(LPCSTR a, LPCSTR b);
void StrSep(const char* input, char delimiter, char*** output, int* count);
wchar_t* charToWchar(const char* c);
char* TranslateGetLastErrorMsg(DWORD code);
BOOL __RandomSeedInit(ULONGLONG* Ptr);
char* RandomTextGenerate(size_t length); 
DWORD GenerateRandomNumber(DWORD min, DWORD max);
LPCSTR CalculateSHA256Hex(LPCVOID pData, DWORD dwDataLen);
LPCSTR ResolveTempShortPath(LPCSTR t);
LPCSTR StrRemove(PVOID addr, ULONG len, LPCSTR str);
BOOL IsMemoryReadable(void* address, size_t size);
BOOL ConvertDosPathToNtPath(const wchar_t* dosPath, wchar_t* ntPath, DWORD ntPathSize);
BOOL IsSystemProcess(LPCWSTR str);
BOOL ConvertSystemPathToFullPath(const wchar_t* systemPath, wchar_t* fullPath, DWORD fullPathSize);
BYTE* HexStringToBytesDynamic(LPCSTR hexStr, int* outLength);

//System.cpp
VERSION_INFO GetVersionProc(void);
char* GetSystemTime();
char* GetSystemTimeFull();
BOOL EnableNoSignedDriverLoad();
BOOL DisableNoSignedDriverLoad();
BOOL IsRunAsAdmin();
//System-Rtl
LUID RtlConvertUlongToLuid(_In_ ULONG Ulong);

//Nt*
NTSTATUS NtDeviceIoControlFile(HANDLE hDev, HANDLE Event, PVOID apc, PVOID apcc, PIO_STATUS_BLOCK io, ULONG code, PVOID bin, ULONG sbin, PVOID bout, ULONG sbout);


//ExceptionHandler.cpp
void SetupCrashReporting();
LONG WINAPI PreExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);


//RTCoreEngine.cpp
DWORD __RTCoreInit(LPCSTR IrDrvPath, LPCSTR HelperDrvPath);
DWORD __RTCoreUninit(LPCSTR HelperDrvPath);

//Process.cpp
DWORD CreateProcessNew(LPCSTR path, LPCSTR cmdline);
BOOL SetPrivilege(HANDLE hProcess, LPCWSTR privilege);
BOOL RtlAdjustCurrentPrivilege(DWORD Privilege);

//GUI.cpp
HWND __AssembleMainGui();
HWND __AssembleSetupGUI();
HWND __AssembleIIDGUI();
HWND PopupListWindow(LPCSTR title, DWORD x, DWORD y, PVOID DataStringBase, PVOID ValDataBase, DWORD dcount);
HWND PopupInputWindow(LPCSTR title, LPCSTR inst, DWORD x, DWORD y, PVOID buffer);
void __GUI_Summary();

//ModuleManager.cpp
DWORD __InitUtils();
DWORD 	__Utils_Free();
DWORD __ThirdPartyModulesInit();

//DriverIoFunction.cpp
BOOL DeleteFileForce(LPCWSTR path);
BOOL DrvUnlockFile(LPCWSTR path);


//Debug.cpp
void Crash_AccessViolation();

//UIAccess.cpp
DWORD PrepareForUIAccess();

//WatchDog.cpp
DWORD __WatchDogInit();