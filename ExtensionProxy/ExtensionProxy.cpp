#include "stdafx.h"
#include "ExtensionProxy.h"

using namespace EP;
//Static variables
const char* ExtensionProxy::VERSION = "0.0.1";
ExtensionProxy* ExtensionProxy::instance = nullptr;
//End of static variables

static inline std::vector<wchar_t> readEntireFile(const std::wstring& path) {
	std::wifstream file(path, std::ios::in | std::ios::ate | std::ios::binary);

	if (file.fail()) {
		char buffer[256];

		strerror_s(buffer, 256, errno);
		std::string errorMessage(buffer);

		throw std::exception(errorMessage.c_str());
	}

	std::vector<wchar_t> data(file.tellg());

	file.seekg(0);

	file.read(data.data(), data.size());

	file.close();

	return data;
}

//Main thread code
DWORD WINAPI mainWorkerThread(LPVOID param) {
	HANDLE notificationHandle = nullptr;
	ExtensionProxy* extensionProxy = reinterpret_cast<ExtensionProxy*>(param);

	std::wstring dllName = extensionProxy->getConfig()[L"extension"];


#ifdef _WIN64
	dllName.append(L"_x64.dll");
#else
	dllName.append(L".dll");
#endif

	std::wstring directoryPath = L"\\\\?\\";
	directoryPath.append(extensionProxy->getConfig()[L"directory"]);


	//Monitor the directory for file changes
	notificationHandle = FindFirstChangeNotificationW(
		directoryPath.c_str(),
		false,
		FILE_NOTIFY_CHANGE_LAST_WRITE
	);

	if (notificationHandle == INVALID_HANDLE_VALUE) {
		WindowsUtils::throwLastWindowsError();
	}

	std::wstring fullFilename = extensionProxy->getConfig()[L"directory"];

	if (!StringUtils::endsWith(fullFilename, L"\\")) {
		fullFilename.append(L"\\");
	}
	fullFilename.append(dllName);


	//Load the library the first time but make sure to load the copy
	extensionProxy->copyFile(fullFilename, fullFilename + L".hot");
	extensionProxy->swapLoadedLibrary(extensionProxy->loadLibrary(fullFilename + L".hot"));

	do {
		NTSTATUS result;
		result = FindNextChangeNotification(notificationHandle);

		if (!NT_SUCCESS(result)) {
			WindowsUtils::throwLastWindowsError();
		}

		if (WaitForSingleObject(notificationHandle, INFINITE) == WAIT_FAILED) {
			WindowsUtils::throwLastWindowsError();
		}
				
		//Wait for the file to be accessible, otherwise we get a "Permission denied" error.
		Sleep(1000);

		//Hash the library file again to see if it changed
		auto newHash = extensionProxy->hashFile(fullFilename);		
		auto loadedLibraryHash = extensionProxy->getCurrentLoadedLibraryHash();

		std::string loadedLibraryText = "";
		for (auto val : loadedLibraryHash) {
			loadedLibraryText.append(" ").append(std::to_string(val));
		}
		Logger::debug("Loaded library hash:");
		Logger::debug(loadedLibraryText);

		std::string newHashText = "";
		for (auto val : newHash) {
			newHashText.append(" ").append(std::to_string(val));
		}
		Logger::debug("New hash:");
		Logger::debug(newHashText);

		bool hashesDiffer = false;

		if (newHash.size() != loadedLibraryHash.size()) {
			hashesDiffer = true;
		} else {
			for (int i = 0; i < newHash.size(); i++) {
				if (newHash[i] != loadedLibraryHash[i]) {
					hashesDiffer = true;
					break;
				}
			}
		}

		if (hashesDiffer) { //Need to swap libraries
			Logger::debug("Swapping libraries");

			extensionProxy->unloadCurrentLibrary();

			std::wstring copyFilename = fullFilename + L".hot";

			extensionProxy->copyFile(fullFilename, copyFilename);

			LibraryData newLibraryData = extensionProxy->loadLibrary(copyFilename);
			extensionProxy->swapLoadedLibrary(newLibraryData);
		}
	} while (extensionProxy->isToRun());

	BCryptDestroyHash(extensionProxy->getCryptoData().hashHandle);
	delete[] extensionProxy->getCryptoData().hashBuffer;
	BCryptCloseAlgorithmProvider(extensionProxy->getCryptoData().algorithmHandle, 0);

	FindCloseChangeNotification(notificationHandle);

	return 0;
}

