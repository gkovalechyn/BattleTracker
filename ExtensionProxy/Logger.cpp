#include "stdafx.h"
#include "Logger.h"

Logger* Logger::instance;

inline std::string formatDate(const std::string format, const tm* time) {
	char buffer[64];
	memset(buffer, 0, 64);
	strftime(buffer, sizeof(buffer), format.c_str(), time);
	return std::string(buffer);
}

inline std::tm localtime_xp(const std::time_t timer) {
	std::tm bt{};
#if defined(__unix__)
	localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
	localtime_s(&bt, &timer);
#else
	static std::mutex mtx;
	std::lock_guard<std::mutex> lock(mtx);
	bt = *std::localtime(&timer);
#endif
	return bt;
}

Logger::Logger() {
	time_t time = std::time(nullptr);
	std::string filename = formatDate("Log-%Y-%m-%d.log", &localtime_xp(time));
	this->fileOut = new std::ofstream(filename, std::ofstream::trunc);

	if (this->fileOut == nullptr) {
		throw std::runtime_error("Could not log file for writing.");
	}
}


Logger::~Logger() {
	this->fileOut->close();
}

void Logger::log(const LogLevel& level, const std::string& message) {
	time_t time = std::time(nullptr);
	std::string timeString = formatDate("[%Y-%m-%d %I:%M:%S]", &localtime_xp(time));
	std::string logLevelString;

	switch (level) {
		case LogLevel::DBG:
			logLevelString = "DEBUG";
			break;
		case LogLevel::INFO:
			logLevelString = "INFO";
			break;
		case LogLevel::WARNING:
			logLevelString = "WARNING";
			break;
		case LogLevel::DANGER:
			logLevelString = "DANGER";
			break;
		case LogLevel::ERR:
			logLevelString = "ERROR";
			break;
		default:
			logLevelString = "ERROR_LOG_LEVEL";
			break;
	}

	Logger::instance->mutex.lock();//Lock

	*(Logger::instance->fileOut) << timeString << " " << logLevelString << " - " << message << std::endl;

	if (Logger::instance->logToConsole) {
		std::cout << timeString << " " << logLevelString << " - " << message << std::endl;
	}

	Logger::instance->mutex.unlock();//Unlock
}

void Logger::debug(const std::string & message) {
	Logger::log(LogLevel::DBG, message);
}

void Logger::info(const std::string& message) {
	Logger::log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
	Logger::log(LogLevel::WARNING, message);
}

void Logger::danger(const std::string& message) {
	Logger::log(LogLevel::DANGER, message);
}

void Logger::error(const std::string& message) {
	Logger::log(LogLevel::ERR, message);
}

void Logger::createInstance() {
	//No need to lock this operation since it will be done once before all the other threads are created
	if (Logger::instance == nullptr) {
		Logger::instance = new Logger();
	}
}

void Logger::setLogToConsoleEnabled(bool value) {
	Logger::instance->logToConsole = value;
}

bool Logger::isLogToConsoleEnabled() {
	return Logger::instance->logToConsole;
}
