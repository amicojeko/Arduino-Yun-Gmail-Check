#ifndef FMT_H_
#define FMT_H_ 

#include <stdarg.h>
#include <stdio.h>
#include <WString.h>
#include <Arduino.h>

#define FMT(...) Fmt::fmt(__VA_ARGS__)

class Fmt {
	friend class Logger;

	public:
		static String fmt(const char* fmt, ...);
		static String fmt(const String fmt, ...);

	private: 
		static int const MAX_BUFF_SIZE;
		static String vfmt(const char *fmt, va_list ap);
};

#endif // FMT_H_
