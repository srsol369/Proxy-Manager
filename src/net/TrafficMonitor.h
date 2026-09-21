#pragma once

#include <cstdint>

namespace npm {

struct TrafficStats {
    uint64_t bytesIn = 0;
    uint64_t bytesOut = 0;
    double kbpsIn = 0.0;
    double kbpsOut = 0.0;
};

class TrafficMonitor {
public:
    void Tick();
    const TrafficStats& Stats() const { return stats_; }

private:
    TrafficStats stats_{};
    uint64_t prevIn_ = 0;
    uint64_t prevOut_ = 0;
    bool primed_ = false;
    uint64_t lastTick_ = 0;
};

}  // namespace npm
