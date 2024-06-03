#pragma once
#include <string>

class Error {
public:
	Error() = default;

	/// @{
	/// Variants that set the error code to a non-zero value.
	explicit Error(const std::string& msg);
	explicit Error(const char* fmt, ...);
	/// @}

	/// Create an error with only a message. The code is set to non-zero.
	Error(int code, const std::string& msg);
	/// Create an error with a printf-style format. The maximum message length is 2048 bytes, including 0 at the end.
	Error(int code, const char* fmt, ...);

	/// Return true if there is an error.
	operator bool();

	/// Print the message.
	void print();

private:
	int code = 0;
	std::string msg;
};
