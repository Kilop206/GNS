#include "engine/events/TCPDelayedAckEvent.hpp"

#include "engine/core/SimulationEngine.hpp"
#include "network/Packet.hpp"
#include "network/transport/tcp/TCPSession.hpp"
#include "network/utils/PacketUtils.hpp"

namespace kns
{
    TCPDelayedAckEvent::TCPDelayedAckEvent(
        double timestamp,
        std::uint64_t session_id,
        int receiver_node
    )
        : Event(timestamp),
          session_id_(session_id),
          receiver_node_(receiver_node)
    {
    }

    void TCPDelayedAckEvent::execute(
        SimulationEngine& engine
    )
    {
        if (!engine.hasTCPSession(session_id_)) {
            return;
        }

        auto& session =
            engine.getTCPSession(session_id_);

        TCPConnection* receiver = nullptr;

        if (
            receiver_node_ ==
            session.getSource()
        ) {
            receiver =
                &session.getClientConnection();
        }
        else if (
            receiver_node_ ==
            session.getDestination()
        ) {
            receiver =
                &session.getServerConnection();
        }
        else {
            return;
        }

        if (!receiver->isEstablished()) {
            return;
        }

        if (receiver->getLocalNode() != receiver_node_) {
            return;
        }

        Packet ack(
            receiver->getLocalNode(),
            receiver->getRemoteNode(),
            receiver->getLocalNode(),
            engine.now(),
            0,
            session_id_
        );

        ack.packet_type = PacketType::ACK;
        ack.tcp = receiver->buildAck();
        ack.departure_time = engine.now();

        PacketUtils::sendPacketThroughTopology(
            engine,
            ack
        );
    }
}