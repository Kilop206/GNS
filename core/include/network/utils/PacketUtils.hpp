#pragma once

#include "engine/core/SimulationEngine.hpp"
#include "network/Packet.hpp"

namespace kns {

    class PacketUtils {
    public:
        static bool sendPacketThroughTopology(
            SimulationEngine& engine,
            const Packet& pkt
        );

        static bool releasePacketThroughTopology(
            SimulationEngine& engine,
            const Packet& pkt
        );

        /// Send a TCP RST from `from` to `to`, acknowledging `remote_seq`.
        static bool sendReset(
            SimulationEngine& engine,
            int from,
            int to,
            std::uint32_t remote_seq,
            std::uint64_t session_id = 0
        );
    };
}