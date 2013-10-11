#include "Fmt.h"

int const Fmt::MAX_BUFF_SIZE = 512; // we don't allocate more than this amount of mmemory

String Fmt::fmt(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	String s = Fmt::vfmt(fmt, ap);
	va_end(ap);
	return(s);
} 

String Fmt::fmt(const String fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	String s = Fmt::vfmt(fmt.c_str(), ap);
	va_end(ap);
	return(s);	
} 

String Fmt::vfmt(const char *fmt, va_list ap) {
	int size = 32;

	while(1) {
		char buffer[size]; 
		int n = vsnprintf(buffer, size, fmt, ap);
		if( (n > -1 && n < size) || (size == MAX_BUFF_SIZE) ) { // don't let th string grow more that MAX_BUFF_SIZE bytes
			String s = buffer;
			return(s);
		} 

		size *= 2;
	}
}
