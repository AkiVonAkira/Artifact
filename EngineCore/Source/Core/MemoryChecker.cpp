#include "Artifact/Core/MemoryChecker.h"
#include "Artifact/Core/LogManager.h"
#include <sstream>

#include "Artifact/Core/Macros.h"

namespace Artifact::Core {

    std::mutex MemoryChecker::s_mutex;
    std::unordered_map<void*, MemoryAllocation> MemoryChecker::s_allocations;
    size_t MemoryChecker::s_totalAllocated = 0;
    size_t MemoryChecker::s_totalDeallocated = 0;
    size_t MemoryChecker::s_peakAllocated = 0;
    bool MemoryChecker::s_enabled = false;

    void MemoryChecker::trackAllocation(void* ptr, size_t size, const char* file, int line, const char* func) {
        if (!s_enabled || !ptr) {
            return;
        }

        std::lock_guard<std::mutex> lock(s_mutex);

        MemoryAllocation alloc;
        alloc.address = ptr;
        alloc.size = size;
        alloc.file = file;
        alloc.line = line;
        alloc.function = func;

        s_allocations[ptr] = alloc;
        s_totalAllocated += size;

        if (s_totalAllocated - s_totalDeallocated > s_peakAllocated) {
            s_peakAllocated = s_totalAllocated - s_totalDeallocated;
        }
    }

    void MemoryChecker::trackDeallocation(void* ptr) {
        if (!s_enabled || !ptr) {
            return;
        }

        std::lock_guard<std::mutex> lock(s_mutex);

        auto it = s_allocations.find(ptr);
        if (it != s_allocations.end()) {
            s_totalDeallocated += it->second.size;
            s_allocations.erase(it);
        }
    }

    size_t MemoryChecker::getTotalAllocated() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_totalAllocated - s_totalDeallocated;
    }

    size_t MemoryChecker::getAllocationCount() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_allocations.size();
    }

    void MemoryChecker::checkLeaks() {
        std::lock_guard<std::mutex> lock(s_mutex);

        if (s_allocations.empty()) {
            LOG_INFO("Memory Check: No leaks detected!");
            return;
        }

        LOG_WARN("Memory Check: {} active allocations, {} bytes leaked",
                 s_allocations.size(),
                 s_totalAllocated - s_totalDeallocated);

        size_t count = 0;
        constexpr size_t MAX_LEAKS_TO_PRINT = 10;

        for (const auto& [ptr, alloc] : s_allocations) {
            if (count >= MAX_LEAKS_TO_PRINT) {
                LOG_WARN("  ... and {} more leaks", s_allocations.size() - MAX_LEAKS_TO_PRINT);
                break;
            }

            LOG_WARN("  Leak: {} bytes at {} ({}:{} in {})",
                     alloc.size,
                     ptr,
                     alloc.file ? alloc.file : "unknown",
                     alloc.line,
                     alloc.function ? alloc.function : "unknown");
            ++count;
        }
    }

    std::string MemoryChecker::getMemoryReport() {
        std::lock_guard<std::mutex> lock(s_mutex);

        std::ostringstream oss;
        oss << "=== Memory Report ===\n";
        oss << "Total Allocated:   " << s_totalAllocated << " bytes\n";
        oss << "Total Deallocated: " << s_totalDeallocated << " bytes\n";
        oss << "Current Usage:     " << (s_totalAllocated - s_totalDeallocated) << " bytes\n";
        oss << "Peak Usage:        " << s_peakAllocated << " bytes\n";
        oss << "Active Allocations: " << s_allocations.size() << "\n";

        if (!s_allocations.empty()) {
            oss << "\nActive Allocations:\n";
            size_t count = 0;
            constexpr size_t MAX_TO_PRINT = 20;

            for (const auto& [ptr, alloc] : s_allocations) {
                if (count >= MAX_TO_PRINT) {
                    oss << "  ... and " << (s_allocations.size() - MAX_TO_PRINT) << " more\n";
                    break;
                }

                oss << "  " << alloc.size << " bytes at " << ptr;
                if (alloc.file) {
                    oss << " (" << alloc.file << ":" << alloc.line << ")";
                }
                oss << "\n";
                ++count;
            }
        }

        return oss.str();
    }

    void MemoryChecker::reset() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_allocations.clear();
        s_totalAllocated = 0;
        s_totalDeallocated = 0;
        s_peakAllocated = 0;
    }

    void MemoryChecker::setEnabled(bool enabled) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_enabled = enabled;
    }

    bool MemoryChecker::isEnabled() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_enabled;
    }

} // namespace Artifact::Core

