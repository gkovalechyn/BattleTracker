#pragma once
#include "stdafx.h"

typedef void(*RVExtensionVersionFunction)(char* output, int maxOutputSize);
typedef void(*RVExtensionFunction)(char* output, int maxOutputSize, const char* function);
typedef int(*RVExtensionArgsFunction)(char* output, int maxOutputSize, const char* function, const char* argv[], int argc);
typedef void(*OnLoadFunction)();
typedef void(*OnUnloadFunction)();

//https://docs.microsoft.com/en-us/windows/desktop/seccng/creating-a-hash-with-cng
#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)

namespace EP {
	typedef struct ThreadData {
		HANDLE handle = INVALID_HANDLE_VALUE;
		DWORD threadID = 0;
	} ThreadData;

	typedef struct LibraryData {
		HMODULE handle = NULL;

		std::vector<uint8_t> hash;

		RVExtensionVersionFunction versionFunction = nullptr;
		RVExtensionFunction defaultFunction = nullptr;
		RVExtensionArgsFunction argsFunction = nullptr;
		OnLoadFunction onLoadFunction = nullptr;
		OnUnloadFunction onUnloadFunction = nullptr;
	} LibraryData;

	typedef struct CryptoData {
		BCRYPT_ALG_HANDLE algorithmHandle = INVALID_HANDLE_VALUE;
		BCRYPT_HASH_HANDLE hashHandle = INVALID_HANDLE_VALUE;

		UCHAR* hashBuffer = nullptr;
		ULONG bufferSize = 0;
	} CryptoData;

	class ExtensionProxy {
	public:
		static const char* VERSION;

		void callRVVersion(char* output, int maxOutputSize);
		void callRVExtension(char *output, int maxOutputSize, const char *function);
		int callRVExtensionArgs(char *output, int outputSize, const char *function, const char* argv[], int argc);

		static ExtensionProxy* getInstance();
		std::unordered_map<std::wstring, std::wstring>& getConfig();
		CryptoData& getCryptoData();

		std::vector<uint8_t> hashFile(const std::wstring& path);
		std::vector<uint8_t> getCurrentLoadedLibraryHash();

		void swapLoadedLibrary(LibraryData data);
		LibraryData loadLibrary(const std::wstring& path);
		void unloadCurrentLibrary();

		void copyFile(const std::wstring& from, const std::wstring& to);

		bool isToRun();
		void setToRun(bool val);
	private:
		static ExtensionProxy* instance;

		CryptoData cryptoData;
		LibraryData libraryData;
		ThreadData threadData;

		std::recursive_mutex threadMutex;

		std::unordered_map<std::wstring, std::wstring> config;

		bool initialized = false;
		bool toRun = true;

		void initialize();
		void createHashAlgorithmInstance();
		std::unordered_map<std::wstring, std::wstring> loadConfigurationFromFile();
	};
}