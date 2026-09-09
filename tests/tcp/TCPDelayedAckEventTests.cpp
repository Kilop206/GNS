#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "engine/core/SimulationEngine.hpp"
#include "engine/events/PacketReceivedEvent.hpp"
#include "engine/events/TCPDelayedAckEvent.hpp"
#include "network/Link.hpp"
#include "network/Packet.hpp"
#include "network/Topology.hpp"
#include "network/transport/tcp/TCPSession.hpp"

using kns::LinkMode;
using kns::SimulationEngine;
using kns::Packet;
using kns::PacketReceivedEvent;
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
        1,
        0
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
        1,
        1
    );

    REQUIRE_NOTHROW(
        event.execute(engine)
    );
}

TEST_CASE(
    "TCP first data segment schedules delayed ACK",
    "[tcp][delayed-ack][integration]"
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

    auto& server =
        session.getServerConnection();

    const auto sequence =
        server.getExpectedAckNum();

    std::vector<std::uint8_t> payload(
        100,
        0x41
    );

    Packet data(
        0,
        1,
        1,
        engine.now(),
        100,
        session.getSession_id()
    );

    data.packet_type =
        kns::PacketType::DATA;

    data.tcp.seq =
        sequence;

    data.tcp.payload =
        payload;

    PacketReceivedEvent event(
        engine.now(),
        data
    );

    event.execute(engine);

    REQUIRE(
        server.hasDelayedAckPending()
    );

    REQUIRE(
        engine.hasEvents()
    );

    REQUIRE(
        engine.peekNextEventTime() ==
        engine.now() +
        TCPDelayedAckEvent::DEFAULT_DELAY
    );
}

TEST_CASE(
    "TCP second data segment triggers immediate ACK",
    "[tcp][delayed-ack][integration]"
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

    auto& server =
        session.getServerConnection();

    const auto first_sequence =
        server.getExpectedAckNum();

    Packet first(
        0,
        1,
        1,
        engine.now(),
        100,
        session.getSession_id()
    );

    first.packet_type =
        kns::PacketType::DATA;

    first.tcp.seq =
        first_sequence;

    first.tcp.payload.assign(
        100,
        0x41
    );

    PacketReceivedEvent first_event(
        engine.now(),
        first
    );

    first_event.execute(engine);

    REQUIRE(
        server.hasDelayedAckPending()
    );

    const auto second_sequence =
        server.getExpectedAckNum();

    Packet second(
        0,
        1,
        1,
        engine.now(),
        100,
        session.getSession_id()
    );

    second.packet_type =
        kns::PacketType::DATA;

    second.tcp.seq =
        second_sequence;

    second.tcp.payload.assign(
        100,
        0x42
    );

    PacketReceivedEvent second_event(
        engine.now(),
        second
    );

    second_event.execute(engine);

    REQUIRE_FALSE(
        server.hasDelayedAckPending()
    );

    REQUIRE(
        engine.hasEvents()
    );

    REQUIRE(
        engine.peekNextEventTime() >
        engine.now()
    );
}

TEST_CASE(
    "TCP delayed ACK event sends ACK when still pending",
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

    auto& server =
        session.getServerConnection();

    server.markDelayedAckPending();

    const auto acknowledgement =
        server.getExpectedAckNum();

    TCPDelayedAckEvent event(
        engine.now() +
            TCPDelayedAckEvent::DEFAULT_DELAY,
        session.getSession_id(),
        server.getLocalNode(),
        acknowledgement
    );

    event.execute(engine);

    REQUIRE_FALSE(
        server.hasDelayedAckPending()
    );
}

TEST_CASE(
    "TCP obsolete delayed ACK event is ignored after ACK advancement",
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

    auto& server =
        session.getServerConnection();

    server.markDelayedAckPending();

    const auto old_ack =
        server.getExpectedAckNum();

    server.setExpectedAckNum(
        old_ack + 100
    );

    TCPDelayedAckEvent event(
        engine.now() +
            TCPDelayedAckEvent::DEFAULT_DELAY,
        session.getSession_id(),
        server.getLocalNode(),
        old_ack
    );

    event.execute(engine);

    REQUIRE_FALSE(
        server.hasDelayedAckPending()
    );
}