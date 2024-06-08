#include <stdarg.h>
#include <stdio.h>

#include "error.h"

// Macro defining the procedure to get the variable args and produce the output string.
// @param __fmt The name of the variable before the '...' in the calling function.
#define SET_ERROR_FROM_VAARGS(__fmt) \
	va_list __args{}; \
	char __buffer[2048]; \
	va_start(__args, __fmt); \
	vsprintf_s(__buffer, __fmt, __args); \
	va_end(__args); \
	msg = std::string(__buffer);

Error::Error(const std::string& _msg)
	: Error(-1, _msg)
{}

Error::Error(const char* fmt, ...)
	: code(-1)
{
	SET_ERROR_FROM_VAARGS(fmt);
}

Error::Error(int _code, const std::string& _msg)
	: code(_code)
	, msg(_msg)
{}

Error::Error(int _code, const char* fmt, ...)
	: code(_code)
{
	SET_ERROR_FROM_VAARGS(fmt);
}

Error::operator bool() {
	return code != 0;
}

void Error::print() {
	printf("Error: %s\n", msg.c_str());
}
