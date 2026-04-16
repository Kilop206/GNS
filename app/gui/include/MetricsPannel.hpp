#pragma

#include "../../../core/include/engine/core/Stats.hpp"

namespace interface {
    class MetricsPannel {
        public:
            void render(const kns::Stats& stats);
    };
}