#pragma once
#include <ntddk.h>

//func
#define offsetof(s,m) ((::size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))

//IoCtrl Block
#define IRIDIUM_INIT_INST CTL_CODE(FILE_DEVICE_UNKNOWN,0x1001,METHOD_BUFFERED,FILE_ALL_ACCESS)
#define IRIDIUM_COMMON_OPERATION CTL_CODE(FILE_DEVICE_UNKNOWN,0x1002,METHOD_BUFFERED,FILE_ALL_ACCESS)
#define IRIDIUM_DRV_UNLOAD CTL_CODE(FILE_DEVICE_UNKNOWN,0x1003,METHOD_BUFFERED,FILE_ALL_ACCESS)

//Status Define Block
//0xaadddddd aa:00=SUCCESS ff=FAILURE

#define IR_ERROR_INIT_INCOMPLETE 0xff000001 //Driver Init in-complete

//Structure Define Block

typedef unsigned char BYTE; //UNSIGNED CHAR

typedef struct _IRIDIUM_INIT {
    ULONG ProcId;

    BOOLEAN Protect;
    BOOLEAN ProtectEx;

    char sha256[64]; //结构体的SHA256值
} IRIDIUM_INIT, * PIRIDIUM_INIT;

typedef enum IRIDIUM_OPERATIONS {
    FileIo,
    ProcessHandleOpt
};

typedef enum IRIDIUM_FILE_OPERATIONS {
    FileOperationUnlock,
    FileOperationDelete,
    FileOperationDeleteForce,
    FileOperationRead,
    FileOperationWrite
};

typedef enum IRIDIUM_PROCESS_HANDLE_OPERATIONS {
    HandleModifyAccess
};
typedef struct _IRIDIUM_OPERATION_S {
    ULONG MajorFuncId;
    ULONG MinorFuncId;
    ULONG p1;
    ULONG p2;
    BYTE buf[4096 + 128];
    ULONG Datalen;
    NTSTATUS Stat;
    char sha256[64]; //结构体的SHA256值
}IRIDIUM_OPERATION_S, * PIRIDIUM_OPERATION_S;

typedef struct _OFFSET_DESCRIPTOR {
	ULONG EPROCESS_UniqueProcessId;
}OFFSET_DESCRIPTOR, * POFFSET_DESCRIPTOR;


#ifdef _WIN64
typedef struct _LDR_DATA {
	LIST_ENTRY listEntry;//16
	ULONG64 __Undefined1;//8
	ULONG64 __Undefined2;//8
	ULONG64 __Undefined3;//8
	ULONG64 NonPagedDebugInfo;//8
	ULONG64 DllBase;//8
	ULONG64 EntryPoint;//8
	ULONG SizeOfImage;//4
	UNICODE_STRING path;//16
	UNICODE_STRING name;//16
	ULONG   Flags;
}LDR_DATA, * PLDR_DATA;
#else
typedef struct _LDR_DATA {
	LIST_ENTRY listEntry;
	ULONG unknown1;
	ULONG unknown2;
	ULONG unknown3;
	ULONG unknown4;
	ULONG unknown5;
	ULONG unknown6;
	ULONG unknown7;
	UNICODE_STRING path;
	UNICODE_STRING name;
	ULONG   Flags;
}LDR_DATA, * PLDR_DATA;
#endif

typedef struct _LDR_DATA_TABLE_ENTRY32
{
    LIST_ENTRY32 InLoadOrderLinks;
    LIST_ENTRY32 InMemoryOrderModuleList;
    LIST_ENTRY32 InInitializationOrderModuleList;
    ULONG DllBase;
    ULONG EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING32 FullDllName;
    UNICODE_STRING32 BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union
    {
        LIST_ENTRY32 HashLinks;
        ULONG SectionPointer;
    }u1;
    ULONG CheckSum;
    union
    {
        ULONG TimeDateStamp;
        ULONG LoadedImports;
    }u2;
    ULONG EntryPointActivationContext;
    ULONG PatchInformation;
} LDR_DATA_TABLE_ENTRY32, * PLDR_DATA_TABLE_ENTRY32;

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY64    InLoadOrderLinks;
    LIST_ENTRY64    InMemoryOrderLinks;
    LIST_ENTRY64    InInitializationOrderLinks;
    PVOID           DllBase;
    PVOID           EntryPoint;
    ULONG           SizeOfImage;
    UNICODE_STRING  FullDllName;
    UNICODE_STRING  BaseDllName;
    ULONG           Flags;
    USHORT          LoadCount;
    USHORT          TlsIndex;
    PVOID           SectionPointer;
    ULONG           CheckSum;
    PVOID           LoadedImports;
    PVOID           EntryPointActivationContext;
    PVOID           PatchInformation;
    LIST_ENTRY64    ForwarderLinks;
    LIST_ENTRY64    ServiceTagLinks;
    LIST_ENTRY64    StaticLinks;
    PVOID           ContextInformation;
    ULONG64         OriginalBase;
    LARGE_INTEGER   LoadTime;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

