#include <iostream>
#include <Windows.h>
#include "FunctionCall.h"
#include "Define.h"
#include "Structures.h"

BOOL InstallService(LPCSTR svcName, LPCSTR dispName, LPCSTR lpFilePath, LPCSTR lpDesc, DWORD svcType, DWORD svcLaunchMethod, DWORD svcErrorLevel) {
	dbgout("internal_InstallService : ");
	char szFilePath[MAX_PATH] = { 0 };
	SC_HANDLE hSCManager = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
	SC_HANDLE hSvc = 0;
	BOOL u = 0;
	do {
		if (hSCManager == NULL) {
			if (Config->DebugMessage) printf("OpenSCManager failed - 0x%x\n", GetLastError());
			u = 1;
			break;
		}
		GetFullPathNameA(lpFilePath, MAX_PATH, szFilePath, NULL);
		hSvc = CreateServiceA(hSCManager, svcName, dispName, SERVICE_ALL_ACCESS, svcType, svcLaunchMethod, svcErrorLevel, szFilePath, NULL, NULL, NULL, NULL, NULL);
		if (hSvc == NULL) {
			if (Config->DebugMessage) printf("CreateService failed - 0x%x\n", GetLastError());
			u = 1;
			break;
		}
		SetServiceDescription(svcName, lpDesc);
	} while (0);
	CloseServiceHandle(hSvc);
	CloseServiceHandle(hSCManager);
	if (Config->DebugMessage && !u) printf("OK\n");
	return hSvc != 0;
}

BOOL LaunchService(LPCSTR svcName) {
	dbgout("internal_LaunchService : ");
	SC_HANDLE hSCManager = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
	BOOL ret = 0;
	BOOL u = 0;
	SC_HANDLE hSvc = 0;
	do {
		if (hSCManager == NULL) {
			if (Config->DebugMessage) printf("OpenSCManager failed - 0x%x\n", GetLastError());
			u = 1;
			break;
		}
		hSvc = OpenServiceA(hSCManager, svcName, SERVICE_ALL_ACCESS);
		if (hSvc == NULL) {
			if (Config->DebugMessage) printf("OpenService failed - 0x%x\n", GetLastError());
			u = 1;
			break;
		}
	} while (0);
	ret = StartServiceA(hSvc, 0, 0);
	CloseServiceHandle(hSvc);
	CloseServiceHandle(hSCManager);
	if (Config->DebugMessage && !u) printf("ret=0x%x\n",GetLastError());
	return ret;
}

BOOL StopService(LPCSTR svcName) {
	dbgout("internal_StopService : ");
	SC_HANDLE hSCManager = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
	SC_HANDLE hSvc = 0;
	BOOL ret = 0;
	BOOL u = 0;
	do {
		if (hSCManager == NULL) {
			if (Config->DebugMessage) printf("OpenSCManager failed - 0x%x\n", GetLastError());
			u = 1;
			break;
		}
		hSvc = OpenServiceA(hSCManager, svcName, SERVICE_ALL_ACCESS);
		if (hSvc == NULL) {
			if (Config->DebugMessage) printf("OpenService failed - 0x%x\n", GetLastError());
			u = 1;
			break;
		}
		SERVICE_STATUS Stat;
		ret = ControlService(hSvc, SERVICE_CONTROL_STOP, &Stat);
	} while (0);
	CloseServiceHandle(hSvc);
	CloseServiceHandle(hSCManager);
	if (Config->DebugMessage && !u) printf("ret=0x%x\n",GetLastError());
	return ret;
}

BOOL UnInstallService(LPCSTR svcName) {
    dbgout("internal_UninstallService : ");
    SC_HANDLE hSCManager = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE hSvc = 0;
    BOOL ret = 0;
    BOOL u = 0;
    do {
        if (hSCManager == NULL) {
            if (Config->DebugMessage) printf("OpenSCManager failed - 0x%x\n", GetLastError());
            u = 1;
            break;
        }
        hSvc = OpenServiceA(hSCManager, svcName, SERVICE_ALL_ACCESS);
        if (hSvc == NULL) {
            DWORD err = GetLastError();
            if (Config->DebugMessage) printf("OpenService failed - 0x%x\n", err);
            // 如果服务不存在，这不是错误，应该返回TRUE
            if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
                ret = TRUE;
            }
            u = 1;
            break;
        }
        ret = DeleteService(hSvc);
    } while (0);

    if (hSvc) {
        CloseServiceHandle(hSvc);
    }
    if (hSCManager) {
        CloseServiceHandle(hSCManager);
    }

    if (Config->DebugMessage && !u) printf("ret=0x%x\n", GetLastError());
    return ret;
}

BOOL SetServiceDescription(LPCSTR svcName, LPCSTR Description) {
	BOOL ret = 0;
	SC_HANDLE hSCMangager = OpenSCManagerA(0, 0, GENERIC_EXECUTE);
	SC_HANDLE hSvc = 0;
	do {
		if (hSCMangager == 0) {
			break;
		}
		hSvc = OpenServiceA(hSCMangager, svcName, SERVICE_CHANGE_CONFIG);
		if (hSvc == 0) {
			break;
		}
		ret = ChangeServiceConfig2A(hSvc, 1, &Description);
	} while (0);
	CloseServiceHandle(hSvc);
	CloseServiceHandle(hSCMangager);
	return ret;
}