//-------------------------ExtensionProperty methods implementation-------------------------------------

std::unordered_map<std::wstring, std::wstring>  ExtensionProxy::loadConfigurationFromFile() {
	std::wifstream file("ExtensionProxy.cfg");
	std::wstring line;
	std::unordered_map<std::wstring, std::wstring> config;

	while (std::getline(file, line)) {
		size_t separatorPos = line.find_first_of('=');

		if (separatorPos >= 0) {
			std::wstring key = line.substr(0, separatorPos);
			std::wstring value = line.substr(separatorPos + 1, (line.length() - separatorPos - 1));

			StringUtils::trim(key);
			StringUtils::trim(value);

			config[key] = value;
		}
	}


	return config;
}

LibraryData EP::ExtensionProxy::loadLibrary(const std::wstring& path) {
	//Make a copy of the file and load that, so that the real library isn't blocked and can be replaced
	HMODULE newHandle = LoadLibraryExW(path.c_str(), NULL, 0);

	if (newHandle == NULL) {
		WindowsUtils::throwLastWindowsError();
	}

	RVExtensionVersionFunction versionFunction = reinterpret_cast<RVExtensionVersionFunction>(GetProcAddress(newHandle, "RVExtensionVersion"));

	if (versionFunction == NULL) {
		WindowsUtils::throwLastWindowsError();
	}

	RVExtensionFunction defaultFunction = reinterpret_cast<RVExtensionFunction>(GetProcAddress(newHandle, "RVExtension"));

	if (defaultFunction == NULL) {
		WindowsUtils::throwLastWindowsError();
	}


	RVExtensionArgsFunction argsFunction = reinterpret_cast<RVExtensionArgsFunction>(GetProcAddress(newHandle, "RVExtensionArgs"));

	if (argsFunction == NULL) {
		WindowsUtils::throwLastWindowsError();
	}

	LibraryData newData;
	newData.handle = newHandle;
	newData.hash = this->hashFile(path);

	newData.versionFunction = versionFunction;
	newData.defaultFunction = defaultFunction;
	newData.argsFunction = argsFunction;

	//Don't care if these exist or not
	newData.onLoadFunction = reinterpret_cast<OnLoadFunction>(GetProcAddress(newHandle, "OnLoad")); 
	newData.onUnloadFunction = reinterpret_cast<OnUnloadFunction>(GetProcAddress(newHandle, "OnUnload"));

	return newData;
}

void EP::ExtensionProxy::unloadCurrentLibrary() {
	this->threadMutex.lock();

	if (this->libraryData.handle != NULL) {
		if (this->libraryData.onUnloadFunction != nullptr) {
			this->libraryData.onUnloadFunction();
		}

		if (FreeLibrary(this->libraryData.handle) == 0) {
			WindowsUtils::throwLastWindowsError();
		}
		
		this->libraryData.handle = NULL;
	}

	this->threadMutex.unlock();
}

void EP::ExtensionProxy::copyFile(const std::wstring & from, const std::wstring & to) {
	std::wifstream  src(from, std::ios::binary);
	std::wofstream  dst(to, std::ios::binary);

	if (src.fail()) {
		char buffer[256];

		strerror_s(buffer, 256, errno);
		std::string errorMessage(buffer);

		throw std::exception(errorMessage.c_str());
	}

	if (dst.fail()) {
		char buffer[256];

		strerror_s(buffer, 256, errno);
		std::string errorMessage(buffer);

		throw std::exception(errorMessage.c_str());
	}

	dst << src.rdbuf();
}

void EP::ExtensionProxy::initialize() {
	if (this->initialized) {
		return;
	}

	Logger::createInstance();

	this->config = this->loadConfigurationFromFile();

	Logger::info("Loaded configuration: ");

	if (config.find(L"directory") == config.end()) {
		throw std::exception("Missing \"directory\" configuration entry");
	}

	if (config.find(L"extension") == config.end()) {
		throw std::exception("Missing \"extension\" configuration entry");
	}

	this->createHashAlgorithmInstance();

	this->threadData.handle = CreateThread(
		NULL,
		0,			//Use the default stack size
		&mainWorkerThread,
		this,		//Pass the instance as the parameter
		0,			//No flags (Start immediately)
		&threadData.threadID
	);

	this->initialized = true;
}

