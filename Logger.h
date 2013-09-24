#ifndef LOGGER_H_
#define LOGGER_H_ 

#include <WString.h>
#include <Console.h>
#include <iostream>

#include "Fmt.h"

#define LOG(...) Logger::log(__VA_ARGS__)
#ifdef DEBUG 
	#define DBG(...) Logger::log(__VA_ARGS__)
#else
	#define DBG(...) 
#endif

class Logger {
	public:
		static void log(const char* fmt, ...);
		static void log(String fmt, ...);

	private: 
		static void vlog(const char* fmt, va_list ap);
};

#endif // LOGGER_H_
