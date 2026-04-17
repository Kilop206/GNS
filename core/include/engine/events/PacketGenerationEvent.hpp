#pragma once

#include "engine/events/Event.hpp"

namespace kns {

class PacketGenerationEvent : public Event {
    public:
        PacketGenerationEvent(
            double timestamp,
            int source,
            int destination
        );

        void execute(SimulationEngine& engine) override;

    private:
        int source_;
        int destination_;
    };

}