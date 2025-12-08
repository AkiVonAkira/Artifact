#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <mutex>

namespace Artifact::Core {

    /// Memory allocation tracking structure
    struct MemoryAllocation {
        void* address{nullptr};
        size_t size{0};
        const char* file{nullptr};
        int line{0};
        const char* function{nullptr};
    };

    /// Memory checking and debugging utilities
    class MemoryChecker {
    public:
        /// Track a memory allocation
        static void trackAllocation(void* ptr, size_t size, const char* file, int line, const char* func);

        /// Track a memory deallocation
        static void trackDeallocation(void* ptr);

        /// Get total allocated memory
        [[nodiscard]] static size_t getTotalAllocated();

        /// Get number of active allocations
        [[nodiscard]] static size_t getAllocationCount();

        /// Check for memory leaks and print report
        static void checkLeaks();

        /// Get detailed memory report as string
        [[nodiscard]] static std::string getMemoryReport();

        /// Clear all tracking data
        static void reset();

        /// Enable/disable memory tracking
        static void setEnabled(bool enabled);

        /// Check if tracking is enabled
        [[nodiscard]] static bool isEnabled();

    private:
        static std::mutex s_mutex;
        static std::unordered_map<void*, MemoryAllocation> s_allocations;
        static size_t s_totalAllocated;
        static size_t s_totalDeallocated;
        static size_t s_peakAllocated;
        static bool s_enabled;
    };

    /// Helper macros for tracked memory allocation (optional, for debugging)
    #ifdef ARTIFACT_TRACK_MEMORY
        #define ARTIFACT_NEW(type, ...) \
            (Artifact::Core::MemoryChecker::trackAllocation( \
                new type(__VA_ARGS__), \
                sizeof(type), \
                __FILE__, \
                __LINE__, \
                __FUNCTION__))

        #define ARTIFACT_DELETE(ptr) \
            do { \
                if (ptr) { \
                    Artifact::Core::MemoryChecker::trackDeallocation(ptr); \
                    delete ptr; \
                    ptr = nullptr; \
                } \
            } while(0)
    #else
        #define ARTIFACT_NEW(type, ...) new type(__VA_ARGS__)
        #define ARTIFACT_DELETE(ptr) delete ptr
    #endif

} // namespace Artifact::Core

