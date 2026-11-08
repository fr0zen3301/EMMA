#pragma once
#include <string>

// Julian days and calendar dates (UTC)
namespace calendar {

double julianDayNow();                // from the system clock
double startOfDay(double julianDay);  // 00:00 UTC on the same day
std::string date(double julianDay);   // "2026-11-09"

}
