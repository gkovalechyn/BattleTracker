#pragma once
//https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
#include <algorithm> 
#include <cctype>
#include <locale>

namespace StringUtils {
	// trim from start (in place)
	static inline void trimLeft(std::wstring &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(ch);
		}));
	}

	// trim from end (in place)
	static inline void trimRight(std::wstring &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	// trim from both ends (in place)
	static inline void trim(std::wstring &s) {
		trimLeft(s);
		trimRight(s);
	}

	// trim from start (copying)
	static inline std::wstring trimLeftCopy(std::wstring s) {
		trimLeft(s);
		return s;
	}

	// trim from end (copying)
	static inline std::wstring trimRightCopy(std::wstring s) {
		trimRight(s);
		return s;
	}

	// trim from both ends (copying)
	static inline std::wstring trimCopy(std::wstring s) {
		trim(s);
		return s;
	}

	static inline bool endsWith(const std::wstring &str, const std::wstring &suffix) {
		return str.size() >= suffix.size() &&
			str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
	}

};