#include "cachesim/CacheLine.hpp"

// CacheLine is fully defined in the header (small, no out-of-line logic
// needed yet). This .cpp file exists so the class has a single
// translation unit anchor point - keeps the build graph consistent if
// out-of-line methods are added later (e.g. serialization for a
// visualization dashboard).

namespace cachesim {
} // namespace cachesim
