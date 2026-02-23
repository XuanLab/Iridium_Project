#include <Windows.h>
#include <stdio.h>
#include <mutex>
#include "FunctionCall.h"
#include "Console.h"
#include "Define.h"
#include "Structures.h"

static std::mutex g_consoleMutex; // 全局互斥锁

void dbgout(const char* format, ...) {
    if (!Config) return;
    if (!Config->DebugMessage) return;
    SYSTEMTIME st;
    GetLocalTime(&st);

    // 获取控制台句柄
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;

    // 获取原始颜色属性
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    WORD originalAttr = csbi.wAttributes;

    // 线程安全输出
    std::lock_guard<std::mutex> lock(g_consoleMutex);

    // 输出时间部分（默认颜色）
    printf("[%02d:%02d:%02d.%03d]",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    // 输出错误等级（仅文本变色）
    printf("[");
    SetConsoleTextAttribute(hConsole, (C_CYAN & 0x0F) | (C_BLACK << 4));
    printf("DEBUG");
    SetConsoleTextAttribute(hConsole, originalAttr);
    printf("] ");

    // 输出消息内容（默认颜色）
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    return;
}

void printA(const char* msg, int ErrLevel) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    // 准备错误等级信息
    const char* levelStr;
    WORD levelColor;
    switch (ErrLevel) {
    case 1: levelStr = "INFO";  levelColor = C_GREEN;  break;
    case 2: levelStr = "WARN";  levelColor = C_YELLOW; break;
    case 3: levelStr = "ERROR"; levelColor = C_LRED;   break;
    case 4: levelStr = "FATAL"; levelColor = C_LRED;   break;
    default:levelStr = "INFO";  levelColor = C_GREEN;  break;
    }

    // 获取控制台句柄
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;

    // 获取原始颜色属性
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    WORD originalAttr = csbi.wAttributes;

    // 线程安全输出
    std::lock_guard<std::mutex> lock(g_consoleMutex);

    // 输出时间部分（默认颜色）
    printf("[%02d:%02d:%02d.%03d]",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    // 输出错误等级（仅文本变色）
    printf("[");
    SetConsoleTextAttribute(hConsole, (levelColor & 0x0F) | (C_BLACK << 4));
    printf("%s", levelStr);
    SetConsoleTextAttribute(hConsole, originalAttr);
    printf("] ");

    // 输出消息内容（默认颜色）
    printf("%s", msg);
}

void printAE(const char* msg, int color) {
    printA(msg, color);
    printf(" 可能的信息:%s\n", TranslateGetLastErrorMsg(GetLastError()));
}

void printC(const char* msg, int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    WORD originalAttr = csbi.wAttributes;

    std::lock_guard<std::mutex> lock(g_consoleMutex);

    SetConsoleTextAttribute(hConsole, (color & 0x0F) | (C_BLACK << 4));
    printf("%s\n", msg);
    SetConsoleTextAttribute(hConsole, originalAttr);
}


// 清除当前行内容（多线程安全版）
void ClearLine() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    // 使用ANSI清行
    printf("\x1B[2K\r"); // 清除整行
}

void ClearCurrentLine() {
    std::lock_guard<std::mutex> lock(g_consoleMutex);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    // 填充空格清除整行
    COORD pos = { 0, csbi.dwCursorPosition.Y };
    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, pos, &written);

    // 光标复位到行首
    SetConsoleCursorPosition(hConsole, pos);
}

void printAr(const char* msg, int ErrLevel) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    // 准备错误等级信息
    const char* levelStr;
    WORD levelColor;
    switch (ErrLevel) {
    case 1: levelStr = "INFO";  levelColor = C_GREEN;  break;
    case 2: levelStr = "WARN";  levelColor = C_YELLOW; break;
    case 3: levelStr = "ERROR"; levelColor = C_LRED;   break;
    case 4: levelStr = "FATAL"; levelColor = C_LRED;   break;
    default:levelStr = "INFO";  levelColor = C_GREEN;  break;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    WORD originalAttr = csbi.wAttributes;

    std::lock_guard<std::mutex> lock(g_consoleMutex);

    // 使用\r定位行首
    printf("\r");

    // 时间部分（默认颜色）
    printf("[%02d:%02d:%02d.%03d]", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    // 错误等级部分（指定颜色）
    printf("[");
    SetConsoleTextAttribute(hConsole, (levelColor & 0x0F) | (C_BLACK << 4));
    printf("%s", levelStr);
    SetConsoleTextAttribute(hConsole, originalAttr);
    printf("] ");

    // 消息内容（默认颜色）
    printf("%s", msg);

    // 填充剩余空间（防止残留内容）
    int currentLength = 20 + strlen(levelStr) + strlen(msg); // 基础长度估算
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    if (currentLength < consoleWidth) {
        printf("%*s", consoleWidth - currentLength - 1, "");
    }

    fflush(stdout);
}

void printCr(const char* msg, int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    WORD originalAttr = csbi.wAttributes;

    std::lock_guard<std::mutex> lock(g_consoleMutex);

    // 使用\r定位行首
    printf("\r");

    // 设置颜色输出
    SetConsoleTextAttribute(hConsole, (color & 0x0F) | (C_BLACK << 4));
    printf("%s", msg);

    // 填充剩余空间
    int currentLength = strlen(msg);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    if (currentLength < consoleWidth) {
        printf("%*s", consoleWidth - currentLength - 1, "");
    }

    // 恢复颜色
    SetConsoleTextAttribute(hConsole, originalAttr);
    fflush(stdout);
}


void cls() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    if (!FillConsoleOutputCharacter(hConsole,
        (TCHAR)' ',     
        dwConSize,      
        coordScreen,    
        &cCharsWritten))
    {
        return;
    }


    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    if (!FillConsoleOutputAttribute(hConsole, 
        csbi.wAttributes, 
        dwConSize,      
        coordScreen,
        &cCharsWritten))
    {
        return;
    }


    SetConsoleCursorPosition(hConsole, coordScreen);
}