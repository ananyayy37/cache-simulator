#pragma once

#include <cstdint>

namespace cachesim {

// One cache line (one "way" within one set). Deliberately minimal - it
// only knows about itself, never about which set or cache it lives in.
// CacheSet is responsible for indexing into a collection of these.
class CacheLine {
public:
    CacheLine() = default;

    bool isValid() const { return valid_; }
    bool isDirty() const { return dirty_; }
    std::uint64_t tag() const { return tag_; }

    // Installs new data into this line (used on a miss, after a victim
    // has been chosen). Always marks valid; dirty state must be set
    // explicitly afterward by the caller based on write policy.
    void install(std::uint64_t newTag) {
        tag_ = newTag;
        valid_ = true;
        dirty_ = false;
    }

    void setDirty(bool d) { dirty_ = d; }

    // Returns this line to its initial, empty state.
    void invalidate() {
        valid_ = false;
        dirty_ = false;
        tag_ = 0;
    }

private:
    bool valid_ = false;
    bool dirty_ = false;
    std::uint64_t tag_ = 0;
};

} // namespace cachesim
