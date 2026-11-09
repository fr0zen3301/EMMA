#pragma once
#include <string>

// Julian days and calendar dates (UTC)
namespace emma::calendar {

double julianDayNow();
double startOfDay(double julianDay);
std::string date(double julianDay);
std::string localDateTime(double julianDay);

}
