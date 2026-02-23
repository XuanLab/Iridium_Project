#include <ntifs.h>
#include <ntddk.h>
#include "Define.h"
#include "fxCall.h"

NTSTATUS DriverCreate(PDEVICE_OBJECT pdo, PIRP Irp) { //仅允许一个被打开的的句柄
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;

}

NTSTATUS DriverClose(PDEVICE_OBJECT pdo, PIRP Irp) {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

ULONG ClientPid = 0;
BOOLEAN ProtectClient = 0;

NTSTATUS HandleFileOperation(PIRIDIUM_OPERATION_S operation);
NTSTATUS HandlePHOperation(PIRIDIUM_OPERATION_S operation);


NTSTATUS DriverDeviceControl(PDEVICE_OBJECT pdo, PIRP Irp) {
	NTSTATUS Stat = STATUS_SUCCESS; //默认为成功

	PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

	ULONG len = 0; //返回的长度
	ULONG BufferSizeIn = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength; //获取缓冲区长度
	ULONG BufferSizeOut = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;

	switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
	{
	case IRIDIUM_INIT_INST: {
		if (BufferSizeIn % sizeof(IRIDIUM_INIT) != 0) {
			Stat = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		IRIDIUM_INIT* init = (IRIDIUM_INIT*)Buffer;
        DbgPrint(("[IRIDIUM] Received initialize data. Pkg provided SHA256=%s\n"), init->sha256);
        do {
			if (!VerifyPackageSha256(init, offsetof(IRIDIUM_INIT, sha256), init->sha256)) {
				DbgPrint(("[IRIDIUM] Warning: The init pkg's SHA256 is invalid!\n"));
			}
			else {
				DbgPrint(("[IRIDIUM] Init pkg sha256 verification passed.\n"));
			}


        } while (0);
		ClientPid = init->ProcId;
        ProtectClient = init->Protect;
		break;
	}
	case IRIDIUM_COMMON_OPERATION: {
		if (BufferSizeIn % sizeof(IRIDIUM_OPERATION_S) != 0) {
			Stat = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		IRIDIUM_OPERATION_S* op = (IRIDIUM_OPERATION_S*)Buffer;
		DbgPrint(("[IRIDIUM] Received common operation data. Pkg provided SHA256=%s\n"), op->sha256);

		do {
			if (!VerifyPackageSha256(op, offsetof(IRIDIUM_OPERATION_S, sha256), op->sha256)) {
				DbgPrint(("[IRIDIUM] Warning: The operation pkg's SHA256 is invalid!\n"));
			}
			else {
				DbgPrint(("[IRIDIUM] Operation pkg sha256 verification passed.\n"));
			}
		} while (0);

        switch (op->MajorFuncId) {
        case FileIo:
            Stat = HandleFileOperation(op);
            break;
        case ProcessHandleOpt:
            Stat = HandlePHOperation(op);
            break;
        default:
            DbgPrint("[IRIDIUM] Unknown major function: %lu\n", op->MajorFuncId);
            Stat = STATUS_NOT_IMPLEMENTED;
            break;
        }

        // 将操作状态写回结构体
        op->Stat = Stat;
        len = sizeof(IRIDIUM_OPERATION_S);
        break;
	}
    case IRIDIUM_DRV_UNLOAD: {
        DbgPrint("[IRIDIUM] Set driver unload function.\n");
        IrDrvObj->DriverUnload = IridiumUnload;
        break;
    }

	default: {
		Stat = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}
	Irp->IoStatus.Status = Stat;
	Irp->IoStatus.Information = len;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Stat;
}


NTSTATUS HandleFileOperation(PIRIDIUM_OPERATION_S operation) {
    NTSTATUS status = STATUS_SUCCESS;

    switch (operation->MinorFuncId) {
    case FileOperationDeleteForce: {
        if (operation->Datalen < sizeof(WCHAR) || operation->Datalen > sizeof(operation->buf)) {
            DbgPrint("[IRIDIUM] Invalid data length for file delete: %lu\n", operation->Datalen);
            return STATUS_INVALID_PARAMETER;
        }

        PWCHAR filePath = (PWCHAR)operation->buf;

        DbgPrint("[IRIDIUM] Attempting to force delete: %ws\n", filePath);
        UNICODE_STRING file;
        RtlInitUnicodeString(&file, filePath);
        status = DeleteFile(&file);
        if (!NT_SUCCESS(status)) {
            DbgPrint("[IRIDIUM] Force delete file %ws failed with error : 0x%x\n", filePath, status);
        }
        break;
    }
	case FileOperationUnlock: {
        if (operation->Datalen < sizeof(WCHAR) || operation->Datalen > sizeof(operation->buf)) {
            DbgPrint("[IRIDIUM] Invalid data length for file unlock: %lu\n", operation->Datalen);
            return STATUS_INVALID_PARAMETER;
        }

        PWCHAR filePath = (PWCHAR)operation->buf;

        DbgPrint("[IRIDIUM] Attempting to unlock: %ws\n", filePath);
        UNICODE_STRING file;
        RtlInitUnicodeString(&file, filePath);
        status = UnlockFile(&file);
        if (!NT_SUCCESS(status)) {
            DbgPrint("[IRIDIUM] Unlock file %ws failed with error : 0x%x\n", filePath, status);
        }
        break;
	}

    case FileOperationDelete:
        // 实现普通删除逻辑
        DbgPrint("[IRIDIUM] File delete operation requested\n");
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case FileOperationRead:
        // 实现文件读取逻辑
        DbgPrint("[IRIDIUM] File read operation requested\n");
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case FileOperationWrite:
        // 实现文件写入逻辑
        DbgPrint("[IRIDIUM] File write operation requested\n");
        status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        DbgPrint("[IRIDIUM] Unknown file operation: %lu\n", operation->MinorFuncId);
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    return status;
}

NTSTATUS HandlePHOperation(PIRIDIUM_OPERATION_S operation) {
    NTSTATUS status = STATUS_SUCCESS;
    switch (operation->MinorFuncId)
    {
    case HandleModifyAccess: {
        if (operation->Datalen % sizeof(HANDLE_INFO) != 0) {
            return STATUS_INVALID_BUFFER_SIZE;
        }

        PHANDLE_INFO info = (PHANDLE_INFO)operation->buf;

        

        break;
    }
    default:
        break;
    }
}