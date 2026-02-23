#include <Windows.h>
#include "FunctionCall.h"

HKEY Registry_OpenKey(LPCSTR Path ,HKEY HKey) {
	HKEY hKey = 0;
	LSTATUS Stat = ERROR_SUCCESS;
	Stat = RegOpenKeyExA(HKey, Path, 0, KEY_READ | KEY_WRITE, &hKey);

	if (Stat == ERROR_SUCCESS) {
		return hKey;
	}
	else {
		RegCreateKeyA(HKey, Path, &hKey);
		return hKey;
	}
}  //打开注册表项目

char* Registry_ReadStringKey(HKEY Hkey,LPCSTR Path,LPCSTR Subkey) {
	HKEY hKey = Registry_OpenKey(Path, Hkey);
	if ((int)hKey == -1) {
		return nullptr;
	}
	DWORD len = 0;
	char* Data = {0};
	LSTATUS Stat = RegQueryValueExA(hKey, Subkey, 0, 0, (BYTE*)Data, &len);
	Data = (char*)malloc(len);
	Stat = RegQueryValueExA(hKey, Subkey, 0, 0, (BYTE*)Data, &len);
	RegCloseKey(hKey);
	return Data;

} //读取注册表值(字符串)

BOOL Registry_WriteStringKey(HKEY Hkey, LPCSTR Path, LPCSTR Subkey, LPCSTR value) {
	HKEY hKey = Registry_OpenKey(Path, Hkey);
	if ((int)hKey == -1) {
		return false;
	}
	LSTATUS Stat = RegSetValueExA(hKey, Subkey, 0, REG_SZ, (const BYTE*)value, (strlen(value)+1)*sizeof(LPCSTR));
	RegCloseKey(hKey);
	return Stat == ERROR_SUCCESS;
}

BOOL Registry_WriteDwordKey(HKEY Hkey, LPCSTR Path, LPCSTR Subkey, DWORD value) {
	HKEY hKey = Registry_OpenKey(Path, Hkey);
	if ((int)hKey == -1) {
		return false;
	}
	LSTATUS Stat = RegSetValueExA(hKey, Subkey, 0, REG_DWORD_BIG_ENDIAN, (LPBYTE)&value, sizeof(value));
	RegCloseKey(hKey);
	return Stat == ERROR_SUCCESS;
}

BOOL Registry_DeleteKey(HKEY HKey,LPCSTR Path, LPCSTR Subkey) {
	HKEY hKey = Registry_OpenKey(Path, HKey);
	if ((int)hKey == -1) {
		return false;
	}
	LSTATUS Stat = RegDeleteValueA(hKey, Subkey);
	RegCloseKey(hKey);
	return Stat == ERROR_SUCCESS;
}