void ExtensionProxy::createHashAlgorithmInstance() {
	NTSTATUS result;
	result = BCryptOpenAlgorithmProvider(&this->cryptoData.algorithmHandle, BCRYPT_MD5_ALGORITHM, NULL, BCRYPT_HASH_REUSABLE_FLAG);

	if (!NT_SUCCESS(result)) {
		WindowsUtils::throwLastWindowsError();
	}

	DWORD bytesRequired = 0;
	ULONG bytesWritten = 0;
	result = BCryptGetProperty(this->cryptoData.algorithmHandle, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&bytesRequired), (sizeof(DWORD)), &bytesWritten, 0);

	if (!NT_SUCCESS(result)) {
		WindowsUtils::throwLastWindowsError();
	}

	this->cryptoData.bufferSize = bytesRequired;
	this->cryptoData.hashBuffer = new uint8_t[bytesRequired];

	result = BCryptCreateHash(this->cryptoData.algorithmHandle, &this->cryptoData.hashHandle, this->cryptoData.hashBuffer, this->cryptoData.bufferSize, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);

	if (!NT_SUCCESS(result)) {
		WindowsUtils::throwLastWindowsError();
	}
}

void EP::ExtensionProxy::callRVVersion(char * output, int maxOutputSize) {
	if (!this->initialized) {
		this->initialize();
	}

	if (this->libraryData.handle != NULL) {
		if (this->threadMutex.try_lock()) {
			this->libraryData.versionFunction(output, maxOutputSize);

			this->threadMutex.unlock();
		}
	} else {
		memcpy_s(output, maxOutputSize, VERSION, strlen(VERSION));
	}
}

void EP::ExtensionProxy::callRVExtension(char * output, int maxOutputSize, const char * function) {
	if (this->libraryData.handle != NULL) {
		if (this->threadMutex.try_lock()) {
			this->libraryData.defaultFunction(output, maxOutputSize, function);

			this->threadMutex.unlock();
		}
	}
}

int EP::ExtensionProxy::callRVExtensionArgs(char * output, int outputSize, const char * function, const char * argv[], int argc) {
	if (this->libraryData.handle != NULL) {
		if (this->threadMutex.try_lock()) {
			int retVal = this->libraryData.argsFunction(output, outputSize, function, argv, argc);

			this->threadMutex.unlock();

			return retVal;
		}
	}

	return 0;
}

ExtensionProxy * EP::ExtensionProxy::getInstance() {
	if (ExtensionProxy::instance == nullptr) {
		ExtensionProxy::instance = new ExtensionProxy();
	}

	return ExtensionProxy::instance;
}

std::unordered_map<std::wstring, std::wstring>& EP::ExtensionProxy::getConfig() {
	return this->config;
}

CryptoData& EP::ExtensionProxy::getCryptoData() {
	return this->cryptoData;
}

std::vector<uint8_t> EP::ExtensionProxy::hashFile(const std::wstring & path) {
	NTSTATUS result;
	std::vector<wchar_t> data = readEntireFile(path);

	uint8_t* dataToHash = reinterpret_cast<uint8_t*>(data.data());
	ULONG dataSize = static_cast<ULONG>((sizeof(wchar_t) * data.size()));

	result = BCryptHashData(this->getCryptoData().hashHandle, dataToHash, dataSize, 0);

	if (!NT_SUCCESS(result)) {
		WindowsUtils::throwLastWindowsError();
	}


	DWORD hashSize = 0;
	ULONG bytesWritten = 0;
	result = BCryptGetProperty(this->getCryptoData().hashHandle, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashSize), sizeof(DWORD), &bytesWritten, 0);
	std::vector<uint8_t> hashData(hashSize);

	if (!NT_SUCCESS(result)) {
		WindowsUtils::throwLastWindowsError();
	}

	if (!NT_SUCCESS(BCryptFinishHash(this->getCryptoData().hashHandle, hashData.data(), static_cast<ULONG>(hashData.size()), 0))) {
		WindowsUtils::throwLastWindowsError();
	};

	return hashData;
}

std::vector<uint8_t> EP::ExtensionProxy::getCurrentLoadedLibraryHash() {
	return this->libraryData.hash;
}

void EP::ExtensionProxy::swapLoadedLibrary(LibraryData data) {
	this->threadMutex.lock();

	this->unloadCurrentLibrary();

	this->libraryData = data;

	if (data.onLoadFunction != nullptr) {
		data.onLoadFunction();
	}

	this->threadMutex.unlock();
}

bool EP::ExtensionProxy::isToRun() {
	return this->toRun;
}

void EP::ExtensionProxy::setToRun(bool val) {
	this->toRun = val;
}
