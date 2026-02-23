#define _CRT_SECURE_NO_WARNINGS 1
#include <Windows.h>
#include <iostream>
#include <Psapi.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <string>
#include <vector>
#include <map>
#include <tlhelp32.h>
#pragma comment (lib,"shlwapi.lib")
//#pragma comment (lib,"ucrtd.lib")
#pragma comment (lib, "comctl32.lib")
#include "resource.h"
#include "FunctionCall.h"
#include "Define.h"
#include "Structures.h"

//   主窗口          功能选择夹      进程列表      服务列表     网络选择夹   磁盘选择夹  文件列表
HWND MainWindow = 0, FuncSelect = 0, Tasklist = 0, Svclist = 0, Netlist = 0, Stlist = 0, Stflist = 0;
HMENU ProcMenu = 0, SvcMenu = 0;

// 窗口大小常量
#define WINDOW_MIN_WIDTH  800
#define WINDOW_MIN_HEIGHT 600
#define TAB_HEIGHT        21
#define MARGIN            5

LRESULT CALLBACK Window_Main(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
LONGLONG Window_Main_Msg_Cnt = 0;
LRESULT CALLBACK Window_Setup(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
LONGLONG Window_Setup_Msg_Cnt = 0;

//Widget Section
BOOL SetWindowOnTop(HWND hwnd) {
	return SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
}

// 调整子控件大小的函数
void AdjustChildControls(HWND hWnd)
{
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	int clientWidth = rcClient.right - rcClient.left;
	int clientHeight = rcClient.bottom - rcClient.top;

	// 调整主选择夹大小
	if (FuncSelect)
	{
		SetWindowPos(FuncSelect, NULL, 0, 0, clientWidth, clientHeight, SWP_NOZORDER);

		// 获取选择夹的客户区（用于放置内容页面）
		RECT rcTab;
		GetClientRect(FuncSelect, &rcTab);

		// 计算内容区域（减去标签栏高度）
		rcTab.top += TAB_HEIGHT;

		// 调整各个内容页面的大小
		HWND pages[] = { Tasklist, Svclist, Netlist, Stlist };
		for (int i = 0; i < 4; i++)
		{
			if (pages[i])
			{
				SetWindowPos(pages[i], NULL,
					rcTab.left, rcTab.top,
					rcTab.right - rcTab.left,
					rcTab.bottom - rcTab.top,
					SWP_NOZORDER);
			}
		}

		// 如果是存储标签页，还需要调整文件列表
		if (Stlist && Stflist)
		{
			RECT rcStlist;
			GetClientRect(Stlist, &rcStlist);

			// 计算Stlist的内容区域
			rcStlist.top += TAB_HEIGHT;

			if (Stflist)
			{
				SetWindowPos(Stflist, NULL,
					rcStlist.left, rcStlist.top,
					rcStlist.right - rcStlist.left,
					rcStlist.bottom - rcStlist.top,
					SWP_NOZORDER);
			}
		}
	}
}

//Window Section

#define Main_FuncSelect 1
#define Main_tsklist 2
#define Main_svclist 3
#define Main_netlist 4
#define Main_fslist 5
#define Main_fsflist 6

#define tasklist_menu_refresh 5820
#define tasklist_menu_terminateproc 5821
#define tasklist_menu_terminateprocf 5822
#define tasklist_menu_suspendproc 5823
#define tasklist_menu_resumeproc 5824
#define tasklist_menu_createproc 5825
#define tasklist_menu_viewproperties 5826
#define tasklist_menu_findit 5827
#define tasklist_menu_findproc 5828
#define tasklist_menu_terminateprocesstree 5829
#define tasklist_menu_injectdll 5830
#define tasklist_menu_dumpproc 5831

#define svclist_menu_refresh 4810
#define svclist_menu_stopdrv 4811
#define svclist_menu_deletedrv 4812
#define svclist_menu_properties 4813
#define svclist_menu_verifysignature 4814
#define svclist_menu_excludesysmodule 4815
#define svclist_menu_locate 4816
#define svclist_menu_installdrv 4817

#define win_Main_procUpdateTimer 1002
#define win_Main_driverUpdateTimer 1003
#define win_Main_heartbeatTimer 1004

void UpdateProcessListView(HWND hListView);
void UpdateDriverListView(HWND hListView);

//主窗口控件子类化回调
LRESULT CALLBACK TasklistSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK DriverlistSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

HWND __AssembleMainGui() {
	dbgout("__AssembleMainGUI() was called.\n");
	WNDCLASSW Reg = { 0 }; //主窗口回调
	HINSTANCE MainWin = 0;
	Reg.cbClsExtra = 0;
	Reg.cbWndExtra = 0;
	Reg.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	Reg.hCursor = LoadCursorW(0, IDC_ARROW);
	Reg.hIcon = IrIcon;
	Reg.hInstance = hInstance;
	Reg.lpfnWndProc = Window_Main;
	Reg.lpszClassName = L"Iridium";
	Reg.lpszMenuName = 0;
	Reg.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	RegisterClassW(&Reg);

	// 创建主窗口（注意：移除WS_THICKFRAME的限制，允许调整大小）
	MainWindow = CreateWindowA("Iridium", raname, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, Config->IridiumDlgWidth, Config->IridiumDlgHeight, 0, (HMENU)0, MainWin, 0);
	if (!MainWindow) {
		MessageBoxA(0, "Error:Create the main window failed!", "错误", MB_OK | MB_ICONERROR);
		ExitProcess(GetLastError());
	}

	if (Config->DisableCapture) {
		printA("反截屏保护开启,截屏功能可能受影响\n", 1);
		SetWindowDisplayAffinity(MainWindow, WDA_EXCLUDEFROMCAPTURE);
	}

	DWORD dwFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE;
	HWND hwndIns = HWND_TOPMOST;
	if (Config->ForceOnTop) {
		SetWindowPos(MainWindow, hwndIns, 0, 0, 0, 0, dwFlags);
		DWORD dwExStyle = (DWORD)GetWindowLongPtr(MainWindow, GWL_EXSTYLE);
		if (dwExStyle & WS_EX_TOPMOST) {
		}
	}

	dbgout("Created window_Main HWND=0x%x\n", MainWindow);

	HMENU MainMenu = GetSystemMenu(MainWindow, false);		// 复制或修改而访问窗口菜单
	// 允许最大化
	// RemoveMenu(MainMenu, SC_MAXIMIZE, MF_BYCOMMAND);	// 从指定菜单删除一个菜单项或分离一个子菜单
	//RemoveMenu(MainMenu, SC_MINIMIZE, MF_BYCOMMAND);
	DrawMenuBar(MainWindow);

	//初始化根选择夹 - 初始大小设为0，稍后调整
	FuncSelect = CreateWindowA("SysTabControl32", NULL, WS_VISIBLE | WS_CHILD | TCS_TABS, 0, 0, 0, 0, MainWindow, (HMENU)Main_FuncSelect, 0, 0);
	if (!FuncSelect) {
		MessageBoxA(MainWindow, "Error:Create the widget failed!", "错误", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}
	SendMessageA(FuncSelect, WM_SETFONT, (WPARAM)UniversalFont, 2); //设置选择夹字体
	TCITEMA Tab = { 0 };
	LPCSTR List[] = { "进程管理器","驱动","网络","存储","硬件","内核","设置" };
	for (int i = 0; i < ARRAYSIZE(List); i++) {
		Tab.mask = TCIF_IMAGE | TCIF_TEXT;
		Tab.iImage = 0;
		Tab.pszText = (LPSTR)List[i];
		SendMessageA(FuncSelect, TCM_INSERTITEMA, i, (LPARAM)&Tab);
	}
	UpdateWindow(FuncSelect);

	dbgout("Created control_FuncSelect HWND=0x%x\n", FuncSelect);

	//初始化进程列表 - 初始大小设为0
	Tasklist = CreateWindowExA(0, WC_LISTVIEWA, NULL, WS_VISIBLE | WS_CHILD | LVS_REPORT, 0, 0, 0, 0, FuncSelect, (HMENU)Main_tsklist, 0, 0);
	if (!Tasklist) {
		MessageBoxA(MainWindow, "Error:Create the widget failed!", "错误", MB_OK | MB_ICONERROR);
	}
	ListView_SetExtendedListViewStyle(Tasklist, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	LVCOLUMN TempLvc = { 0 }; //可复用
	TempLvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
	LPCWSTR Header[] = { L"进程名称",L"PID",L"线程计数",L"用户名",L"进程基地址",L"文件路径",L"内存占用",L"发布者",L"启动时间",L"网络" };
	// 列宽改为百分比，稍后动态计算
	DWORD len[] = { 140,90,90,120,160,260,80,120,100,90 };
	for (int i = 0; i < ARRAYSIZE(Header); i++) {
		TempLvc.pszText = (LPWSTR)Header[i];
		TempLvc.cx = len[i];
		TempLvc.iSubItem = i;
		ListView_InsertColumn(Tasklist, i, &TempLvc);
	}

	dbgout("Created control_Tasklist HWND=0x%x\n", Tasklist);

	UpdateProcessListView(Tasklist);

	//初始化服务列表
	Svclist = CreateWindowA("SysListView32", NULL, WS_VISIBLE | WS_CHILD | LVS_REPORT, 0, 0, 0, 0, FuncSelect, (HMENU)Main_svclist, 0, 0);
	if (!Svclist) {
		MessageBoxA(MainWindow, "Error:Create the widget failed!", "错误", MB_OK | MB_ICONERROR);
	}
	ListView_SetExtendedListViewStyle(Svclist, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	memset(&TempLvc, 0, sizeof(LVCOLUMN));
	TempLvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
	LPCWSTR Header2[] = { L"序号",L"名称",L"基地址",L"大小",L"文件路径",L"驱动对象",L"对象名",L"公司" };
	DWORD len2[] = { 90,150,190,190,260,180,100,120 };
	for (int i = 0; i < ARRAYSIZE(Header2); i++) {
		TempLvc.pszText = (LPWSTR)Header2[i];
		TempLvc.cx = len2[i];
		TempLvc.iSubItem = i;
		ListView_InsertColumn(Svclist, i, &TempLvc);
	}
	ShowWindowAsync(Svclist, 0);

	dbgout("Created control_Svclist HWND=0x%x\n", Svclist);

	//初始化网络列表
	Netlist = CreateWindowA("SysTabControl32", NULL, WS_VISIBLE | WS_CHILD | TCS_TABS, 0, 0, 0, 0, FuncSelect, (HMENU)Main_netlist, 0, 0);
	if (!Netlist) {
		MessageBoxA(MainWindow, "Error:Create the widget failed!", "错误", MB_OK | MB_ICONERROR);
	}
	SendMessageA(Netlist, WM_SETFONT, (WPARAM)UniversalFont, 2); //设置选择夹字体
	TCITEMA Tab2 = { 0 };
	LPCSTR List2[] = { "端口","Wfp过滤器","Wfp标注","Afd","Tdx","SPI","Hosts文件" };
	for (int i = 0; i < ARRAYSIZE(List2); i++) {
		Tab2.mask = TCIF_IMAGE | TCIF_TEXT;
		Tab2.iImage = 0;
		Tab2.pszText = (LPSTR)List2[i];
		SendMessageA(Netlist, TCM_INSERTITEMA, i, (LPARAM)&Tab2);
	}
	UpdateWindow(Netlist);

	dbgout("Created control_Netlist HWND=0x%x\n", Netlist);

	//初始化存储列表
	Stlist = CreateWindowA("SysTabControl32", NULL, WS_VISIBLE | WS_CHILD | TCS_TABS, 0, 0, 0, 0, FuncSelect, (HMENU)Main_fslist, 0, 0);
	if (!Stlist) {
		MessageBoxA(MainWindow, "Error:Create the widget failed!", "错误", MB_OK | MB_ICONERROR);
	}
	SendMessageA(Stlist, WM_SETFONT, (WPARAM)UniversalFont, 2); //设置选择夹字体
	TCITEMA Tab3 = { 0 };

	DWORD drives = GetLogicalDrives();
	for (int i = 0; i < 26; i++) {
		if (drives & (1 << i)) {
			CHAR drive[] = { TEXT('A') + i, TEXT(':'), TEXT('\\'), TEXT('\0') };
			UINT type = GetDriveTypeA(drive);

			// 只处理固定磁盘、可移动磁盘和网络驱动器
			if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_REMOTE) {
				Tab3.mask = TCIF_IMAGE | TCIF_TEXT;
				Tab3.iImage = 0;
				Tab3.pszText = (LPSTR)drive;
				SendMessageA(Stlist, TCM_INSERTITEMA, i, (LPARAM)&Tab3);
			}
		}
	}
	UpdateWindow(Stlist);
	dbgout("Created control_Stlist HWND=0x%x\n", Stlist);

	//初始化存储-文件列表
	Stflist = CreateWindowA("SysListView32", NULL, WS_VISIBLE | WS_CHILD | LVS_REPORT, 0, 0, 0, 0, Stlist, (HMENU)Main_fsflist, 0, 0);
	if (!Stflist) {
		MessageBoxA(MainWindow, "Error:Create the widget failed!", "错误", MB_OK | MB_ICONERROR);
	}
	ListView_SetExtendedListViewStyle(Stflist, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	memset(&TempLvc, 0, sizeof(LVCOLUMN));
	TempLvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
	LPCWSTR Header3[] = { L"名称",L"类型",L"路径",L"大小",L"发布者",L"创建日期",L"修改日期" };
	DWORD len3[] = { 300,80,400,80,100,100,100 };
	for (int i = 0; i < ARRAYSIZE(Header3); i++) {
		TempLvc.pszText = (LPWSTR)Header3[i];
		TempLvc.cx = len3[i];
		TempLvc.iSubItem = i;
		ListView_InsertColumn(Stflist, i, &TempLvc);
	}
	ShowWindowAsync(Stflist, 0);

	dbgout("Created control_Stflist HWND=0x%x\n", Stflist);

	if (SetTimer(MainWindow, win_Main_procUpdateTimer, 1000, 0)) {
		dbgout("Set window timer for process list update.\n");
	}

	if (SetTimer(MainWindow, win_Main_driverUpdateTimer, 1000, 0)) {
		dbgout("Set window timer for driver list update.\n");
	}

	if (SetTimer(MainWindow, win_Main_heartbeatTimer, 1000, 0)) {
		dbgout("Set window timer for heart beat update.\n");
		
	}

	//注册热键
	if (RegisterHotKey(MainWindow, 100001, MOD_ALT | MOD_CONTROL, 0x53)) dbgout("Registered Setup hot key : Ctrl+Alt+S\n");

	//创建菜单
	ProcMenu = CreatePopupMenu();
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_refresh, L"刷新");
	AppendMenu(ProcMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_terminateproc, L"&T.结束进程(R3)");
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_terminateprocf, L"&F.结束进程+(R0)");
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_suspendproc, L"&S.挂起进程");
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_resumeproc, L"&R.恢复进程");
	AppendMenu(ProcMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_viewproperties, L"&P.详细信息");
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_findit, L"&L.定位到资源管理器");
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_injectdll, L"&I.注入DLL");
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_dumpproc, L"&D.生成Dump文件");
	AppendMenu(ProcMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(ProcMenu, MF_STRING, tasklist_menu_findproc, L"&Q.搜索进程");

	SvcMenu = CreatePopupMenu();
	AppendMenu(SvcMenu, MF_STRING, svclist_menu_refresh, L"刷新");
	AppendMenu(SvcMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(SvcMenu, MF_STRING, svclist_menu_stopdrv, L"&U.卸载驱动");
	AppendMenu(SvcMenu, MF_STRING, svclist_menu_deletedrv, L"&D.删除驱动");
	AppendMenu(SvcMenu, MF_STRING, svclist_menu_properties, L"&P.详细信息");
	AppendMenu(SvcMenu, MF_STRING, svclist_menu_locate, L"&L.定位到资源管理器");
	AppendMenu(SvcMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(SvcMenu, MF_STRING, svclist_menu_verifysignature, L"&V.验证数字签名");
	AppendMenu(SvcMenu, MF_STRING, svclist_menu_excludesysmodule, L"&E.排除系统模块");
	AppendMenu(SvcMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(SvcMenu, MF_STRING, svclist_menu_installdrv, L"&C.加载驱动");

	//子类化控件
	SetWindowSubclass(Tasklist, TasklistSubclassProc, 0, 0);
	SetWindowSubclass(Svclist, DriverlistSubclassProc, 0, 0);

	// 初始调整控件大小
	AdjustChildControls(MainWindow);

	return MainWindow;
}

HWND c0, c1, c2, c3, c4, c5;

LRESULT CALLBACK TasklistSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) { //用于进程列表的子类化窗口过程
	switch (uMsg) {
	case WM_RBUTTONDOWN: {
		// 处理右键按下，确保选中正确的项
		LVHITTESTINFO hitTest = { 0 };
		hitTest.pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		int hitIndex = ListView_HitTest(hWnd, &hitTest);

		if (hitIndex != -1 && (hitTest.flags & LVHT_ONITEM)) {
			// 取消其他项的选中状态
			ListView_SetItemState(hWnd, -1, 0, LVIS_SELECTED);
			// 选中右键点击的项
			ListView_SetItemState(hWnd, hitIndex, LVIS_SELECTED, LVIS_SELECTED);
			SetFocus(hWnd); // 确保控件获得焦点
		}
		return 0; // 已处理，不继续传递
	}

	case WM_RBUTTONUP: {
		if (ListView_GetSelectedCount(hWnd) > 0) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ClientToScreen(hWnd, &pt);

			// 重要修改：使用不同的菜单显示方式
			HWND hMainWnd = GetAncestor(hWnd, GA_ROOT); // 直接获取根窗口（主窗口）

			// 方法1：直接发送命令
			UINT cmd = TrackPopupMenuEx(ProcMenu,
				TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
				pt.x, pt.y, hMainWnd, NULL);

			if (cmd != 0) {
				// 直接发送命令到主窗口
				SendMessage(hMainWnd, WM_COMMAND, MAKEWPARAM(0, cmd), 0);
			}
			return 0;
		}
		break;
	}

	case WM_KEYDOWN: {
		// 处理键盘上下文菜单键
		if (wParam == VK_APPS) { // 上下文菜单键
			if (ListView_GetSelectedCount(hWnd) > 0) {
				// 获取选中项的位置
				int selected = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
				if (selected != -1) {
					RECT rect;
					ListView_GetItemRect(hWnd, selected, &rect, LVIR_BOUNDS);
					POINT pt = { rect.left, rect.bottom };
					ClientToScreen(hWnd, &pt);

					UINT cmd = TrackPopupMenuEx(ProcMenu,
						TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
						pt.x, pt.y, GetParent(GetParent(hWnd)), NULL);

					if (cmd != 0) {
						PostMessage(GetParent(GetParent(hWnd)), WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
					}
					return 0;
				}
			}
		}
		break;
	}

	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, TasklistSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DriverlistSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) { //用于进程列表的子类化窗口过程
	switch (uMsg) {
	case WM_RBUTTONDOWN: {
		// 处理右键按下，确保选中正确的项
		LVHITTESTINFO hitTest = { 0 };
		hitTest.pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		int hitIndex = ListView_HitTest(hWnd, &hitTest);

		if (hitIndex != -1 && (hitTest.flags & LVHT_ONITEM)) {
			// 取消其他项的选中状态
			ListView_SetItemState(hWnd, -1, 0, LVIS_SELECTED);
			// 选中右键点击的项
			ListView_SetItemState(hWnd, hitIndex, LVIS_SELECTED, LVIS_SELECTED);
			SetFocus(hWnd); // 确保控件获得焦点
		}
		return 0; // 已处理，不继续传递
	}

	case WM_RBUTTONUP: {
		if (ListView_GetSelectedCount(hWnd) > 0) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ClientToScreen(hWnd, &pt);

			// 重要修改：使用不同的菜单显示方式
			HWND hMainWnd = GetAncestor(hWnd, GA_ROOT); // 直接获取根窗口（主窗口）

			// 方法1：直接发送命令
			UINT cmd = TrackPopupMenuEx(SvcMenu,
				TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
				pt.x, pt.y, hMainWnd, NULL);

			if (cmd != 0) {
				// 直接发送命令到主窗口
				SendMessage(hMainWnd, WM_COMMAND, MAKEWPARAM(0, cmd), 0);
			}
			return 0;
		}
		break;
	}

	case WM_KEYDOWN: {
		// 处理键盘上下文菜单键
		if (wParam == VK_APPS) { // 上下文菜单键
			if (ListView_GetSelectedCount(hWnd) > 0) {
				// 获取选中项的位置
				int selected = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
				if (selected != -1) {
					RECT rect;
					ListView_GetItemRect(hWnd, selected, &rect, LVIR_BOUNDS);
					POINT pt = { rect.left, rect.bottom };
					ClientToScreen(hWnd, &pt);

					UINT cmd = TrackPopupMenuEx(SvcMenu,
						TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
						pt.x, pt.y, GetParent(GetParent(hWnd)), NULL);

					if (cmd != 0) {
						PostMessage(GetParent(GetParent(hWnd)), WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
					}
					return 0;
				}
			}
		}
		break;
	}

	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, TasklistSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Window_Main(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	WORD dlgID = LOWORD(wParam);//控件ID
	WORD codeID = HIWORD(wParam); //消息代码
	//printf("%d - %d\n", Message, codeID);
	HWND hDlg = (HWND)dlgID;
	Window_Main_Msg_Cnt++;
	switch (Message)
	{
	case WM_INITDIALOG: {
		break;
	}

	case WM_DESTROY: {
		break;
	}

	case WM_SIZE: {
		// 处理窗口大小改变
		if (wParam != SIZE_MINIMIZED) {
			AdjustChildControls(hWnd);
		}
		break;
	}

	case WM_GETMINMAXINFO: {
		// 设置窗口最小尺寸
		MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
		pMMI->ptMinTrackSize.x = WINDOW_MIN_WIDTH;
		pMMI->ptMinTrackSize.y = WINDOW_MIN_HEIGHT;
		return 0;
	}

	case WM_SIZING: {
		// 在调整窗口大小时更新控件
		AdjustChildControls(hWnd);
		break;
	}

	case WM_NOTIFY: {
		NMHDR* pNMHDR = (NMHDR*)lParam;
		UINT ctrlID = pNMHDR->idFrom;  // 控件 ID
		UINT codeID2 = pNMHDR->code;

		if (ctrlID == Main_FuncSelect) {
			switch (codeID2)
			{
			case TCN_SELCHANGE: {
				int id = TabCtrl_GetCurSel(pNMHDR->hwndFrom);
				switch (id)
				{
				case 0: { //进程列表
					ShowWindowAsync(Tasklist, 1);
					ShowWindowAsync(Svclist, 0);
					ShowWindowAsync(Netlist, 0);
					ShowWindowAsync(Stlist, 0);
					ShowWindowAsync(Stflist, 0);
					break;
				}

				case 1: { //服务列表
					ShowWindowAsync(Tasklist, 0);
					ShowWindowAsync(Svclist, 1);
					ShowWindowAsync(Netlist, 0);
					ShowWindowAsync(Stlist, 0);
					ShowWindowAsync(Stflist, 0);
					break;
				}
				case 2: { //网络列表
					ShowWindowAsync(Tasklist, 0);
					ShowWindowAsync(Svclist, 0);
					ShowWindowAsync(Netlist, 1);
					ShowWindowAsync(Stlist, 0);
					ShowWindowAsync(Stflist, 0);
					break;
				}
				case 3: {
					ShowWindowAsync(Tasklist, 0);
					ShowWindowAsync(Svclist, 0);
					ShowWindowAsync(Netlist, 0);
					ShowWindowAsync(Stlist, 1);
					ShowWindowAsync(Stflist, 1);

					//每次选择到该子夹时刷新一次磁盘列表
					SendMessageA(Stlist, TCM_DELETEALLITEMS, 0, 0);
					TCITEMA Tab3 = { 0 };
					DWORD drives = GetLogicalDrives();
					for (int i = 0; i < 26; i++) {
						if (drives & (1 << i)) {
							CHAR drive[] = { TEXT('A') + i, TEXT(':'), TEXT('\\'), TEXT('\0') };
							UINT type = GetDriveTypeA(drive);

							// 只处理固定磁盘、可移动磁盘和网络驱动器
							if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_REMOTE) {
								Tab3.mask = TCIF_IMAGE | TCIF_TEXT;
								Tab3.iImage = 0;
								Tab3.pszText = (LPSTR)drive;
								SendMessageA(Stlist, TCM_INSERTITEMA, i, (LPARAM)&Tab3);
								dbgout("Found harddisk device,volume:%s\n", drive);
							}
						}
					}
					UpdateWindow(Stlist);
					break;
				}

				case 6: { //设置
					__AssembleSetupGUI();
					TabCtrl_SetCurSel(FuncSelect, 0);
					break;
				}
				default:
					break;
				}
				break;
			}
			default:
				break;
			}
			break;
		}

		if (ctrlID == Main_fslist) {
			switch (codeID2)
			{
			case TCN_SELCHANGE: { //磁盘选择夹更换
				TCITEMA item = { 0 };
				char buf[8] = { 0 };
				item.mask = TCIF_TEXT;
				item.cchTextMax = 8;
				item.pszText = buf;
				int id = TabCtrl_GetCurSel(pNMHDR->hwndFrom);
				SendMessageA(Stlist, TCM_GETITEM, (WPARAM)id, (LPARAM)&item); //获取当前的磁盘 e.g. A B C

			}
			default:
				break;
			}

		}

		if (dlgID == 3) {
			break;
		}
		break;
	}

	case WM_COMMAND: {
		switch (codeID)
		{
		case tasklist_menu_terminateproc: {
			wchar_t processName[MAX_PATH] = { 0 };
			int selected = ListView_GetNextItem(Tasklist, -1, LVNI_SELECTED);
			if (selected != -1) {
				// 获取进程信息
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_PARAM;
				lvi.iItem = selected;
				ListView_GetItem(Tasklist, &lvi);
				DWORD pid = (DWORD)lvi.lParam;
				ListView_GetItemText(Tasklist, selected, 0, processName, MAX_PATH);

				wchar_t confirmMsg[256] = { 0 };
				char errMsg[256] = { 0 };

				if (IsSystemProcess(processName)) {
					swprintf_s(confirmMsg, 256, L"确定要结束系统进程%s吗?结束重要的系统进程可能导致系统不稳定甚至蓝屏,继续?", processName);
				}
				else {
					swprintf_s(confirmMsg, 256, L"确定要结束进程%s吗?", processName);
				}

				if (MessageBox(MainWindow, confirmMsg, L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES) {
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
					if (hProcess) {
						if (TerminateProcess(hProcess, 0)) {
							UpdateProcessListView(Tasklist);
						}
						else {
							sprintf_s(errMsg, 256, "终止进程 %ws 时出现错误: %s\n所执行的操作未成功.尝试使用R0终止进程以解决该问题.", processName, TranslateGetLastErrorMsg(GetLastError()));
							MessageBoxA(MainWindow, errMsg, "错误", MB_OK | MB_ICONERROR);
						}
						CloseHandle(hProcess);
					}
					else {
						sprintf_s(errMsg, 256, "打开进程 %ws 时出现错误: %s\n所执行的操作未成功.\n尝试使用R0终止进程以解决该问题.", processName, TranslateGetLastErrorMsg(GetLastError()));
						MessageBoxA(MainWindow, errMsg, "错误", MB_OK | MB_ICONERROR);
					}
				}
			}
			break;
		}
		case tasklist_menu_refresh: {
			UpdateProcessListView(Tasklist);
			break;
		}

		case svclist_menu_stopdrv: {
			wchar_t svcname[MAX_PATH] = { 0 };
			int selected = ListView_GetNextItem(Svclist, -1, LVNI_SELECTED);
			if (selected != -1) {
				// 获取进程信息
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_PARAM;
				lvi.iItem = selected;
				ListView_GetItem(Tasklist, &lvi);
				DWORD pid = (DWORD)lvi.lParam;
				ListView_GetItemText(Svclist, selected, 1, svcname, MAX_PATH);

				if (!UnloadDriverWithName(svcname)) {
					MessageBoxA(MainWindow, "卸载启动程序失败.", "错误", MB_ICONERROR | MB_OK);
					break;
				}

				break;
			}
		}
		default:
			break;
		}
		break;
	}

	case WM_TIMER: {
		if (wParam == win_Main_procUpdateTimer) {
			UpdateProcessListView(Tasklist);
			break;
		}

		if (wParam == win_Main_driverUpdateTimer) {
			UpdateDriverListView(Svclist);
			break;
		}

		if (wParam == win_Main_heartbeatTimer) {
			HeartBeat = GetTickCount64();
			break;
		}
	}

	case WM_HOTKEY: {
		UINT modifiers = LOWORD(lParam); // 获取修饰键
		UINT key = HIWORD(lParam); // 获取虚拟键码
		if (modifiers == MOD_ALT | MOD_CONTROL && key == 0x53) {//Ctrl+Alt+S
			if (IsWindow(c0)) {
				ShowWindowAsync(c0, 1);
			}
			else {
				c0 = __AssembleSetupGUI();
			}
		}
	}

	case WM_CONTEXTMENU: {
	}

	default:
		return DefWindowProcW(hWnd, Message, wParam, lParam);
	}

	return false;
}



//进程列表刷新
struct ProcessInfo {
	DWORD pid;
	std::wstring name;
	std::string logonusrname;
	DWORD dThread;
	std::string path;
	// 其他字段可以根据需要添加
};


// 获取进程列表
std::vector<ProcessInfo> GetCurrentProcesses() {
	std::vector<ProcessInfo> processes;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return processes;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			ProcessInfo info;
			info.pid = pe32.th32ProcessID;
			info.name = pe32.szExeFile;
			info.dThread = pe32.cntThreads;
			//info.logonusrname = 
			processes.push_back(info);
		} while (Process32NextW(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return processes;
}

// 在ListView中查找指定PID的项
int FindProcessInListView(HWND hListView, DWORD pid) {
	int itemCount = ListView_GetItemCount(hListView);

	for (int i = 0; i < itemCount; i++) {
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;

		if (ListView_GetItem(hListView, &lvi)) {
			if (lvi.lParam == (LPARAM)pid) {
				return i; // 找到，返回索引
			}
		}
	}

	return -1; // 未找到
}

// 更新ListView进程列表
void UpdateProcessListView(HWND hListView) {
	// 获取当前ListView中的所有PID
	std::map<DWORD, bool> existingPids;
	int itemCount = ListView_GetItemCount(hListView);

	for (int i = 0; i < itemCount; i++) {
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;

		if (ListView_GetItem(hListView, &lvi)) {
			existingPids[(DWORD)lvi.lParam] = true;
		}
	}

	// 获取当前系统进程
	std::vector<ProcessInfo> currentProcesses = GetCurrentProcesses();

	// 添加新进程
	for (const auto& process : currentProcesses) {
		if (existingPids.find(process.pid) == existingPids.end()) {
			// 新进程，添加到ListView
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = ListView_GetItemCount(hListView);
			lvi.lParam = process.pid;

			// 使用静态缓冲区或动态分配
			static wchar_t nameBuffer[MAX_PATH];
			if (wcscmp(process.name.c_str(), L"[System Process]") == 0) {
				wcscpy_s(nameBuffer, L"系统空闲进程");
			}
			else {
				wcscpy_s(nameBuffer, process.name.c_str());
			}
			lvi.pszText = nameBuffer;

			int newItem = ListView_InsertItem(hListView, &lvi);

			// 设置PID列
			if (newItem != -1) {
				wchar_t pidStr[32] = { 0 };
				swprintf_s(pidStr, L"%d", process.pid);
				ListView_SetItemText(hListView, newItem, 1, pidStr);
			}

			if (newItem != -1) {
				wchar_t tcntStr[32] = { 0 };
				swprintf_s(tcntStr, L"%04d", process.dThread);
				ListView_SetItemText(hListView, newItem, 2, tcntStr);
			}
		}
	}

	// 删除已退出的进程
	std::map<DWORD, bool> currentPids;
	for (const auto& process : currentProcesses) {
		currentPids[process.pid] = true;
	}

	// 从后往前删除，避免索引变化问题
	for (int i = itemCount - 1; i >= 0; i--) {
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;

		if (ListView_GetItem(hListView, &lvi)) {
			DWORD pid = (DWORD)lvi.lParam;
			if (currentPids.find(pid) == currentPids.end()) {
				ListView_DeleteItem(hListView, i);
			}
		}
	}
}

// 驱动信息结构体
typedef struct _DRIVER_INFO {
	int index;
	wchar_t name[256];
	ULONG_PTR baseAddress;
	DWORD size;
	wchar_t filePath[MAX_PATH];
	wchar_t company[256];
	// 用于比较的唯一标识符
	ULONG_PTR uniqueId;
} DRIVER_INFO, * PDRIVER_INFO;

// 全局变量，用于跟踪当前的驱动列表
std::map<ULONG_PTR, DRIVER_INFO> g_currentDrivers;

// 动态获取 NtQuerySystemInformation 函数
PNtQuerySystemInformation GetNtQuerySystemInformation() {
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	if (!hNtDll) {
		return nullptr;
	}

	return (PNtQuerySystemInformation)GetProcAddress(hNtDll, "NtQuerySystemInformation");
}

// 获取文件版本信息中的公司名称
BOOL GetFileCompanyInfo(const wchar_t* filePath, wchar_t* company, DWORD companySize) {
	if (!filePath || !company) return FALSE;

	company[0] = L'\0';

	DWORD verHandle = 0;
	DWORD verSize = GetFileVersionInfoSizeW(filePath, &verHandle);
	if (verSize == 0) return FALSE;

	BYTE* verData = new BYTE[verSize];
	if (!verData) return FALSE;

	BOOL result = FALSE;

	if (GetFileVersionInfoW(filePath, verHandle, verSize, verData)) {
		UINT bufferSize = 0;
		VS_FIXEDFILEINFO* fixedFileInfo = NULL;

		if (VerQueryValueW(verData, L"\\", (VOID**)&fixedFileInfo, &bufferSize)) {
			wchar_t* companyName = NULL;
			UINT companyLen = 0;

			// 尝试获取公司名称
			if (VerQueryValueW(verData, L"\\StringFileInfo\\040904b0\\CompanyName",
				(VOID**)&companyName, &companyLen)) {
				wcsncpy_s(company, companySize, companyName, _TRUNCATE);
				result = TRUE;
			}
			// 如果上面失败，尝试其他语言代码页
			else if (VerQueryValueW(verData, L"\\StringFileInfo\\040904e4\\CompanyName",
				(VOID**)&companyName, &companyLen)) {
				wcsncpy_s(company, companySize, companyName, _TRUNCATE);
				result = TRUE;
			}
			else if (VerQueryValueW(verData, L"\\StringFileInfo\\000004b0\\CompanyName",
				(VOID**)&companyName, &companyLen)) {
				wcsncpy_s(company, companySize, companyName, _TRUNCATE);
				result = TRUE;
			}
		}
	}

	delete[] verData;
	return result;
}

// 使用 ZwQuerySystemInformation 枚举系统驱动
std::vector<DRIVER_INFO> EnumSystemDrivers() {
	std::vector<DRIVER_INFO> drivers;

	// 动态获取函数指针
	PNtQuerySystemInformation NtQuerySystemInformation = GetNtQuerySystemInformation();
	if (!NtQuerySystemInformation) {
		return drivers;
	}

	ULONG bufferSize = 0;
	NTSTATUS status = NtQuerySystemInformation(SystemModuleInformation, NULL, 0, &bufferSize);

	// 预期的状态是 STATUS_INFO_LENGTH_MISMATCH
	if (status != 0xC0000004) { // STATUS_INFO_LENGTH_MISMATCH
		return drivers;
	}

	// 分配足够的内存
	PRTL_PROCESS_MODULES moduleInfo = (PRTL_PROCESS_MODULES)malloc(bufferSize);
	if (!moduleInfo) {
		return drivers;
	}

	// 查询系统模块信息
	status = NtQuerySystemInformation(SystemModuleInformation, moduleInfo, bufferSize, &bufferSize);
	if (!status) {
		for (ULONG i = 0; i < moduleInfo->NumberOfModules; i++) {
			DRIVER_INFO info = { 0 };
			info.index = i + 1;

			// 获取模块信息
			RTL_PROCESS_MODULE_INFORMATION* module = &moduleInfo->Modules[i];

			// 基地址和大小
			info.baseAddress = (ULONG_PTR)module->ImageBase;
			info.size = module->ImageSize;

			// 使用基地址作为唯一标识符
			info.uniqueId = info.baseAddress;

			// 文件名（从完整路径中提取）
			char* fileName = (char*)module->FullPathName + module->OffsetToFileName;

			// 转换为宽字符
			size_t convertedChars = 0;
			mbstowcs_s(&convertedChars, info.name, 256, fileName, _TRUNCATE);

			// 完整文件路径
			wchar_t buf[260] = { 0 };
			mbstowcs_s(&convertedChars, buf, MAX_PATH, (char*)module->FullPathName, _TRUNCATE);

			ConvertSystemPathToFullPath(buf, info.filePath, 260);

			// 获取公司信息
			GetFileCompanyInfo(info.filePath, info.company, 256);

			drivers.push_back(info);
		}
	}

	free(moduleInfo);
	return drivers;
}

// 在ListView中查找指定驱动
int FindDriverInListView(HWND hListView, ULONG_PTR uniqueId) {
	int itemCount = ListView_GetItemCount(hListView);

	for (int i = 0; i < itemCount; i++) {
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;

		if (ListView_GetItem(hListView, &lvi)) {
			PDRIVER_INFO pInfo = (PDRIVER_INFO)lvi.lParam;
			if (pInfo && pInfo->uniqueId == uniqueId) {
				return i; // 找到，返回索引
			}
		}
	}

	return -1; // 未找到
}

// 更新驱动程序列表到ListView - 智能更新版本
void UpdateDriverListView(HWND hListView) {
	// 获取当前系统驱动列表
	std::vector<DRIVER_INFO> currentDrivers = EnumSystemDrivers();

	// 创建当前驱动的唯一标识符映射
	std::map<ULONG_PTR, bool> currentDriverIds;
	for (const auto& driver : currentDrivers) {
		currentDriverIds[driver.uniqueId] = true;
	}

	// 获取ListView中现有的驱动
	std::map<ULONG_PTR, bool> existingDriverIds;
	int itemCount = ListView_GetItemCount(hListView);

	for (int i = 0; i < itemCount; i++) {
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;

		if (ListView_GetItem(hListView, &lvi)) {
			PDRIVER_INFO pInfo = (PDRIVER_INFO)lvi.lParam;
			if (pInfo) {
				existingDriverIds[pInfo->uniqueId] = true;
			}
		}
	}

	// 删除已卸载的驱动
	for (int i = itemCount - 1; i >= 0; i--) {
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;

		if (ListView_GetItem(hListView, &lvi)) {
			PDRIVER_INFO pInfo = (PDRIVER_INFO)lvi.lParam;
			if (pInfo && currentDriverIds.find(pInfo->uniqueId) == currentDriverIds.end()) {
				// 释放内存并删除项
				delete pInfo;
				ListView_DeleteItem(hListView, i);
			}
		}
	}

	// 添加新加载的驱动
	for (const auto& driver : currentDrivers) {
		if (existingDriverIds.find(driver.uniqueId) == existingDriverIds.end()) {
			// 新驱动，添加到ListView
			PDRIVER_INFO pNewDriver = new DRIVER_INFO;
			*pNewDriver = driver;

			LVITEMW lvi = { 0 };
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = ListView_GetItemCount(hListView);
			lvi.lParam = (LPARAM)pNewDriver;

			// 第0列：序号
			wchar_t indexStr[16];
			swprintf_s(indexStr, L"%d", pNewDriver->index);
			lvi.pszText = indexStr;

			int itemIndex = ListView_InsertItem(hListView, &lvi);
			if (itemIndex != -1) {
				// 第1列：名称
				ListView_SetItemText(hListView, itemIndex, 1, (LPWSTR)pNewDriver->name);

				// 第2列：基地址
				wchar_t addrStr[32];
				swprintf_s(addrStr, L"0x%p", (PVOID)pNewDriver->baseAddress);
				ListView_SetItemText(hListView, itemIndex, 2, addrStr);

				// 第3列：大小
				wchar_t sizeStr[32];
				if (pNewDriver->size > 0) {
					if (pNewDriver->size >= 1024 * 1024) {
						swprintf_s(sizeStr, L"%.2f MB", pNewDriver->size / (1024.0 * 1024.0));
					}
					else if (pNewDriver->size >= 1024) {
						swprintf_s(sizeStr, L"%.2f KB", pNewDriver->size / 1024.0);
					}
					else {
						swprintf_s(sizeStr, L"%lu B", pNewDriver->size);
					}
				}
				else {
					wcscpy_s(sizeStr, L"-");
				}
				ListView_SetItemText(hListView, itemIndex, 3, sizeStr);

				//4 文件路径
				ListView_SetItemText(hListView, itemIndex, 4, (LPWSTR)pNewDriver->filePath);

				//5 驱动对象（需要内核模式访问，这里留空或显示N/A）
				ListView_SetItemText(hListView, itemIndex, 5, (LPWSTR)L"N/A");

				//6 对象名（需要内核模式访问，这里留空或显示N/A）
				ListView_SetItemText(hListView, itemIndex, 6, (LPWSTR)L"N/A");

				//7 公司
				if (wcslen(pNewDriver->company) > 0) {
					ListView_SetItemText(hListView, itemIndex, 7, (LPWSTR)pNewDriver->company);
				}
				else {
					ListView_SetItemText(hListView, itemIndex, 7, (LPWSTR)L"-");
				}
			}
		}
	}

	// 更新全局驱动列表
	g_currentDrivers.clear();
	for (const auto& driver : currentDrivers) {
		g_currentDrivers[driver.uniqueId] = driver;
	}
}

// 清理ListView中的驱动数据
void CleanupDriverListView(HWND hListView) {
	int itemCount = ListView_GetItemCount(hListView);

	for (int i = 0; i < itemCount; i++) {
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;

		if (ListView_GetItem(hListView, &lvi)) {
			PDRIVER_INFO pInfo = (PDRIVER_INFO)lvi.lParam;
			if (pInfo) {
				delete pInfo;
			}
		}
	}

	ListView_DeleteAllItems(hListView);
	g_currentDrivers.clear();
}

// 获取驱动变化统计
void GetDriverChangeStats(int* added, int* removed) {
	if (!added || !removed) return;

	*added = 0;
	*removed = 0;

	// 获取当前系统驱动列表
	std::vector<DRIVER_INFO> currentDrivers = EnumSystemDrivers();

	// 创建当前驱动的唯一标识符映射
	std::map<ULONG_PTR, bool> currentDriverIds;
	for (const auto& driver : currentDrivers) {
		currentDriverIds[driver.uniqueId] = true;
	}

	// 计算新增的驱动
	for (const auto& driver : currentDrivers) {
		if (g_currentDrivers.find(driver.uniqueId) == g_currentDrivers.end()) {
			(*added)++;
		}
	}

	// 计算移除的驱动
	for (const auto& pair : g_currentDrivers) {
		if (currentDriverIds.find(pair.first) == currentDriverIds.end()) {
			(*removed)++;
		}
	}
}


HWND Setup, SetupOpt, d[64] = { 0 }, ab[64] = { 0 }, cm[64] = { 0 };

#define STATIC 0
#define Cb_Raname 21
#define Cb_DbgMsg 22
#define Cb_LdDrv 23
#define Cb_Protect 24
#define Cb_ProtectEx 25
#define Cb_DisCapt 26
#define Cb_DNetwork 27
#define Cb_ECache 28
#define Cb_FOTop 29
#define Cb_UseHotfix 291
#define Cb_DbgChk 292
#define Btn_cm_Save 12
#define Btn_UnlDrv 13
#define Btn_ReloadDrv 14

#define win_Setup_DrvStat_Timer 0x1001

HWND __AssembleSetupGUI() {

	if (IsWindow(Setup)) {
		ShowWindow(Setup, 1);
		return Setup;
	}

	dbgout("__AssembleSetupGUI() was called.\n");
	WNDCLASSW Reg = { 0 }; //主窗口回调
	HINSTANCE MainWin = 0;
	Reg.cbClsExtra = 0;
	Reg.cbWndExtra = 0;
	Reg.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	Reg.hCursor = LoadCursorW(0, IDC_ARROW);
	Reg.hIcon = IrIcon;
	Reg.hInstance = hInstance;
	Reg.lpfnWndProc = Window_Setup;
	Reg.lpszClassName = L"IridiumSetup";
	Reg.lpszMenuName = 0;
	Reg.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassW(&Reg);

	Setup = CreateWindowA("IridiumSetup", raname, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 350, 0, (HMENU)0, MainWin, 0);
	if (!Setup) {
		MessageBoxA(0, "Error:Create the setup window failed!", "错误", MB_OK | MB_ICONERROR);
		return 0;
	}

	if (Config->DisableCapture) SetWindowDisplayAffinity(Setup, WDA_EXCLUDEFROMCAPTURE);

	DWORD dwFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE;
	HWND hwndIns = HWND_TOPMOST;
	SetWindowPos(Setup, hwndIns, 0, 0, 0, 0, dwFlags);

	dbgout("Created window_Setup HWND=");
	if (Config->DebugMessage)printf("0x%x\n", Setup);

	HMENU MainMenu = GetSystemMenu(Setup, false);		// 复制或修改而访问窗口菜单
	RemoveMenu(MainMenu, SC_MAXIMIZE, MF_BYCOMMAND);	// 从指定菜单删除一个菜单项或分离一个子菜单
	//RemoveMenu(MainMenu, SC_MINIMIZE, MF_BYCOMMAND);
	DrawMenuBar(Setup);
	

	//初始化根选择夹
	SetupOpt = CreateWindowA("SysTabControl32", NULL, WS_VISIBLE | WS_CHILD | TCS_TABS, 0, 0, 640, 340, Setup, (HMENU)2, 0, 0);
	if (!SetupOpt) {
		MessageBoxA(Setup, "Error:Create the widget failed!", "错误", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}
	SendMessageA(SetupOpt, WM_SETFONT, (WPARAM)UniversalFont, 2); //设置选择夹字体
	TCITEMA Tab = { 0 };
	LPCSTR List[] = { "通用","驱动","关于" };
	for (int i = 0; i < ARRAYSIZE(List); i++) {
		Tab.mask = TCIF_IMAGE | TCIF_TEXT;
		Tab.iImage = 0;
		Tab.pszText = (LPSTR)List[i];
		SendMessageA(SetupOpt, TCM_INSERTITEMA, i, (LPARAM)&Tab);
	}
	UpdateWindow(SetupOpt);

	dbgout("Created control_SetupOpt HWND=");
	if (Config->DebugMessage)printf("0x%x\n", SetupOpt);

	//通用组件
	cm[0] = CreateWindowA("BUTTON", "窗口随机名称", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 25, 140, 21, Setup, (HMENU)Cb_Raname, 0, 0);
	SendMessageA(cm[0], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->RandomName) SendMessage(cm[0], BM_SETCHECK, BST_CHECKED, 0);

	cm[1] = CreateWindowA("BUTTON", "打印调试消息", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 46, 140, 21, Setup, (HMENU)Cb_DbgMsg, 0, 0);
	SendMessageA(cm[1], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->DebugMessage) SendMessage(cm[1], BM_SETCHECK, BST_CHECKED, 0);

	cm[2] = CreateWindowA("BUTTON", "加载驱动(推荐)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 67, 140, 21, Setup, (HMENU)Cb_LdDrv, 0, 0);
	SendMessageA(cm[2], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->LoadDriver) SendMessage(cm[2], BM_SETCHECK, BST_CHECKED, 0);

	cm[3] = CreateWindowA("BUTTON", "正常驱动保护(推荐)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 88, 140, 21, Setup, (HMENU)Cb_Protect, 0, 0);
	SendMessageA(cm[3], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->DriverProtect) SendMessage(cm[3], BM_SETCHECK, BST_CHECKED, 0);

	cm[4] = CreateWindowA("BUTTON", "激进驱动保护(不推荐)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 109, 140, 21, Setup, (HMENU)Cb_ProtectEx, 0, 0);
	SendMessageA(cm[4], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->DriverProtectEx) SendMessage(cm[4], BM_SETCHECK, BST_CHECKED, 0);

	cm[5] = CreateWindowA("BUTTON", "窗口反截屏保护", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 130, 140, 21, Setup, (HMENU)Cb_DisCapt, 0, 0);
	SendMessageA(cm[5], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->DisableCapture) SendMessage(cm[5], BM_SETCHECK, BST_CHECKED, 0);

	cm[6] = CreateWindowA("BUTTON", "禁止程序访问网络", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 151, 140, 21, Setup, (HMENU)Cb_DNetwork, 0, 0);
	SendMessageA(cm[6], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (!Config->Network) SendMessage(cm[6], BM_SETCHECK, BST_CHECKED, 0);

	cm[7] = CreateWindowA("BUTTON", "缓存数据以提升性能", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 172, 140, 21, Setup, (HMENU)Cb_ECache, 0, 0);
	SendMessageA(cm[7], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->CacheOffsets) SendMessage(cm[7], BM_SETCHECK, BST_CHECKED, 0);

	cm[8] = CreateWindowA("BUTTON", "窗口置顶(UIAccess)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 195, 140, 21, Setup, (HMENU)Cb_FOTop, 0, 0);
	SendMessageA(cm[8], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->UiAccess) SendMessage(cm[8], BM_SETCHECK, BST_CHECKED, 0);

	cm[9] = CreateWindowA("BUTTON", "保存配置", WS_CHILD, 0, 275, 70, 25, Setup, (HMENU)Btn_cm_Save, 0, 0);
	SendMessageA(cm[9], WM_SETFONT, (WPARAM)UniversalFont, 2);

	cm[10] = CreateWindowA("BUTTON", "使用热修复文件", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 219, 140, 21, Setup, (HMENU)Cb_UseHotfix, 0, 0);
	SendMessageA(cm[10], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->UseHotFixFile) SendMessage(cm[10], BM_SETCHECK, BST_CHECKED, 0);

	cm[11] = CreateWindowA("BUTTON", "启用调试器检测", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 150, 25, 140, 21, Setup, (HMENU)Cb_DbgChk, 0, 0);
	SendMessageA(cm[11], WM_SETFONT, (WPARAM)UniversalFont, 2);
	if (Config->DebuggerCheck) SendMessage(cm[11], BM_SETCHECK, BST_CHECKED, 0);
	//驱动组件

	if (SetTimer(Setup, win_Setup_DrvStat_Timer, 2000, 0)) {
		dbgout("Set window timer for driver status update.\n");
	}

	d[0] = CreateWindowA("STATIC", "注意:由于作者没有钱购买数字签名,所以使用了易受攻击的驱动来加载驱动程序,你可以在项目的Driver.cpp中查看具体实现方法.但是由于该方法被大多数杀毒软件所拦截,故启动时请放行本程序或关闭杀毒软件.", WS_CHILD, 0, 25, 620, 36, Setup, (HMENU)0, 0, 0);
	SendMessageA(d[0], WM_SETFONT, (WPARAM)UniversalFont, 2);

	d[1]= CreateWindowA("STATIC", "驱动程序状态:", WS_CHILD, 0, 58, 80, 21, Setup, (HMENU)0, 0, 0);
	SendMessageA(d[1], WM_SETFONT, (WPARAM)UniversalFont, 2);

	d[2] = CreateWindowA("STATIC", "--", WS_CHILD, 80, 58, 40, 21, Setup, (HMENU)0, 0, 0);
	SendMessageA(d[2], WM_SETFONT, (WPARAM)UniversalFont, 2);

	char buf[256] = { 0 };
	sprintf_s(buf, "CI.dll信息:\nCI.DLL基址:0x%p\ng_CiOptions地址:0x%p", Ci_Base, gCiOffset + Ci_Base);
	d[3] = CreateWindowA("STATIC", buf, WS_CHILD, 0, 80, 320, 50, Setup, (HMENU)0, 0, 0);
	SendMessageA(d[3], WM_SETFONT, (WPARAM)UniversalFont, 2);

	RtlZeroMemory(buf, sizeof(buf));
	sprintf_s(buf, "驱动程序信息:\nIridium服务名:%s\n当前状态:0x%x", drv_m_name, GetServiceStatus(drv_m_name));
	d[4] = CreateWindowA("STATIC", buf, WS_CHILD, 0, 140, 320, 50, Setup, (HMENU)0, 0, 0);
	SendMessageA(d[4], WM_SETFONT, (WPARAM)UniversalFont, 2);

	d[5] = CreateWindowA("BUTTON", "卸载驱动程序", WS_CHILD, 120, 58, 100, 21, Setup, (HMENU)Btn_UnlDrv, 0, 0);
	SendMessageA(d[5], WM_SETFONT, (WPARAM)UniversalFont, 2);

	d[6] = CreateWindowA("BUTTON", "加载驱动程序", WS_CHILD, 240, 58, 100, 21, Setup, (HMENU)Btn_ReloadDrv, 0, 0);
	SendMessageA(d[6], WM_SETFONT, (WPARAM)UniversalFont, 2);

	//关于组件
	ab[0] = CreateWindowA("STATIC", "欢迎使用Iridium! 一个面向Windows研究者和开发人员的开源ARK工具.在PCHunter在新版系统中失去作用后,有大量的新的开源ARK工具涌现,Iridium就是其中一个,我们实现了PCHunter中的大部分功能供开发者使用,能够尽可能的找出系统中的问题.但由于学业繁忙,代码可能写的很随意,功能也可能有Bug,请与我反馈,谢谢你们的支持!", WS_CHILD, 0, 25, 620, 50, Setup, (HMENU)0, 0, 0);
	SendMessageA(ab[0], WM_SETFONT, (WPARAM)UniversalFont, 2);

	ab[1] = CreateWindowA("STATIC", "支持的操作系统版本: Windows 10,Windows 11 64-bit only.\n适配Windows 7的版本将在后续更新中实现.\n\n许可证:GPLv3\n\nCopyright (C) XuanLaboratory 2025", WS_CHILD, 0, 100, 350, 120, Setup, (HMENU)0, 0, 0);
	SendMessageA(ab[1], WM_SETFONT, (WPARAM)UniversalFont, 2);

	return Setup;
}

DWORD Setup_CurrentSel = 0;

LRESULT CALLBACK Window_Setup(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	WORD dlgID = LOWORD(wParam);//控件ID
	WORD codeID = HIWORD(wParam); //消息代码
	HWND hDlg = (HWND)dlgID;
	Window_Setup_Msg_Cnt++;
	switch (Message)
	{
	case WM_INITDIALOG: {

		break;
	}

	case WM_TIMER: {
		switch (wParam)
		{
		case win_Setup_DrvStat_Timer: {
			if (TabCtrl_GetCurSel(SetupOpt) == 1) {
				if (GetServiceStatus(drv_m_name) == SERVICE_RUNNING) {
					SetWindowTextA(d[2], "活动");
				}
				else {
					SetWindowTextA(d[2], "离线");
				}
				UpdateWindow(d[2]);

				char buf[256] = { 0 };
				sprintf_s(buf, "驱动程序信息:\nIridium服务名:%s\n当前状态:0x%x", drv_m_name, GetServiceStatus(drv_m_name));
				SetWindowTextA(d[4], buf);
			}
			break;
		}
		default:
			break;
		}
		break;
	}

	case WM_DESTROY: {

		break;
	}
	case WM_NOTIFY: {
		NMHDR* pNMHDR = (NMHDR*)lParam;
		UINT ctrlID = pNMHDR->idFrom;  // 控件 ID
		UINT codeID2 = pNMHDR->code;
		if (ctrlID == 2) {
			switch (codeID2)
			{
			case TCN_SELCHANGE: {
				int id = TabCtrl_GetCurSel(pNMHDR->hwndFrom);
				Setup_CurrentSel = TabCtrl_GetCurSel(pNMHDR->hwndFrom);
				switch (id)
				{
				case 0: { //通用
					for (int i = 0; i < ARRAYSIZE(cm); i++) {
						if (cm[i] == 0) break;
						ShowWindowAsync(cm[i], 1);
					}

					for (int i = 0; i < ARRAYSIZE(d); i++) { //driver组件
						if (d[i] == 0) break;
						ShowWindowAsync(d[i], 0);
					}

					for (int i = 0; i < ARRAYSIZE(ab); i++) { //about组件
						if (ab[i] == 0) break;
						ShowWindowAsync(ab[i], 0);
					}
					break;
				}

				case 1: { //驱动
					for (int i = 0; i < ARRAYSIZE(cm); i++) {
						if (cm[i] == 0) break;
						ShowWindowAsync(cm[i], 0);
					}

					for (int i = 0; i < ARRAYSIZE(d); i++) { //driver组件
						if (d[i] == 0) break;
						ShowWindowAsync(d[i], 1);
					}

					for (int i = 0; i < ARRAYSIZE(ab); i++) { //about组件
						if (ab[i] == 0) break;
						ShowWindowAsync(ab[i], 0);
					}
					break;
				}

				case 2: { //关于
					for (int i = 0; i < ARRAYSIZE(cm); i++) {
						if (cm[i] == 0) break;
						ShowWindowAsync(cm[i], 0);
					}

					for (int i = 0; i < ARRAYSIZE(d); i++) {
						if (d[i] == 0) break;
						ShowWindowAsync(d[i], 0);
					}

					for (int i = 0; i < ARRAYSIZE(ab); i++) { //about组件
						if (ab[i] == 0) break;
						ShowWindowAsync(ab[i], 1);
					}
					break;
				}

				default:
					break;
				}
				break;
			}
			default:
				break;
			}
		}

		if (dlgID == 3) {

			break;
		}


		break;
	}

	case WM_COMMAND: {
		if (Setup_CurrentSel == 0 && codeID == BN_CLICKED) {
			ShowWindowAsync(cm[9], 1);
		}

		if (dlgID == Btn_cm_Save) {
			switch (codeID)
			{
			case BN_CLICKED: {
				BOOL r = 1;
				dbgout("Saving configuration...\n");
				printA("正在保存配置文件...\n", 1);
				r |= WriteConfigBoolean(Cfg, "General", "RandomName", IsDlgButtonChecked(Setup, Cb_Raname));
				r |= WriteConfigBoolean(Cfg, "General", "DebugMessage", IsDlgButtonChecked(Setup, Cb_DbgMsg));
				r |= WriteConfigBoolean(Cfg, "General", "AllowNetwork", !IsDlgButtonChecked(Setup, Cb_DNetwork));
				r |= WriteConfigBoolean(Cfg, "Driver", "LoadDriver", IsDlgButtonChecked(Setup, Cb_LdDrv));
				r |= WriteConfigBoolean(Cfg, "Driver", "Protect", IsDlgButtonChecked(Setup, Cb_Protect));
				r |= WriteConfigBoolean(Cfg, "Driver", "ProtectEx", IsDlgButtonChecked(Setup, Cb_ProtectEx));
				r |= WriteConfigBoolean(Cfg, "System", "CacheOffsets", IsDlgButtonChecked(Setup, Cb_ECache));
				r |= WriteConfigBoolean(Cfg, "Window", "DisableCapture", IsDlgButtonChecked(Setup, Cb_DisCapt));
				r |= WriteConfigBoolean(Cfg, "Window", "ForceOnTop", IsDlgButtonChecked(Setup, Cb_FOTop));
				r |= WriteConfigBoolean(Cfg, "System", "UseHotfix", IsDlgButtonChecked(Setup, Cb_UseHotfix));
				r |= WriteConfigBoolean(Cfg, "General", "DebuggerCheck", IsDlgButtonChecked(Setup, Cb_DbgChk));

				if (r) {
					MessageBoxA(Setup, "配置文件成功保存,新配置将在下一次启动时应用", "提示", MB_ICONINFORMATION | MB_OK);
					break;
				}
				else {
					MessageBoxA(Setup, "配置文件未能成功保存.", "警告", MB_ICONWARNING | MB_OK);
					break;
				}
				break;
			}
			default:
				break;
			}
		}

		if (dlgID == Btn_UnlDrv) {
			switch (codeID)
			{
			case BN_CLICKED: {
				BOOL r = 1;
				DWORD CurrentStatus = GetServiceStatus(drv_m_name);
				dbgout("Current Iridium service status:0x%x\n", CurrentStatus);

				if (CurrentStatus == SERVICE_RUNNING) {
					if (MessageBoxA(Setup, "是否要卸载Iridium内核模式驱动?\n卸载后,需要驱动辅助的功能将无法使用", "提示", MB_ICONWARNING | MB_YESNO) == IDYES) {
						dbgout("Unloading iridium driver...\n");
						HANDLE hDrv = CreateFileA("\\\\.\\Iridium", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hDrv == INVALID_HANDLE_VALUE) {
							MessageBoxA(Setup, "与Iridium内核模式驱动通信失败", "错误", MB_ICONERROR | MB_OK);
							break;
						}
						else {
							dbgout("Posting unload request to iridium driver...\n");
							DeviceIoControl(hDrv, IRIDIUM_DRV_UNLOAD, 0, 0, 0, 0, 0, 0);
						}
						CloseHandle(hDrv);
						r |= StopService(drv_m_name);
						Sleep(20);
						r |= UnInstallService(drv_m_name);

						if (!r) { MessageBoxA(Setup, "卸载Iridium内核模式驱动失败", "错误", MB_ICONERROR | MB_OK); break; }

						MessageBoxA(Setup, "成功卸载了Iridium驱动程序", "提示", MB_ICONINFORMATION | MB_OK);
						break;
					}
					else {
						break;
					}
				}
				else {
					MessageBoxA(Setup, "Iridium驱动程序未被加载.", "错误", MB_ICONERROR | MB_OK);
					break;
				}
			}
			default:
				break;
			}
			break;
		}

		if (dlgID == Btn_ReloadDrv) {
			switch (codeID)
			{
			case BN_CLICKED: {
				HANDLE hDrv = CreateFileA("\\\\.\\Iridium", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hDrv != INVALID_HANDLE_VALUE) {
					CloseHandle(hDrv);
					MessageBoxA(Setup, "Iridium驱动程序已经加载,在加载前,请先卸载驱动程序", "错误", MB_ICONERROR | MB_OK);
					break;
				}

				dbgout("Loading iridium driver...\n");
				if (!__DriverInit()) {
					MessageBoxA(Setup, "Iridium驱动程序加载成功", "错误", MB_ICONINFORMATION | MB_OK);
					break;
				}
				else {
					MessageBoxA(Setup, "Iridium加载失败", "错误", MB_ICONERROR | MB_OK);
					break;
				}

			}
			default:
				break;
			}
			break;
		}

		break;
	}
	
	default:
		return DefWindowProcW(hWnd, Message, wParam, lParam);
	}

	return false;
}

INT_PTR CALLBACK Window_IID(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND iid[11] = { 0 };
HWND DebugWindow = 0;

HWND __AssembleIIDGUI() {
	dbgout("__AssembleSetupGUI() was called.\n");
	WNDCLASSW Reg = { 0 }; //主窗口回调
	HINSTANCE MainWin = 0;
	Reg.cbClsExtra = 0;
	Reg.cbWndExtra = 0;
	Reg.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	Reg.hCursor = LoadCursorW(0, IDC_ARROW);
	Reg.hIcon = IrIcon;
	Reg.hInstance = hInstance;
	Reg.lpfnWndProc = Window_IID;
	Reg.lpszClassName = L"IridiumDbg";
	Reg.lpszMenuName = 0;
	Reg.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassW(&Reg);

	DebugWindow = CreateWindowA("IridiumDbg", "IID", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 280, 300, 0, (HMENU)0, MainWin, 0);
	if (!DebugWindow) {
		MessageBoxA(0, "Error:Create the setup window failed!", "错误", MB_OK | MB_ICONERROR);
		return 0;
	}

	HMENU MainMenu = GetSystemMenu(DebugWindow, false);		// 复制或修改而访问窗口菜单
	RemoveMenu(MainMenu, SC_MAXIMIZE, MF_BYCOMMAND);	// 从指定菜单删除一个菜单项或分离一个子菜单
	RemoveMenu(MainMenu, SC_MINIMIZE, MF_BYCOMMAND);
	RemoveMenu(MainMenu, SC_CLOSE, MF_BYCOMMAND);
	DrawMenuBar(DebugWindow);

	HFONT hFont = UniversalFont;

	DWORD dwFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE;
	HWND hwndIns = HWND_TOPMOST;
	SetWindowPos(DebugWindow, hwndIns, 0, 0, 0, 0, dwFlags);

	// 内存地址标签
	iid[1] = CreateWindowA("STATIC", "内存地址:",
		WS_CHILD | WS_VISIBLE,
		5, 5, 55, 22,
		DebugWindow, (HMENU)0, hInstance, 0);
	SendMessageA(iid[1], WM_SETFONT, (WPARAM)hFont, 2);

	// 内存地址编辑框
	iid[2] = CreateWindowA("EDIT", "",
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_UPPERCASE | ES_AUTOHSCROLL,
		65, 5, 155, 22,
		DebugWindow, (HMENU)111, hInstance, 0);
	SendMessageA(iid[2], WM_SETFONT, (WPARAM)hFont, 2);

	// 数据大小标签
	iid[3] = CreateWindowA("STATIC", "数据大小:",
		WS_CHILD | WS_VISIBLE,
		5, 30, 55, 20,
		DebugWindow, (HMENU)0, hInstance, 0);
	SendMessageA(iid[3], WM_SETFONT, (WPARAM)hFont, 2);

	// 数据大小编辑框
	iid[4] = CreateWindowA("EDIT", "",
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
		65, 30, 77, 20,
		DebugWindow, (HMENU)2, hInstance, 0);
	SendMessageA(iid[4], WM_SETFONT, (WPARAM)hFont, 2);

	// 读取按钮
	iid[5] = CreateWindowA("BUTTON", "读取",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		5, 55, 50, 25,
		DebugWindow, (HMENU)3, hInstance, 0);
	SendMessageA(iid[5], WM_SETFONT, (WPARAM)hFont, 2);

	// 写入按钮
	iid[6] = CreateWindowA("BUTTON", "写入",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		60, 55, 50, 25,
		DebugWindow, (HMENU)4, hInstance, 0);
	SendMessageA(iid[6], WM_SETFONT, (WPARAM)hFont, 2);

	// 清零按钮
	iid[7] = CreateWindowA("BUTTON", "清零内存",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		115, 55, 50, 25,
		DebugWindow, (HMENU)5, hInstance, 0);
	SendMessageA(iid[7], WM_SETFONT, (WPARAM)hFont, 2);

	// 编辑/查看标签
	iid[8] = CreateWindowA("STATIC", "编辑/查看 (HEX)",
		WS_CHILD | WS_VISIBLE,
		5, 85, 100, 20,
		DebugWindow, (HMENU)0, hInstance, 0);
	SendMessageA(iid[8], WM_SETFONT, (WPARAM)hFont, 2);

	// 输出编辑框
	iid[9] = CreateWindowA("EDIT", "",
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
		5, 105, 270, 150,
		DebugWindow, (HMENU)7, hInstance, 0);
	SendMessageA(iid[9], WM_SETFONT, (WPARAM)hFont, 2);

	iid[10] = CreateWindowA("BUTTON", "分配内存",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		170, 55, 50, 25,
		DebugWindow, (HMENU)6, hInstance, 0);
	SendMessageA(iid[10], WM_SETFONT, (WPARAM)hFont, 2);

	return DebugWindow;
}

INT_PTR CALLBACK Window_IID(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	WORD dlgID = LOWORD(wParam);//控件ID
	WORD codeID = HIWORD(wParam); //消息代码
	HWND hDlg = (HWND)dlgID;

	switch (uMsg)
	{
	case WM_INITDIALOG: {

		break;
	}

	case WM_TIMER: {
		break;
	}

	case WM_DESTROY: {

		break;
	}
	case WM_NOTIFY: {
		NMHDR* pNMHDR = (NMHDR*)lParam;
		UINT ctrlID = pNMHDR->idFrom;  // 控件 ID
		UINT codeID2 = pNMHDR->code;

		break;
	}

	case WM_COMMAND: {   //地址2 大小4
		if (dlgID == 3) { //读取
			switch (codeID)
			{
			case BN_CLICKED: {
				char buffer[64] = { 0 };
				LONGLONG addr = 0, size = 0;
				char* endptr = NULL;

				// 读取内存地址（十六进制）
				GetWindowTextA(iid[2], buffer, sizeof(buffer));

				if (strlen(buffer) > 0) {
					addr = strtoll(buffer, &endptr, 16);
					if (endptr == buffer || *endptr != '\0') {
						MessageBoxA(DebugWindow, "内存地址格式错误", "IID", MB_ICONERROR | MB_OK);
						break;
					}
				}
				else {
					MessageBoxA(DebugWindow, "请输入内存地址", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				// 读取数据大小（十进制）
				RtlZeroMemory(buffer, sizeof(buffer));
				GetWindowTextA(iid[4], buffer, sizeof(buffer));

				if (strlen(buffer) > 0) {
					size = strtoll(buffer, &endptr, 10);
					if (endptr == buffer || *endptr != '\0' || size <= 0) {
						MessageBoxA(DebugWindow, "数据大小格式错误", "IID", MB_ICONERROR | MB_OK);
						break;
					}
				}
				else {
					MessageBoxA(DebugWindow, "请输入数据大小", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				dbgout("Trying to read: ");
				if (Config->DebugMessage) printf("Address=0x%08X, Size=%uByte\n", addr, size);

				// 分配内存缓冲区
				char* membuf = (char*)malloc(size + 1);
				if (!membuf) {
					MessageBoxA(DebugWindow, "内存分配失败", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				RtlZeroMemory(membuf, size + 1);
				memcpy(membuf, (PVOID)addr, size);

				// 将二进制数据转换为十六进制字符串显示
				char* hexOutput = (char*)malloc(size * 3 + 1); // 每个字节需要3个字符（2个十六进制+1个空格）
				if (!hexOutput) {
					MessageBoxA(DebugWindow, "输出缓冲区分配失败", "IID", MB_ICONERROR | MB_OK);
					free(membuf);
					break;
				}

				RtlZeroMemory(hexOutput, size * 3 + 1);

				// 转换为十六进制字符串
				char* ptr = hexOutput;
				for (DWORD i = 0; i < size; i++) {
					sprintf_s(ptr, 4, "%02X ", (unsigned char)membuf[i]);
					ptr += 3;
				}

				// 显示结果
				char* result = (char*)malloc(size * 3 + 2);
				sprintf_s(result, size * 3 + 2, "%s", hexOutput);

				SetWindowTextA(iid[9], result);
				UpdateWindow(iid[9]);

				free(hexOutput);
				free(result);
				free(membuf);
				break;
			}
			default:
				break;
			}
			break;
		}

		if (dlgID == 4) { //写入
			switch (codeID)
			{
			case BN_CLICKED: {
				char buffer[64] = { 0 };
				LONGLONG addr = 0, size = 0;
				char* endptr = NULL;

				// 读取内存地址（十六进制）
				GetWindowTextA(iid[2], buffer, sizeof(buffer));

				if (strlen(buffer) > 0) {
					addr = strtoll(buffer, &endptr, 16);
					if (endptr == buffer || *endptr != '\0') {
						MessageBoxA(DebugWindow, "内存地址格式错误", "IID", MB_ICONERROR | MB_OK);
						break;
					}
				}
				else {
					MessageBoxA(DebugWindow, "请输入内存地址", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				// 读取数据大小（十进制）- 对于写入操作，这个字段是可选的
				RtlZeroMemory(buffer, sizeof(buffer));
				GetWindowTextA(iid[4], buffer, sizeof(buffer));

				// 获取输出框中的十六进制数据
				char hexData[4096] = { 0 };
				GetWindowTextA(iid[9], hexData, sizeof(hexData));

				if (strlen(hexData) == 0) {
					MessageBoxA(DebugWindow, "请输入要写入的十六进制数据", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				// 过滤十六进制数据（移除空格、换行等非十六进制字符）
				char filteredData[4096] = { 0 };
				int filteredIndex = 0;

				for (int i = 0; hexData[i] != '\0' && filteredIndex < sizeof(filteredData) - 1; i++) {
					char c = hexData[i];
					if ((c >= '0' && c <= '9') ||
						(c >= 'A' && c <= 'F') ||
						(c >= 'a' && c <= 'f')) {
						// 转换为大写
						if (c >= 'a' && c <= 'f') {
							c = c - 32;
						}
						filteredData[filteredIndex++] = c;
					}
					// 忽略空格、换行、制表符等其他字符
				}
				filteredData[filteredIndex] = '\0';

				// 检查十六进制数据长度是否为偶数（每个字节需要2个十六进制字符）
				int hexLength = strlen(filteredData);
				if (hexLength % 2 != 0) {
					MessageBoxA(DebugWindow, "十六进制数据长度必须为偶数", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				// 计算实际数据大小（字节数）
				int dataSize = hexLength / 2;

				// 如果用户指定了大小，使用用户指定的大小（但不能小于实际数据大小）
				if (strlen(buffer) > 0) {
					size = strtoll(buffer, &endptr, 10);
					if (endptr == buffer || *endptr != '\0' || size <= 0) {
						MessageBoxA(DebugWindow, "数据大小格式错误", "IID", MB_ICONERROR | MB_OK);
						break;
					}

					if (size < dataSize) {
						MessageBoxA(DebugWindow, "指定的数据大小小于实际数据大小", "IID", MB_ICONERROR | MB_OK);
						break;
					}
				}
				else {
					// 没有指定大小，使用数据实际大小
					size = dataSize;
				}

				dbgout("Trying to write: ");
				if (Config->DebugMessage) printf("Address=0x%08X, Size=%uByte\n", addr, size);

				// 分配内存缓冲区
				char* membuf = (char*)malloc(size);
				if (!membuf) {
					MessageBoxA(DebugWindow, "内存分配失败", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				RtlZeroMemory(membuf, size);

				// 将十六进制字符串转换为二进制数据
				for (int i = 0; i < dataSize; i++) {
					char hexByte[3] = { filteredData[i * 2], filteredData[i * 2 + 1], '\0' };
					membuf[i] = (char)strtoul(hexByte, NULL, 16);
				}

				// 使用SEH安全地写入内存
				BOOL writeSuccess = FALSE;

				// 设置异常处理器
				PVOID oldHandler = AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS ExceptionInfo) -> LONG {
					if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
						return EXCEPTION_EXECUTE_HANDLER;
					}
					return EXCEPTION_CONTINUE_SEARCH;
					});

				__try {
					// 尝试写入内存

					if (MessageBoxA(DebugWindow, "写入数据到该地址?", "IID", MB_ICONQUESTION | MB_YESNO) == IDYES) {
						memcpy((PVOID)addr, membuf, size);
						writeSuccess = TRUE;
					}
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					// 内存访问异常处理
					writeSuccess = FALSE;
				}

				// 移除异常处理器
				if (oldHandler) {
					RemoveVectoredExceptionHandler(oldHandler);
				}


				// 释放内存
				free(membuf);
				break;
			}
			default:
				break;
			}
			break;
		}

		if (dlgID == 5) { //清零
			switch (codeID)
			{
			case BN_CLICKED: {
				char buffer[64] = { 0 };
				LONGLONG addr = 0, size = 0;
				char* endptr = NULL;

				// 读取内存地址（十六进制）
				GetWindowTextA(iid[2], buffer, sizeof(buffer));

				if (strlen(buffer) > 0) {
					addr = strtoll(buffer, &endptr, 16);
					if (endptr == buffer || *endptr != '\0') {
						MessageBoxA(DebugWindow, "内存地址格式错误", "IID", MB_ICONERROR | MB_OK);
						break;
					}
				}
				else {
					MessageBoxA(DebugWindow, "请输入内存地址", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				// 读取数据大小（十进制）
				RtlZeroMemory(buffer, sizeof(buffer));
				GetWindowTextA(iid[4], buffer, sizeof(buffer));

				if (strlen(buffer) > 0) {
					size = strtoll(buffer, &endptr, 10);
					if (endptr == buffer || *endptr != '\0' || size <= 0) {
						MessageBoxA(DebugWindow, "数据大小格式错误", "IID", MB_ICONERROR | MB_OK);
						break;
					}
				}
				else {
					MessageBoxA(DebugWindow, "请输入数据大小", "IID", MB_ICONERROR | MB_OK);
					break;
				}

				dbgout("Trying to zero-memory: ");
				if (Config->DebugMessage) printf("Address=0x%08X, Size=%uByte\n", addr, size);
				__try {

					if (MessageBoxA(DebugWindow, "清除该地址的内存数据?", "IID", MB_ICONQUESTION | MB_YESNO) == IDYES) {
						RtlZeroMemory((PVOID)addr, size);
					}

				}

				__except (EXCEPTION_EXECUTE_HANDLER) {
					MessageBoxA(DebugWindow, "清零内存失败!", "IID", MB_ICONERROR | MB_OK);
				}

				break;
			}
			default:
				break;
			}
			break;
		}

		if (dlgID == 6) {
			char buffer[64] = { 0 };
			LONGLONG size = 0;
			char* endptr = NULL;
			GetWindowTextA(iid[4], buffer, sizeof(buffer));

			if (strlen(buffer) > 0) {
				size = strtoll(buffer, &endptr, 10);
				if (endptr == buffer || *endptr != '\0' || size <= 0) {
					MessageBoxA(DebugWindow, "数据大小格式错误", "IID", MB_ICONERROR | MB_OK);
					break;
				}
			}
			else {
				MessageBoxA(DebugWindow, "请输入数据大小", "IID", MB_ICONERROR | MB_OK);
				break;
			}

			dbgout("Trying to allocate memory,size=");
			printf("%dByte\n", size);

			RtlZeroMemory(buffer, 64);

			sprintf_s(buffer, "0x%p", malloc(size));
			SetWindowTextA(iid[2], buffer);
			break;
		}
		break;
	}
				   
	default:
		return DefWindowProcW(hwndDlg, uMsg, wParam, lParam);
	}

	return false;
}

INT_PTR CALLBACK Window_PLWC(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND PopupListWindow(LPCSTR title, DWORD x, DWORD y, PVOID DataStringBase, PVOID ValDataBase, DWORD dcount) {
	dbgout("PopupListWindow() was called.\n");
	HWND list = 0, dl = 0;

	WNDCLASSW Reg = { 0 }; //主窗口回调
	HINSTANCE MainWin = 0;
	Reg.cbClsExtra = 0;
	Reg.cbWndExtra = 0;
	Reg.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	Reg.hCursor = LoadCursorW(0, IDC_ARROW);
	Reg.hIcon = IrIcon;
	Reg.hInstance = hInstance;
	Reg.lpfnWndProc = Window_PLWC;
	Reg.lpszClassName = L"IridiumListWindow";
	Reg.lpszMenuName = 0;
	Reg.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassW(&Reg);

	list = CreateWindowA("IridiumListWindow", title, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, x, y, 0, (HMENU)0, MainWin, 0);
	if (!list) {
		return 0;
	}

	dl = CreateWindowExA(0, WC_LISTVIEWA, NULL, WS_VISIBLE | WS_CHILD | LVS_REPORT, 0, 0, x, y - 15, list, (HMENU)3, 0, 0);
	if (!dl) {
		DestroyWindow(list);
		return 0;
	}
	ListView_SetExtendedListViewStyle(dl, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	LVCOLUMN TempLvc = { 0 }; //可复用
	TempLvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;

	LPSTR* data = (LPSTR*)DataStringBase;
	LPSTR* val = (LPSTR*)ValDataBase;
	LPCWSTR Header[] = { L"项目",L"值"};
	DWORD len[] = { x / 2,x / 2 };
	for (int i = 0; i < ARRAYSIZE(Header); i++) {
		TempLvc.pszText = (LPWSTR)Header[i];
		TempLvc.cx = len[i];
		TempLvc.iSubItem = i;
		ListView_InsertColumn(dl, i, &TempLvc);
	}
	LVITEMA lvItem = { 0 };
	lvItem.mask = LVIF_TEXT;
	for (int i = 0; i < dcount; i++) {
		// 插入新行（设置第一列）
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		lvItem.pszText = data[i];
		SendMessageA(dl, LVM_INSERTITEMA, 0, (LPARAM)&lvItem);

		// 设置第二列及其他子项的文本
		lvItem.iSubItem = 1;
		lvItem.pszText = val[i];
		SendMessageA(dl, LVM_SETITEMA, 0, (LPARAM)&lvItem);
	}
	SendMessageW(dl, WM_SETREDRAW, TRUE, 0);
	return list;
}

INT_PTR CALLBACK Window_PLWC(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {


	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

HWND PopupInputWindow(LPCSTR title, LPCSTR inst, DWORD x, DWORD y, PVOID buffer) {
	dbgout("PopupInputWindow() was called.\n");
	HWND bg = 0, ed = 0, ex = 0;

	WNDCLASSW Reg = { 0 }; //主窗口回调
	HINSTANCE MainWin = 0;
	Reg.cbClsExtra = 0;
	Reg.cbWndExtra = 0;
	Reg.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	Reg.hCursor = LoadCursorW(0, IDC_ARROW);
	Reg.hIcon = IrIcon;
	Reg.hInstance = hInstance;
	Reg.lpfnWndProc = Window_PLWC;
	Reg.lpszClassName = L"IridiumInputWindow";
	Reg.lpszMenuName = 0;
	Reg.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassW(&Reg);

	// 计算窗口实际大小（包含边框和标题栏）
	RECT rect = { 0, 0, (LONG)x, (LONG)y };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME, FALSE);
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;

	bg = CreateWindowA("IridiumInputWindow", title,
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight, 0, (HMENU)0, MainWin, 0);
	if (!bg) {
		return 0;
	}

	// 计算控件位置和大小（使用客户区坐标）
	RECT clientRect;
	GetClientRect(bg, &clientRect);
	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;

	// 静态文本控件 - 显示说明文字
	HWND s = CreateWindowA("STATIC", inst,
		WS_CHILD | WS_VISIBLE,
		10, 15, clientWidth - 20, 30, bg, 0, 0, 0);
	SendMessageA(s, WM_SETFONT, (WPARAM)UniversalFont, TRUE);

	// 编辑框控件
	int editHeight = 25;
	ed = CreateWindowA("EDIT", "",
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
		10, 50, clientWidth - 20, editHeight, bg, 0, 0, 0);
	SendMessageA(ed, WM_SETFONT, (WPARAM)UniversalFont, TRUE);

	// 确定按钮
	int buttonWidth = 60;
	int buttonHeight = 25;
	ex = CreateWindowA("BUTTON", "确定",
		WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
		clientWidth - buttonWidth - 10,
		clientHeight - buttonHeight - 10,
		buttonWidth, buttonHeight, bg, 0, 0, 0);
	SendMessageA(ex, WM_SETFONT, (WPARAM)UniversalFont, TRUE);

	// 设置焦点到编辑框
	SetFocus(ed);

	HMENU MainMenu = GetSystemMenu(bg, false);		// 复制或修改而访问窗口菜单
	RemoveMenu(MainMenu, SC_MAXIMIZE, MF_BYCOMMAND);	// 从指定菜单删除一个菜单项或分离一个子菜单
	RemoveMenu(MainMenu, SC_MINIMIZE, MF_BYCOMMAND);
	DrawMenuBar(DebugWindow);

	// 显示窗口
	ShowWindow(bg, SW_SHOW);
	UpdateWindow(bg);

	return bg;
}

void __GUI_Summary() {
	dbgout("Number of message of window_Main : ");
	if (Config->DebugMessage) printf("%lld\n", Window_Main_Msg_Cnt);
	dbgout("Number of message of window_Setup : ");
	if (Config->DebugMessage) printf("%lld\n", Window_Setup_Msg_Cnt);
}
