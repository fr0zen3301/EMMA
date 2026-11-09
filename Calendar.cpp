#include "Calendar.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>

namespace calendar {

double julianDayNow() {
    // The system clock counts from the Unix epoch, 1970-01-01 00:00 UTC = JD 2440587.5
    const double seconds =
        std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    return 2440587.5 + seconds / 86400.0;
}

double startOfDay(double jd) {
    // Julian days start at noon, so midnight falls on the .5
    return std::floor(jd - 0.5) + 0.5;
}

std::string date(double jd) {
    const double z = std::floor(jd + 0.5);
    const double f = jd + 0.5 - z;
    double a = z;
    if (z >= 2299161.0) {
        const double alpha = std::floor((z - 1867216.25) / 36524.25);
        a = z + 1.0 + alpha - std::floor(alpha / 4.0);
    }
    const double b = a + 1524.0;
    const double c = std::floor((b - 122.1) / 365.25);
    const double d = std::floor(365.25 * c);
    const double e = std::floor((b - d) / 30.6001);
    const int day = static_cast<int>(b - d - std::floor(30.6001 * e) + f);
    const int month = static_cast<int>(e < 14.0 ? e - 1.0 : e - 13.0);
    const int year = static_cast<int>(month > 2 ? c - 4716.0 : c - 4715.0);

    char text[16];
    std::snprintf(text, sizeof(text), "%04d-%02d-%02d", year, month, day);
    return text;
}

std::string localDateTime(double jd) {
    const std::time_t t = static_cast<std::time_t>(std::floor((jd - 2440587.5) * 86400.0));
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &local);
    return text;
}

}