typedef struct _HANDLE_INFO
{
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT  HandleValue;
    ULONG GrantedAccess;
    ULONG64 Object;
    UCHAR Name[256];
} HANDLE_INFO, * PHANDLE_INFO;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
    USHORT  UniqueProcessId;
    USHORT  CreatorBackTraceIndex;
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT  HandleValue;
    PVOID Object;
    ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG64 NumberOfHandles;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;


typedef struct _OBJECT_BASIC_INFORMATION
{
    ULONG                   Attributes;
    ACCESS_MASK             DesiredAccess;
    ULONG                   HandleCount;
    ULONG                   ReferenceCount;
    ULONG                   PagedPoolUsage;
    ULONG                   NonPagedPoolUsage;
    ULONG                   Reserved[3];
    ULONG                   NameInformationLength;
    ULONG                   TypeInformationLength;
    ULONG                   SecurityDescriptorLength;
    LARGE_INTEGER           CreationTime;
} OBJECT_BASIC_INFORMATION, * POBJECT_BASIC_INFORMATION;

typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING          TypeName;
    ULONG                   TotalNumberOfHandles;
    ULONG                   TotalNumberOfObjects;
    WCHAR                   Unused1[8];
    ULONG                   HighWaterNumberOfHandles;
    ULONG                   HighWaterNumberOfObjects;
    WCHAR                   Unused2[8];
    ACCESS_MASK             InvalidAttributes;
    GENERIC_MAPPING         GenericMapping;
    ACCESS_MASK             ValidAttributes;
    BOOLEAN                 SecurityRequired;
    BOOLEAN                 MaintainHandleCount;
    USHORT                  MaintainTypeList;
    POOL_TYPE               PoolType;
    ULONG                   DefaultPagedPoolCharge;
    ULONG                   DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_HANDLE_FLAG_INFORMATION
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
}OBJECT_HANDLE_FLAG_INFORMATION, * POBJECT_HANDLE_FLAG_INFORMATION;

typedef struct _LDR_DATA_TABLE_ENTRY64
{
    LIST_ENTRY64 InLoadOrderLinks;
    LIST_ENTRY64 InMemoryOrderLinks;
    LIST_ENTRY64 InInitializationOrderLinks;
    ULONG64 DllBase;
    ULONG64 EntryPoint;
    ULONG64 SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY64 HashLinks;
    ULONG64 SectionPointer;
    ULONG64 CheckSum;
    ULONG64 TimeDateStamp;
    ULONG64 LoadedImports;
    ULONG64 EntryPointActivationContext;
    ULONG64 PatchInformation;
    LIST_ENTRY64 ForwarderLinks;
    LIST_ENTRY64 ServiceTagLinks;
    LIST_ENTRY64 StaticLinks;
    ULONG64 ContextInformation;
    ULONG64 OriginalBase;
    LARGE_INTEGER LoadTime;
} LDR_DATA_TABLE_ENTRY64, * PLDR_DATA_TABLE_ENTRY64;

// -------------------------------------------------------
// 导出函数定义
// -------------------------------------------------------

extern "C" NTKERNELAPI NTSTATUS ObSetHandleAttributes
(
    HANDLE Handle,
    POBJECT_HANDLE_FLAG_INFORMATION HandleFlags,
    KPROCESSOR_MODE PreviousMode
);

extern "C" NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation
(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG  ReturnLength
);

extern "C" NTSYSAPI NTSTATUS NTAPI ZwDuplicateObject
(
    HANDLE    SourceProcessHandle,
    HANDLE    SourceHandle,
    HANDLE    TargetProcessHandle OPTIONAL,
    PHANDLE   TargetHandle OPTIONAL,
    ACCESS_MASK DesiredAccess,
    ULONG   HandleAttributes,
    ULONG   Options
);

extern "C" NTSYSAPI NTSTATUS NTAPI ZwOpenProcess
(
    PHANDLE       ProcessHandle,
    ACCESS_MASK     AccessMask,
    POBJECT_ATTRIBUTES  ObjectAttributes,
    PCLIENT_ID      ClientId
);

//g_Var Define Block
extern PDRIVER_OBJECT IrDrvObj;
extern OFFSET_DESCRIPTOR offset_desc;
extern UNICODE_STRING DeviceName, SymbolLink;

extern ULONG ClientPid;
extern BOOLEAN ProtectClient;


//APIs
extern "C" NTKERNELAPI NTSTATUS PsSuspendProcess(PEPROCESS proc);    //暂停进程
extern "C" NTKERNELAPI NTSTATUS PsResumeProcess(PEPROCESS proc);    //恢复进程
extern "C" NTKERNELAPI HANDLE PsGetProcessInheritedFromUniqueProcessId(IN PEPROCESS Process); //未公开进行导出 获取父进程Pid
extern "C" NTKERNELAPI NTSTATUS ZwQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
extern "C" NTKERNELAPI NTSTATUS ZwSetInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength);
extern "C" NTKERNELAPI PVOID PsGetProcessPeb(_In_ PEPROCESS Process);
extern "C" NTKERNELAPI BOOLEAN PsGetProcessWow64Process(PEPROCESS Process);
//extern "C" _Kernel_entry_ NTSTATUS NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);