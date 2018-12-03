#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <ctime>
#include <array>
#include <iostream>

enum LogLevel {
	DBG = -1,
	INFO = 0,
	WARNING = 1,
	DANGER = 2,
	ERR = 3
};

class Logger {
public:
	~Logger();

	static void log(const LogLevel& level, const std::string& message);
	static void debug(const std::string& message);
	static void info(const std::string& message);
	static void warning(const std::string& message);
	static void danger(const std::string& message);
	static void error(const std::string& message);

	static void createInstance();

	static void setLogToConsoleEnabled(bool value);
	static bool isLogToConsoleEnabled();
private:
	Logger();

	static Logger* instance;

	bool logToConsole = true;

	//Mutex to log the log writes since this can be accessed by multiple threads
	std::mutex mutex = {};
	//File the log will be written to
	std::ofstream* fileOut;
};
