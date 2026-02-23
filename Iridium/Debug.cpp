#include <Windows.h>
#include <iostream>
#include <Psapi.h>
#include "FunctionCall.h"
#include "Define.h"

void Crash_AccessViolation() {
	int* ptr = NULL;
	*ptr = 42;
}