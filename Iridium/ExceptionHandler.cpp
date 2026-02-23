#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include <windows.h>
#include <psapi.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <intrin.h>
#include <dbghelp.h>
#include <algorithm>
#include <cctype>
#include <iterator>
#include "FunctionCall.h"
#include "Console.h"
#include "Define.h"
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
//#pragma comment(lib, "ucrtd.lib")
//#pragma comment(lib, "vcruntimed.lib")

BOOL IsDebuggerPresented = 1;

void WriteToFile(HANDLE hFile, const std::string& content) {
    DWORD bytesWritten;
    BOOL b = WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.size()), &bytesWritten, nullptr);
}

// 生成16进制内存dump
void GenerateHexDump(HANDLE hProcess, const void* address, size_t size, HANDLE hFile) {
    const size_t bytesPerLine = 16;
    unsigned char buffer[bytesPerLine];
    SIZE_T bytesRead = 0;

    for (size_t offset = 0; offset < size; offset += bytesPerLine) {
        SIZE_T readSize = (std::min)(bytesPerLine, size - offset);

        // 尝试读取内存
        if (!ReadProcessMemory(hProcess, static_cast<const char*>(address) + offset,
            buffer, readSize, &bytesRead) || bytesRead == 0) {
            std::stringstream errorLine;
            errorLine << "  [ReadProcessMemory failed at 0x" << std::hex
                << reinterpret_cast<uintptr_t>(static_cast<const char*>(address) + offset)
                << ", Error: " << GetLastError() << "]\n";
            WriteToFile(hFile, errorLine.str());
            break;
        }

        std::stringstream line;
        // 地址偏移
        uintptr_t currentAddr = reinterpret_cast<uintptr_t>(static_cast<const char*>(address) + offset);
        line << "0x" << std::setw(16) << std::setfill('0') << std::hex << currentAddr << ": ";

        // 16进制数据
        for (size_t i = 0; i < readSize; i++) {
            line << std::setw(2) << std::setfill('0') << std::hex
                << static_cast<int>(buffer[i]) << " ";
        }

        // 对齐格式
        if (readSize < bytesPerLine) {
            for (size_t i = readSize; i < bytesPerLine; i++) {
                line << "   ";
            }
        }

        // ASCII表示
        line << " ";
        for (size_t i = 0; i < readSize; i++) {
            line << (std::isprint(buffer[i]) ? static_cast<char>(buffer[i]) : '.');
        }
        line << "\n";

        WriteToFile(hFile, line.str());
    }
}

// 检查内存是否可读
bool IsMemoryReadable(const MEMORY_BASIC_INFORMATION& mbi) {
    return (mbi.State == MEM_COMMIT) &&
        !(mbi.Protect & PAGE_NOACCESS) &&
        (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE |
            PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
            PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY)) &&
        !(mbi.Protect & PAGE_GUARD);
}

//dbg-detector
LONG WINAPI PreExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    printf("Exception has been collected.\n");
    IsDebuggerPresented = 0;

    if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) return EXCEPTION_CONTINUE_EXECUTION;
    if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_FLT_DIVIDE_BY_ZERO) return EXCEPTION_CONTINUE_EXECUTION;

    return EXCEPTION_EXECUTE_HANDLER;
}


