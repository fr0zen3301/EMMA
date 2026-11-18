#pragma once
#include <string>

namespace emma::http {

// Downloads a web page over HTTPS and waits for it. Returns false only if no reply came
// back at all. Uses WinHTTP on Windows and curl elsewhere
bool get(const std::string& url, std::string& body, long& status, std::string& error);

}
