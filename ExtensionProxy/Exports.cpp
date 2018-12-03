#include "stdafx.h"
#include "Exports.h"

//Default RVExtension exports
void __stdcall RVExtensionVersion(char *output, int outputSize) {
	EP::ExtensionProxy::getInstance()->callRVVersion(output, outputSize);
}

void __stdcall RVExtension(char *output, int outputSize, const char *function) {
	EP::ExtensionProxy::getInstance()->callRVExtension(output, outputSize, function);
}

int __stdcall RVExtensionArgs(char *output, int outputSize, const char *function, const char **argv, int argc) {
	return EP::ExtensionProxy::getInstance()->callRVExtensionArgs(output, outputSize, function, argv, argc);
}