// 异常处理器
LONG WINAPI CustomExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) return EXCEPTION_CONTINUE_EXECUTION;
    DestroyWindow(MainWindow);
    MessageBeep(MB_ICONERROR);
    printf("*****************************************************************\n");
    printf(">Oops! 程序在地址为0x%x的地方挂了,代码:0x%x.\n", pExceptionInfo->ExceptionRecord->ExceptionAddress, pExceptionInfo->ExceptionRecord->ExceptionCode);
    printf(">错误 : ");
    printC("程序产生了一个致命错误,无法继续运行.正在生成转储文件...\n", C_LRED);
    printf(">我们只收集某些错误信息,然后为你重新启动程序.你可以将转储文件上传至Iridium服务器,也可以直接删除它们.\n");
    printf(">正在写入快速参考文件...\n");
    HANDLE hProcess = GetCurrentProcess();
    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);

    // 创建dump文件
    char dumpFileName[MAX_PATH];
    sprintf_s(dumpFileName, "CrashDump_%04d%02d%02d_%02d%02d%02d.txt",
        sysTime.wYear, sysTime.wMonth, sysTime.wDay,
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

    // 使用WinAPI创建文件
    HANDLE hFile = CreateFileA(dumpFileName, GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return EXCEPTION_CONTINUE_SEARCH;

    // 写入寄存器信息
    PCONTEXT ctx = pExceptionInfo->ContextRecord;
    std::stringstream header;
    header << "Iridium Version : " << IRIDIUM_VERSION << "\n";
    header << "Crashed at 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionAddress << " With code 0x" << pExceptionInfo->ExceptionRecord->ExceptionCode << "\n\n\n";
    header << ">>>注意:该文件仅供快速参考,若需要分析,请移步至同名的.dmp文件<<<\n\n";
    header << "=== CPU Registers ===\n";
    header << "RAX: 0x" << std::hex << ctx->Rax << "\n";
    header << "RBX: 0x" << std::hex << ctx->Rbx << "\n";
    header << "RCX: 0x" << std::hex << ctx->Rcx << "\n";
    header << "RDX: 0x" << std::hex << ctx->Rdx << "\n";
    header << "RSI: 0x" << std::hex << ctx->Rsi << "\n";
    header << "RDI: 0x" << std::hex << ctx->Rdi << "\n";
    header << "RBP: 0x" << std::hex << ctx->Rbp << "\n";
    header << "RSP: 0x" << std::hex << ctx->Rsp << "\n";
    header << "R8: 0x" << std::hex << ctx->R8 << "\n";
    header << "R9: 0x" << std::hex << ctx->R9 << "\n";
    header << "R10: 0x" << std::hex << ctx->R10 << "\n";
    header << "R11: 0x" << std::hex << ctx->R11 << "\n";
    header << "R12: 0x" << std::hex << ctx->R12 << "\n";
    header << "R13: 0x" << std::hex << ctx->R13 << "\n";
    header << "R14: 0x" << std::hex << ctx->R14 << "\n";
    header << "R15: 0x" << std::hex << ctx->R15 << "\n";
    header << "RIP: 0x" << std::hex << ctx->Rip << "\n";
    header << "EFLAGS: 0x" << std::hex << ctx->EFlags << "\n\n";

    // 添加段寄存器信息
    header << "=== Segment Registers ===\n";
    header << "CS: 0x" << std::hex << ctx->SegCs << "\n";
    header << "DS: 0x" << std::hex << ctx->SegDs << "\n";
    header << "ES: 0x" << std::hex << ctx->SegEs << "\n";
    header << "FS: 0x" << std::hex << ctx->SegFs << "\n";
    header << "GS: 0x" << std::hex << ctx->SegGs << "\n";
    header << "SS: 0x" << std::hex << ctx->SegSs << "\n\n";

    WriteToFile(hFile, header.str());
    printf(">CPU寄存器数据转储完成.\n");

    // 遍历内存区域 - 从低地址开始
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = 0;
    size_t totalRegions = 0;
    size_t dumpedRegions = 0;

    std::string regionHeader = "=== Memory Dump ===\n\n";
    WriteToFile(hFile, regionHeader);

    // 获取系统信息以确定地址范围
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

    std::stringstream info;
    info << "内存转储范围: 0x0 到 0x" << std::hex << maxAddress << "\n";
    WriteToFile(hFile, info.str());

    while (address < maxAddress) {
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
            DWORD err = GetLastError();
            if (err == ERROR_INVALID_PARAMETER) break; // 到达地址空间末尾

            std::stringstream errorMsg;
            errorMsg << "VirtualQuery 失败,在 0x" << std::hex << address
                << ", 错误: " << err << "\n";
            WriteToFile(hFile, errorMsg.str());
            break;
        }

        totalRegions++;

        // 检查是否可读
        if (IsMemoryReadable(mbi)) {
            dumpedRegions++;

            std::stringstream regionInfo;
            uintptr_t startAddr = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            uintptr_t endAddr = startAddr + mbi.RegionSize;

            regionInfo << "块 " << dumpedRegions << ": 0x" << std::hex << startAddr
                << " - 0x" << endAddr
                << " (" << std::dec << mbi.RegionSize << " 字节, 保护: 0x"
                << std::hex << mbi.Protect << ")\n";
            WriteToFile(hFile, regionInfo.str());

            // 生成该区域的16进制dump (限制大小)
            size_t dumpSize = (std::min)(static_cast<size_t>(mbi.RegionSize), static_cast<size_t>(512));
            GenerateHexDump(hProcess, mbi.BaseAddress, dumpSize, hFile);

            WriteToFile(hFile, "\n"); // 区域间分隔
        }

        // 移动到下一个区域
        address = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;

        // 防止无限循环
        if (mbi.RegionSize == 0) break;
        if (address == 0) break; // 地址回绕
    }

    // 添加内存遍历摘要
    std::stringstream summary;
    summary << "\n=== Memory Dump Summary ===\n";
    summary << "扫描到的内存块总数: " << totalRegions << "\n";
    summary << "可转储的内存块总数: " << dumpedRegions << "\n";
    printf(">内存数据转储完成.\n");
    WriteToFile(hFile, summary.str());

    // 确保所有内容写入磁盘
    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    //正式的DUMP
    printf(">正在生成可分析的转储文件...\n");
    sprintf_s(dumpFileName, "CrashDump_%04d%02d%02d_%02d%02d%02d.dmp",
        sysTime.wYear, sysTime.wMonth, sysTime.wDay,
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
    hFile = CreateFileA(dumpFileName, GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return EXCEPTION_CONTINUE_SEARCH;

    // 写入minidump
    MINIDUMP_EXCEPTION_INFORMATION mei;
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = pExceptionInfo;
    mei.ClientPointers = FALSE;


    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &mei, nullptr, nullptr);

    CloseHandle(hFile);

    printf(">完成.\n");

    printf("*****************************************************************\n");

    Sleep(1000);

    WriteConfigInt(Cfg, "General", "CrashCount", CrashCount + 1);

    CreateProcessNew(".\\Iridium.exe", 0);
    FreeConsole();

    ExitProcess(pExceptionInfo->ExceptionRecord->ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL WINAPI ConsoleHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT: {
            HWND h = MainWindow ? MainWindow : GetConsoleWindow();
            if (MessageBoxA(h, "你似乎按下了Ctrl-C组合键,是否要退出Iridium?", "提示", MB_ICONINFORMATION | MB_YESNO) == IDYES) {
                DestroyWindow(MainWindow);
                PostMessageA(MainWindow, WM_QUIT, 0, 0);
            }
            return TRUE;
        }
        case CTRL_CLOSE_EVENT:
            return TRUE;
        default:
            return FALSE; // Pass signal to the next handler
    }
}


void SetupCrashReporting() {
    SetUnhandledExceptionFilter(CustomExceptionHandler);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
}

