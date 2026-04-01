#include <cstdint>
#include "network/Packet.hpp"
#include "events/Event.hpp"

namespace kns {

    struct PacketArrivalEvent : public Event {
        public:
            kns::Packet packet;
            std::uint64_t timestamp;

            PacketArrivalEvent(std::uint64_t timestamp, const kns::Packet& packet, std::uint64_t arrival_time);
    };

}