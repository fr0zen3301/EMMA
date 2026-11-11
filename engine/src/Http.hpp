#pragma once
#include <string>

namespace emma::http {

// Blocking HTTPS GET. Returns false and fills error only when no reply arrived
// (no network, a timeout, a failed secure connection). Any reply, error statuses
// included, comes back in body with its HTTP status, since an error reply
// usually explains itself. Uses WinHTTP on Windows and the system curl
// elsewhere, so EMMA needs no extra networking library
bool get(const std::string& url, std::string& body, long& status, std::string& error);

}
