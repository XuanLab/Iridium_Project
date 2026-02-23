#include <ntddk.h>
#include "fxCall.h"
#include "Define.h"

UNICODE_STRING DeviceName, SymbolLink;
PDRIVER_OBJECT IrDrvObj;

void IridiumUnload(PDRIVER_OBJECT DriverObject);

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS Stat = STATUS_FAILED_DRIVER_ENTRY;

	DbgPrint("[IRIDIUM] Initializing Iridium Kernel Mode Driver...\n");
	Stat = __DrvInit(DriverObject);
	
	//Config Routines
	IrDrvObj = DriverObject;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;

	PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;

	if (!NT_SUCCESS(DeleteFile(&pLdr->FullDllName))) {
		DbgPrint(("[IRIDIUM] Delete driver file failed with 0x%x.\n"), Stat);
	}
	else {
		DbgPrint("[IRIDIUM] Deleted driver file.\n");
	}

	((PLDR_DATA)DriverObject->DriverSection)->Flags |= 0x20; //¹ýµôMmVerifyCallbackFunction

	Stat = __ProtectClientByRegisterCallback();

	return Stat;
}

void IridiumUnload(PDRIVER_OBJECT DriverObject) {
	NTSTATUS Status = STATUS_FAIL_CHECK;

	DbgPrint("[IRIDIUM] Iridium is unloading...\n");

	UnRegNotifyCallbacks();

	Status = IoDeleteSymbolicLink(&SymbolLink);
	IoDeleteDevice(DriverObject->DeviceObject);


}
