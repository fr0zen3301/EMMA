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

namespace http {

bool get(const std::string& url, std::string& body, std::string& error) {
    body.clear();
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

    DWORD status = 0, size = sizeof(status);
    WinHttpQueryHeaders(request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);

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

    if (status != 200) {
        error = "the server answered HTTP " + std::to_string(status);
        return false;
    }
    return true;
}

}

#else  // macOS and Linux: the system curl

#include <array>
#include <cstdio>

namespace http {

bool get(const std::string& url, std::string& body, std::string& error) {
    body.clear();
    // EMMA builds its URLs with every quote percent-encoded, so single quotes keep the shell out of it
    if (url.find('\'') != std::string::npos) {
        error = "unsupported character in the address";
        return false;
    }
    const std::string command = "curl -sS --fail --max-time 20 '" + url + "' 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        error = "couldn't start curl";
        return false;
    }
    std::array<char, 4096> buffer;
    std::size_t n;
    while ((n = std::fread(buffer.data(), 1, buffer.size(), pipe)) > 0) body.append(buffer.data(), n);

    if (pclose(pipe) != 0) {
        error = body.empty() ? "curl failed" : body.substr(0, 200);
        while (!error.empty() && (error.back() == '\n' || error.back() == '\r')) error.pop_back();
        return false;
    }
    return true;
}

}

#endif
