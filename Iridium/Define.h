#pragma once
#include "ntapiCaller.h"
#define Cfg ".\\Iridium.ini"
#define IRIDIUM_VERSION "prototype-verification" 

#define IRIDIUM_KMODE_DRIVER_CONTAINED

//Iridium Checker Error Codes Define Section (0x00001fff)
#define IR_CHECK_SUCCESSFULLY 0x0
#define IR_CHECK_VERSION_OUTDATED 0x00001001
#define IR_CHECK_DEBUGGER_DETECTED 0x00001002
#define IR_CHECK_MEMORY_INSUFFICIENT 0x00001003
#define IR_CHECK_MODULE_CHECK_FAILURE 0x00001004

//Iridium Driver Error Codes Define Section (0x00003fff)
#define IR_DRIVER_SUCCESSFULLY 0x0
#define IR_DRIVER_SELECT_CANCELED 0x00003001
#define IR_DRIVER_SYMBOLLINK_ERROR 0x00003002
#define IR_DRIVER_POST_CFG_ERROR 0x00003003
#define IR_DRIVER_DRIVER_DIED 0x00003004

#define IR_DRIVER_STAT_BUSY 0x00003101


//Iridium Init Error Codes Define Section (00004fff)
#define IR_INIT_SUCCESSFULLY 0x0
#define IR_INIT_MEM_POOL_FAILURE 0x00004001
#define IR_INIT_THREAD_POOL_FAILURE 0x00004002
#define IR_INIT_CFG_NOT_FOUND 0x00004003
#define IR_INIT_UTILS_INIT_FAILURE 0x00004004
#define IR_INIT_API_NOT_FOUNT 0x00004005
#define IR_INIT_MODULE_FAILURE 0x00004006
#define IR_INIT_CREATE_SNAPSHOT_FAILURE 0x00004007

//func
#define IrIcon LoadIconW(GetModuleHandleA(0), MAKEINTRESOURCEW(IridiumIco))
#define GET_X_LPARAM LOWORD
#define GET_Y_LPARAM HIWORD


//全局变量
extern HINSTANCE hInstance;
extern ULONGLONG ProgramBootTick;
extern char* raname; //随机名称 禁用时为默认名
extern int CrashCount;

extern "C" DWORD DebuggerCheckEnabled;

extern LPCSTR cur_irDrv, cur_irhelpDrv;

//Iridium驱动程序服务名
extern LPCSTR drv_m_name;

extern HFONT UniversalFont;
extern HWND MainWindow;

extern DWORD64 Ci_Base;
extern DWORD gCiOffset;

extern FNtDeviceIoControlFile pNtDeviceIoControlFile;
extern FNtCreateFile pNtCreateFile;

extern PIRIDIUM_CONFIG Config;
extern LONGLONG HeartBeat;

//g_unpub_api
extern NtSetInformationThread fNtSetInformationThread;

//IOCTRL
#define IRIDIUM_INIT_INST CTL_CODE(FILE_DEVICE_UNKNOWN,0x1001,METHOD_BUFFERED,FILE_ALL_ACCESS)
#define IRIDIUM_DRV_UNLOAD CTL_CODE(FILE_DEVICE_UNKNOWN,0x1003,METHOD_BUFFERED,FILE_ALL_ACCESS)
#define IRIDIUM_COMMON_OPERATION CTL_CODE(FILE_DEVICE_UNKNOWN,0x1002,METHOD_BUFFERED,FILE_ALL_ACCESS)