#include "../include/MetricsPannel.hpp"

namespace interface {
    void MetricsPannel::render(const kns::Stats& stats) {

        float avg_latency = 0.0f;
        if (stats.packets_delivered > 0) {
            avg_latency = stats.total_latency / stats.packets_delivered;
        }

        ImGui::Text(
            "Packets sent: %d\nPackets Delivered: %d\nPackets Lost: %d\nAverage Latency: %.2f",
            stats.packets_sent,
            stats.packets_delivered,
            stats.packets_lost,
            avg_latency
        );
    }
}