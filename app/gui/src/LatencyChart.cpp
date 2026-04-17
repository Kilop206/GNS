#include <algorithm>
#include <numeric>

#include "../include/LatencyChart.hpp"

namespace gui {
    void CircularBuffer::calculateMaximumLatency() {
        maximum_latency = *std::max_element(buffer.begin(), buffer.end());
    }

    void CircularBuffer::calculateMinimumLatency() {
        minimum_latency = *std::min_element(buffer.begin(), buffer.end());
    }

    void CircularBuffer::calculateMediumLatency() {
        float sum = std::accumulate(buffer.begin(), buffer.end(), 0.0f);

        medium_latency = sum / buffer.size();
    }

    void CircularBuffer::addLatencyToBuffer(float latency) {
        buffer.push_back(latency);
    }
}