DWORD GetServiceStatus(LPCSTR svcName) {
	dbgout("GetServiceStatus() was called,target service : %s\n", svcName);
	SC_HANDLE hSCMangager = OpenSCManagerA(0, 0, GENERIC_ALL);
	if (hSCMangager == 0) {
		dbgout("Failed to open sc manager. 0x%x\n", GetLastError());
		return -1;
	}
	SC_HANDLE hSvc = OpenServiceA(hSCMangager, svcName, SERVICE_QUERY_STATUS);
	if (hSvc == 0) {
		dbgout("Failed to open service. 0x%x\n", GetLastError());
		return -1;
	}

	SERVICE_STATUS Stat = { 0 };
	if (!QueryServiceStatus(hSvc, &Stat)) return -1;
	return Stat.dwCurrentState;
}

BOOL UnloadDriverWithName(LPCWSTR drvFileName) {
    SC_HANDLE scManager = NULL;
    SC_HANDLE serviceHandle = NULL;
    BOOL result = FALSE;
    DWORD bytesNeeded = 0;
    DWORD serviceCount = 0;
    LPENUM_SERVICE_STATUS services = NULL;

    // 打开服务控制管理器
    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);
    if (scManager == NULL) {
        return FALSE;
    }

    // 第一次调用，获取所需缓冲区大小
    EnumServicesStatus(scManager, SERVICE_DRIVER, SERVICE_STATE_ALL,
        NULL, 0, &bytesNeeded, &serviceCount, NULL);

    if (GetLastError() != ERROR_MORE_DATA) {
        CloseServiceHandle(scManager);
        return FALSE;
    }

    // 分配缓冲区
    services = (LPENUM_SERVICE_STATUS)LocalAlloc(LPTR, bytesNeeded);
    if (services == NULL) {
        CloseServiceHandle(scManager);
        return FALSE;
    }

    // 第二次调用，实际获取服务列表
    if (!EnumServicesStatus(scManager, SERVICE_DRIVER, SERVICE_STATE_ALL,
        services, bytesNeeded, &bytesNeeded, &serviceCount, NULL)) {
        LocalFree(services);
        CloseServiceHandle(scManager);
        return FALSE;
    }

    // 遍历所有驱动程序服务，查找匹配的文件名
    for (DWORD i = 0; i < serviceCount; i++) {
        SC_HANDLE currentService = OpenService(scManager, services[i].lpServiceName,
            SERVICE_QUERY_CONFIG | SERVICE_STOP | SERVICE_CHANGE_CONFIG);
        if (currentService != NULL) {
            DWORD bufferSize = 0;
            QueryServiceConfig(currentService, NULL, 0, &bufferSize);

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                LPQUERY_SERVICE_CONFIG serviceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LPTR, bufferSize);
                if (serviceConfig != NULL) {
                    if (QueryServiceConfigW(currentService, serviceConfig, bufferSize, &bufferSize)) {
                        // 检查驱动文件路径是否包含目标文件名
                        if (serviceConfig->lpBinaryPathName != NULL) {
							wchar_t buf[260] = { 0 };
							ConvertSystemPathToFullPath(serviceConfig->lpBinaryPathName, buf, 260);

                            std::wstring binaryPath(buf);
                            std::wstring targetFile(drvFileName);
                            // 在路径中查找文件名（不区分大小写）
                            if (binaryPath.find(targetFile) != std::wstring::npos) {

                                SERVICE_STATUS serviceStatus;

                                // 停止服务
                                if (ControlService(currentService, SERVICE_CONTROL_STOP, &serviceStatus)) {
                                    // 等待服务完全停止
                                    for (int waitCount = 0; waitCount < 50; waitCount++) {
                                        QueryServiceStatus(currentService, &serviceStatus);
                                        if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
                                            break;
                                        }
                                        Sleep(100); // 等待100ms
                                    }
									
                                    // 禁用服务（防止重启后自动启动）
                                    if (ChangeServiceConfig(currentService,
                                        SERVICE_NO_CHANGE,
                                        SERVICE_DISABLED,
                                        SERVICE_NO_CHANGE,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
                                        result = TRUE;
                                    }
                                }
                                else {
                                    DWORD error = GetLastError();
                                    if (error == ERROR_SERVICE_NOT_ACTIVE) {
                                        if (ChangeServiceConfig(currentService,
                                            SERVICE_NO_CHANGE,
                                            SERVICE_DISABLED,
                                            SERVICE_NO_CHANGE,
                                            NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
                                            result = TRUE;
                                        }
                                    }
                                    else {
                                    }
                                }

                                serviceHandle = currentService;
                                LocalFree(serviceConfig);
                                break;
                            }
                        }
                    }
                    LocalFree(serviceConfig);
                }
            }
            if (currentService != serviceHandle) {
                CloseServiceHandle(currentService);
            }
        }
    }

    // 清理资源
    if (services != NULL) {
        LocalFree(services);
    }
    if (serviceHandle != NULL) {
        CloseServiceHandle(serviceHandle);
    }
    if (scManager != NULL) {
        CloseServiceHandle(scManager);
    }
    return result;
}
