#include "Features/CaseFile.h"

#include "Data/Cases.h"

namespace CaseFile {

bool Solved(const Game::Snapshot& s, int i) {
    const auto& c = Data::kCases[i];
    if (s.day > c.lastDay) return true;  // the game continued past the deadline
    return c.rescueFlag >= 0 && s.Flag(c.rescueFlag);
}

Status Current(const Game::Snapshot& s) {
    Status st;
    if (!s.valid) return st;
    for (int i = 0; i < Data::kCaseCount; ++i) {
        const auto& c = Data::kCases[i];
        if (Solved(s, i)) continue;
        st.index = i;
        st.rescueKnown = c.rescueFlag >= 0;
        st.verified = c.flagVerified;
        st.daysLeft = c.lastDay - s.day;
        st.todayUsable = s.time <= 4;  // up to and including After School
        if (s.day < c.accessDay) st.state = State::Upcoming;
        else if (s.day == c.lastDay) st.state = State::LastDay;
        else st.state = State::Active;
        return st;
    }
    return st;
}

}  // namespace CaseFile
