#pragma once
// The in-game calendar: day 0 is Friday, April 1st 2011; the year ends March 31st 2012.

#include <cstdint>

namespace Calendar {

struct Date {
    int month, day, weekday;  // weekday 0 = Sunday
};

Date FromIndex(int dayIndex);
int ToIndex(int month, int day);         // -1 if not in the game year
const char* WeekdayShort(int weekday);   // "Sun"
const char* TimeName(int timeOfDay);     // "After School"
bool IsSunday(int dayIndex);

}  // namespace Calendar
