#include <catch2/catch_test_macros.hpp>

#include <string>

#include "engine/core/SimulationEngine.hpp"
#include "engine/events/TCPDelayedAckEvent.hpp"
#include "network/Link.hpp"
#include "network/Topology.hpp"
#include "network/transport/tcp/TCPSession.hpp"

using kns::LinkMode;
using kns::SimulationEngine;
using kns::TCPDelayedAckEvent;
using kns::TCPState;
using kns::Topology;

namespace
{
    void establishSession(kns::TCPSession& session)
    {
        auto& client =
            session.getClientConnection();

        auto& server =
            session.getServerConnection();

        REQUIRE(
            client.send_syn()
        );

        const auto client_syn_sequence =
            client.getSeqNum();

        REQUIRE(
            server.receive_syn(
                client_syn_sequence
            )
        );

        const auto syn_ack =
            server.buildSynAck();

        REQUIRE(
            client.receive_syn_ack(
                syn_ack.seq,
                syn_ack.ack
            )
        );

        const auto ack =
            client.buildAck();

        REQUIRE(
            server.receive_ack(
                ack.ack,
                0.0
            )
        );

        REQUIRE(
            client.getTcpState() ==
            TCPState::ESTABLISHED
        );

        REQUIRE(
            server.getTcpState() ==
            TCPState::ESTABLISHED
        );
    }
}

TEST_CASE(
    "TCP delayed ACK event exposes 200 ms default delay",
    "[tcp][delayed-ack]"
)
{
    REQUIRE(
        TCPDelayedAckEvent::DEFAULT_DELAY ==
        0.2
    );
}

TEST_CASE(
    "TCP delayed ACK event identifies itself correctly",
    "[tcp][delayed-ack]"
)
{
    TCPDelayedAckEvent event(
        0.2,
        1,
        1
    );

    REQUIRE(
        event.getName() ==
        std::string("TCPDelayedAckEvent")
    );

    REQUIRE(
        event.getTimestamp() ==
        0.2
    );
}

TEST_CASE(
    "TCP delayed ACK event does nothing for unknown session",
    "[tcp][delayed-ack]"
)
{
    Topology topology(2);

    auto link =
        topology.addLinkPtr(
            0,
            1,
            100.0,
            100.0,
            0.0,
            LinkMode::FULL_DUPLEX
        );

    REQUIRE(link != nullptr);

    SimulationEngine engine(topology);

    TCPDelayedAckEvent event(
        engine.now() +
            TCPDelayedAckEvent::DEFAULT_DELAY,
        999,
        1
    );

    REQUIRE_NOTHROW(
        event.execute(engine)
    );

    REQUIRE(
        engine.now() ==
        0.0
    );
}

TEST_CASE(
    "TCP delayed ACK event executes against established receiver",
    "[tcp][delayed-ack]"
)
{
    Topology topology(2);

    auto link =
        topology.addLinkPtr(
            0,
            1,
            100.0,
            100.0,
            0.0,
            LinkMode::FULL_DUPLEX
        );

    REQUIRE(link != nullptr);

    SimulationEngine engine(topology);

    auto& session =
        engine.createTCPSession(
            0,
            1
        );

    establishSession(session);

    TCPDelayedAckEvent event(
        engine.now() +
            TCPDelayedAckEvent::DEFAULT_DELAY,
        session.getSession_id(),
        1
    );

    REQUIRE_NOTHROW(
        event.execute(engine)
    );
}