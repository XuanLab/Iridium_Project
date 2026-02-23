#include <ntifs.h>
#include "Define.h"
#include "fxCall.h"

OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID, POB_PRE_OPERATION_INFORMATION Info) {
	if (ProtectClient) {
		PEPROCESS Process = (PEPROCESS)Info->Object;
		ULONG Pid = HandleToULong(PsGetProcessId(Process));
		if ((Pid == ClientPid) && ProtectClient) {
			Info->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_ALL_ACCESS;
		}
	}
	return OB_PREOP_SUCCESS;
}


void OnPreProcessCreateCallbackEx(HANDLE ParentProc, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO Info) {
	if (Info == NULL) { //进程退出通知
		if ((ULONG)ProcessId == ClientPid) {
			DbgPrint("[IRIDIUM] Client process exited.\n");
		}
	}
	else { //进程创建通知

	}
}



OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID, POB_PRE_OPERATION_INFORMATION Info);
void OnPreProcessCreateCallbackEx(HANDLE ParentProc, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO Info);

PVOID ProcessNotifyCallbackHandle;
BOOLEAN ProcessCreateNotifyStat = 0;
BOOLEAN ProcessNotifyCallbackStat = 0;

void UnRegNotifyCallbacks(void) {
	if (ProcessNotifyCallbackStat) ObUnRegisterCallbacks(ProcessNotifyCallbackHandle);
	if (ProcessCreateNotifyStat) PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)OnPreProcessCreateCallbackEx, TRUE);
}


NTSTATUS __ProtectClientByRegisterCallback() {
	DbgPrint("[IRIDIUM] Registering callbacks...\n");
	NTSTATUS Status = STATUS_ACCESS_DENIED;
	//初始化注册信息 （注册进程通知回调 开始)
	OB_CALLBACK_REGISTRATION ObCallbackReg = { 0 };
	OB_OPERATION_REGISTRATION ObOperationReg = { 0 };
	RtlZeroMemory(&ObCallbackReg, sizeof(ObCallbackReg));
	RtlZeroMemory(&ObOperationReg, sizeof(ObOperationReg));

	//初始化ObCallbackReg
	ObCallbackReg.Version = OB_FLT_REGISTRATION_VERSION;
	ObCallbackReg.Altitude = RTL_CONSTANT_STRING(L"439999.19491001");
	ObCallbackReg.OperationRegistrationCount = 1;
	ObCallbackReg.RegistrationContext = nullptr;
	ObCallbackReg.OperationRegistration = &ObOperationReg;

	//初始化ObOperationReg
	ObOperationReg.ObjectType = PsProcessType;
	ObOperationReg.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	ObOperationReg.PreOperation = OnPreOpenProcess;

	Status = ObRegisterCallbacks(&ObCallbackReg, &ProcessNotifyCallbackHandle);
	ProcessNotifyCallbackStat = (Status == STATUS_SUCCESS);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("[IRIDIUM] Register ObRegisterCallbacks failed with 0x%x.\n", Status);
		return Status;
	}

	Status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)OnPreProcessCreateCallbackEx, FALSE);
	ProcessCreateNotifyStat = NT_SUCCESS(Status);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("[IRIDIUM] Register CreateProecssNotifyRoutineEx failed with 0x%x.\n", Status);
		return Status;
	}

	return STATUS_SUCCESS;
}
