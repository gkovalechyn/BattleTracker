#pragma once
#include "ExtensionProxy.h"

extern "C" {
	__declspec(dllexport) void __stdcall RVExtensionVersion(char *output, int outputSize);

	__declspec(dllexport) void __stdcall RVExtension(char *output, int outputSize, const char *function);

	__declspec(dllexport) int __stdcall RVExtensionArgs(char *output, int outputSize, const char *function, const char **argv, int argc);
}