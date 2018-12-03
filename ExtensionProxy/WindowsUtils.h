#pragma once

//https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror

namespace WindowsUtils {
	static std::string getWindowsErrorMessage(DWORD errorCode) {
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);

		//Free the buffer.
		LocalFree(messageBuffer);

		return message;
	}

	static void throwLastWindowsError() {
		DWORD lastError = GetLastError();
		std::string errorMessage = WindowsUtils::getWindowsErrorMessage(lastError);

		throw std::exception(errorMessage.c_str());
	}
}