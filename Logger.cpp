#include "Logger.h"

void Logger::log(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Logger::vlog(fmt, args);
	va_end(args);
}

void Logger::log(String fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Logger::vlog(fmt.c_str(), args);
	va_end(args);
}

void Logger::vlog(const char* fmt, va_list ap) {
	String s = Fmt::vfmt(fmt, ap);
	if(Console) {
		Console.println(s);
	}
	if(Serial) {
		Serial.println(s);
		Serial.flush();
	}
}