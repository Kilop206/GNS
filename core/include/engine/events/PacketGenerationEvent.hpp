#pragma once

#include "engine/events/Event.hpp"
#include "network/Packet.hpp"

namespace kns {

class PacketGenerationEvent : public Event {
    public:
        PacketGenerationEvent(
            double timestamp,
            int source,
            int destination,
            int packet_size
        );

        void execute(SimulationEngine& engine) override;

    private:
        int source_;
        int destination_;
        int packet_size_;
    };

}