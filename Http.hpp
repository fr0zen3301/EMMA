#pragma once
#include <string>

namespace http {

// Blocking HTTPS GET. Returns false and fills error on a network failure or a
// status other than 200. Uses WinHTTP on Windows and the system curl elsewhere,
// so EMMA needs no extra networking library.
bool get(const std::string& url, std::string& body, std::string& error);

} // namespace http
