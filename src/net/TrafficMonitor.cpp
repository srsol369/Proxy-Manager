#include "net/TrafficMonitor.h"

#include <windows.h>
#include <iphlpapi.h>

namespace npm {

void TrafficMonitor::Tick() {
    MIB_IFTABLE* table = nullptr;
    DWORD size = 0;
    if (GetIfTable(nullptr, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER) {
        return;
    }
    table = static_cast<MIB_IFTABLE*>(malloc(size));
    if (!table) {
        return;
    }

    uint64_t in = 0;
    uint64_t out = 0;
    if (GetIfTable(table, &size, FALSE) == NO_ERROR) {
        for (DWORD i = 0; i < table->dwNumEntries; ++i) {
            const auto& row = table->table[i];
            if (row.dwType == IF_TYPE_SOFTWARE_LOOPBACK) {
                continue;
            }
            in += row.dwInOctets;
            out += row.dwOutOctets;
        }
    }
    free(table);

    const ULONGLONG now = GetTickCount64();
    if (primed_) {
        const double seconds = static_cast<double>(now - lastTick_) / 1000.0;
        if (seconds > 0.05) {
            const double din = static_cast<double>(in >= prevIn_ ? in - prevIn_ : 0);
            const double dout = static_cast<double>(out >= prevOut_ ? out - prevOut_ : 0);
            stats_.kbpsIn = (din * 8.0 / 1000.0) / seconds;
            stats_.kbpsOut = (dout * 8.0 / 1000.0) / seconds;
        }
    }

    stats_.bytesIn = in;
    stats_.bytesOut = out;
    prevIn_ = in;
    prevOut_ = out;
    lastTick_ = now;
    primed_ = true;
}

}  // namespace npm
