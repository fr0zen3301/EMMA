#include "Http.hpp"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

namespace {

struct Handle {
    HINTERNET h = nullptr;
    explicit Handle(HINTERNET handle) : h(handle) {}
    ~Handle() {
        if (h) WinHttpCloseHandle(h);
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    explicit operator bool() const { return h != nullptr; }
};

std::string describe(DWORD code) {
    switch (code) {
    case ERROR_WINHTTP_TIMEOUT: return "the request timed out";
    case ERROR_WINHTTP_NAME_NOT_RESOLVED: return "couldn't find the server, are you online?";
    case ERROR_WINHTTP_CANNOT_CONNECT: return "couldn't connect to the server";
    case ERROR_WINHTTP_CONNECTION_ERROR: return "the connection was reset";
    case ERROR_WINHTTP_SECURE_FAILURE: return "the secure connection failed";
    default: return "network error " + std::to_string(code);
    }
}

} // namespace

namespace emma::http {

bool get(const std::string& url, std::string& body, long& status, std::string& error) {
    body.clear();
    status = 0;
    const std::string scheme = "https://";
    if (url.compare(0, scheme.size(), scheme) != 0) {
        error = "only https addresses are supported";
        return false;
    }
    const std::size_t slash = url.find('/', scheme.size());
    const std::string host = url.substr(scheme.size(), slash == std::string::npos ? std::string::npos
                                                                                   : slash - scheme.size());
    const std::string path = slash == std::string::npos ? "/" : url.substr(slash);
    const std::wstring wideHost(host.begin(), host.end());  // the URL is plain ASCII
    const std::wstring widePath(path.begin(), path.end());

#ifdef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
    const DWORD access = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;  // follows the system proxy settings
#else
    const DWORD access = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
#endif
    Handle session(WinHttpOpen(L"EMMA/3.0 (+https://github.com/fr0zen3301/EMMA)", access,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) {
        error = describe(GetLastError());
        return false;
    }
    WinHttpSetTimeouts(session.h, 5000, 5000, 10000, 15000);  // resolve, connect, send, receive (ms)

    Handle connection(WinHttpConnect(session.h, wideHost.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection) {
        error = describe(GetLastError());
        return false;
    }
    Handle request(WinHttpOpenRequest(connection.h, L"GET", widePath.c_str(), nullptr, WINHTTP_NO_REFERER,
                                      WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request) {
        error = describe(GetLastError());
        return false;
    }
    if (!WinHttpSendRequest(request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.h, nullptr)) {
        error = describe(GetLastError());
        return false;
    }

    DWORD code = 0, size = sizeof(code);
    WinHttpQueryHeaders(request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &code, &size, WINHTTP_NO_HEADER_INDEX);

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.h, &available)) {
            error = describe(GetLastError());
            return false;
        }
        if (available == 0) break;
        const std::size_t old = body.size();
        body.resize(old + available);
        DWORD read = 0;
        if (!WinHttpReadData(request.h, body.data() + old, available, &read)) {
            error = describe(GetLastError());
            return false;
        }
        body.resize(old + read);
    }

    // An error status still counts as a reply, body says the error
    status = static_cast<long>(code);
    return true;
}

}

#else  // macOS and Linux: the system curl

#include <array>
#include <cstdio>
#include <cstdlib>

namespace emma::http {

namespace {
// curl writes the HTTP status after the body
const std::string STATUS_MARKER = "\nEMMA_HTTP_STATUS:";
}

bool get(const std::string& url, std::string& body, long& status, std::string& error) {
    body.clear();
    status = 0;
    // EMMA builds its URLs with every quote percent-encoded, so single quotes keep the shell out of it
    if (url.find('\'') != std::string::npos) {
        error = "unsupported character in the address";
        return false;
    }
    // No --fail: that would throw away the body of an error reply, which usually
    // explains the error. -w adds the status line that STATUS_MARKER finds
    const std::string command =
        "curl -sS --max-time 20 -w '\\nEMMA_HTTP_STATUS:%{http_code}\\n' '" + url + "' 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        error = "couldn't start curl";
        return false;
    }
    std::string output;
    std::array<char, 4096> buffer;
    std::size_t n;
    while ((n = std::fread(buffer.data(), 1, buffer.size(), pipe)) > 0) output.append(buffer.data(), n);
    const int exitCode = pclose(pipe);

    // Cut out the status line, leaving the body exactly as sent
    const std::size_t at = output.rfind(STATUS_MARKER);
    if (at != std::string::npos) {
        const std::size_t digits = at + STATUS_MARKER.size();
        const std::size_t end = output.find('\n', digits);
        status = std::strtol(output.substr(digits, end == std::string::npos ? std::string::npos : end - digits).c_str(),
                             nullptr, 10);
        output.erase(at, end == std::string::npos ? std::string::npos : end + 1 - at);
    }

    // No reply at all: curl explains why in a line starting "curl: ("
    if (exitCode != 0 || status == 0) {
        const std::size_t message = output.find("curl: (");
        error = message == std::string::npos ? output : output.substr(message);
        const std::size_t lineEnd = error.find_first_of("\r\n");
        if (lineEnd != std::string::npos) error.erase(lineEnd);
        if (error.empty()) error = "curl failed";
        if (error.size() > 200) error = error.substr(0, 200);
        return false;
    }
    body = std::move(output);
    return true;
}

}

#endif
