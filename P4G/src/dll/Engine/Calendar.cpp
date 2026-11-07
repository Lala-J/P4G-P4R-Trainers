#include "Engine/Calendar.h"

namespace Calendar {

namespace {
// April 2011 .. March 2012 (2012 is a leap year: February has 29 days)
constexpr int kMonth[12] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 2, 3};
constexpr int kDays[12] = {30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29, 31};
constexpr int kFirstWeekday = 5;  // 2011-04-01 was a Friday
}  // namespace

Date FromIndex(int idx) {
    if (idx < 0) idx = 0;
    int weekday = (kFirstWeekday + idx) % 7;
    for (int m = 0; m < 12; ++m) {
        if (idx < kDays[m]) return {kMonth[m], idx + 1, weekday};
        idx -= kDays[m];
    }
    return {3, 31, weekday};
}

int ToIndex(int month, int day) {
    int idx = 0;
    for (int m = 0; m < 12; ++m) {
        if (kMonth[m] == month) return (day >= 1 && day <= kDays[m]) ? idx + day - 1 : -1;
        idx += kDays[m];
    }
    return -1;
}

const char* WeekdayShort(int w) {
    static const char* n[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return n[(w % 7 + 7) % 7];
}

const char* TimeName(int t) {
    static const char* n[] = {"Early Morning", "Morning", "Lunchtime", "Afternoon",
                              "After School", "Evening", "Night"};
    return (t >= 0 && t < 7) ? n[t] : "?";
}

bool IsSunday(int idx) { return FromIndex(idx).weekday == 0; }

}  // namespace Calendar
