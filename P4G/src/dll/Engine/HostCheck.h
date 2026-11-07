#pragma once
// Verifies the host before anything touches it. Fails closed: an unknown build keeps the
// trainer dormant, and a failed probe disables only the feature group that depends on it.

#include <string>

namespace HostCheck {

struct Result {
    bool supported = false;    // right executable and build
    std::string reason;        // why not, for the log and the UI
    int probesPassed = 0;
    int probesTotal = 0;
};

const Result& Run();           // runs once, then cached
const Result& Get();

// True when every probe in the group passed (and the build is supported).
bool GroupOk(const char* group);

// Human-readable list of failed probes, for the Advanced page.
std::string FailedProbes();

}  // namespace HostCheck
