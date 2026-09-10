#include <catch2/catch_test_macros.hpp>

#include "engine/core/SimulationEngine.hpp"
#include "network/Topology.hpp"
#include "network/Packet.hpp"
#include "network/utils/PacketUtils.hpp"
#include "network/transport/tcp/TCPListener.hpp"
#include "network/transport/tcp/TCPSession.hpp"

using kns::LinkMode;
using kns::Packet;
using kns::PacketType;
using kns::PacketUtils;
using kns::SimulationEngine;
using kns::TCPFlag;
using kns::TCPListener;
using kns::TCPState;
using kns::TCPSession;
using kns::Topology;

TEST_CASE(
    "TCPListener accepts an incoming connection in LISTEN state",
    "[tcp][listener][passive-open]"
)
{
    Topology topology(2);

    topology.addLink(
        0,
        1,
        10.0,
        10.0,
        0.0,
        LinkMode::FULL_DUPLEX
    );

    SimulationEngine engine(topology);

    TCPListener& listener =
        engine.startTCPListen(1);

    REQUIRE(listener.isListening());
    REQUIRE(listener.getNodeId() == 1);
    REQUIRE(listener.getActiveConnections() == 0);

    const std::uint64_t session_id =
        engine.acceptOnListener(1, 0, 1000);

    REQUIRE(
        session_id != TCPListener::INVALID_SESSION_ID
    );

    REQUIRE(listener.getActiveConnections() == 1);
    REQUIRE(engine.hasTCPSession(session_id));

    TCPSession& session =
        engine.getTCPSession(session_id);

    auto& server =
        session.getServerConnection();

    REQUIRE(server.getLocalNode() == 1);
    REQUIRE(server.getRemoteNode() == 0);
    REQUIRE(server.getTcpState() == TCPState::LISTEN);

    REQUIRE(
        server.receive_syn(1000)
    );

    REQUIRE(
        server.getTcpState() == TCPState::SYN_RECEIVED
    );

    REQUIRE(
        server.getExpectedAckNum() == 1001
    );
}

TEST_CASE(
    "TCPListener accepts SYN through PacketReceivedEvent",
    "[tcp][listener][passive-open][integration]"
)
{
    Topology topology(2);

    topology.addLink(
        0,
        1,
        10.0,
        10.0,
        0.0,
        LinkMode::FULL_DUPLEX
    );

    SimulationEngine engine(topology);

    engine.setGlobalPacketSize(1000);

    TCPListener& listener =
        engine.startTCPListen(1);

    REQUIRE(listener.isListening());

    Packet syn(
        0,
        1,
        0,
        engine.now(),
        engine.getGlobalPacketSize(),
        0
    );

    syn.tcp.seq = 1000;
    syn.tcp.flags = TCPFlag::SYN;
    syn.packet_type = PacketType::SYN;

    REQUIRE(
        PacketUtils::sendPacketThroughTopology(
            engine,
            syn
        )
    );

    REQUIRE(listener.getActiveConnections() == 0);

    REQUIRE(engine.processEvent());

    REQUIRE(listener.getActiveConnections() == 1);
    REQUIRE(engine.getTCPSessions().size() == 1);

    const auto& sessions =
        engine.getTCPSessions();

    const std::uint64_t session_id =
        sessions.begin()->first;

    const auto& session =
        engine.getTCPSession(session_id);

    REQUIRE(
        session.getClientConnection().getLocalNode() == 0
    );

    REQUIRE(
        session.getClientConnection().getRemoteNode() == 1
    );

    REQUIRE(
        session.getServerConnection().getLocalNode() == 1
    );

    REQUIRE(
        session.getServerConnection().getRemoteNode() == 0
    );

    REQUIRE(
        session.getServerConnection().getTcpState() ==
        TCPState::SYN_RECEIVED
    );
}

TEST_CASE(
    "TCPListener completes passive TCP handshake",
    "[tcp][listener][passive-open][handshake]"
)
{
    Topology topology(2);

    topology.addLink(
        0,
        1,
        10.0,
        10.0,
        0.0,
        LinkMode::FULL_DUPLEX
    );

    SimulationEngine engine(topology);

    engine.setGlobalPacketSize(1000);

    auto& listener =
        engine.startTCPListen(1);

    REQUIRE(listener.isListening());

    Packet syn(
        0,
        1,
        0,
        engine.now(),
        engine.getGlobalPacketSize(),
        0
    );

    syn.tcp.seq = 1000;
    syn.tcp.flags = TCPFlag::SYN;
    syn.packet_type = PacketType::SYN;

    REQUIRE(
        PacketUtils::sendPacketThroughTopology(
            engine,
            syn
        )
    );

    /*
     * Process the incoming SYN.
     *
     * This creates the session and transitions:
     *
     * server: LISTEN -> SYN_RECEIVED
     * client: CLOSED -> SYN_SENT
     */
    REQUIRE(engine.processEvent());

    REQUIRE(engine.getTCPSessions().size() == 1);

    const auto session_id =
        engine.getTCPSessions().begin()->first;

    auto& session =
        engine.getTCPSession(session_id);

    REQUIRE(
        session.getClientConnection().getTcpState() ==
        TCPState::SYN_SENT
    );

    REQUIRE(
        session.getServerConnection().getTcpState() ==
        TCPState::SYN_RECEIVED
    );

    /*
     * The listener created the server-side SYN-ACK and
     * PacketReceivedEvent scheduled it on the network.
     *
     * Process the SYN-ACK arrival at the client.
     */
    REQUIRE(engine.processEvent());

    REQUIRE(
        session.getClientConnection().getTcpState() ==
        TCPState::ESTABLISHED
    );

    REQUIRE(
        session.getServerConnection().getTcpState() ==
        TCPState::SYN_RECEIVED
    );

    /*
     * The client generated an ACK in response to the SYN-ACK.
     * Process that ACK at the server.
     */
    REQUIRE(engine.processEvent());

    REQUIRE(
        session.getClientConnection().getTcpState() ==
        TCPState::ESTABLISHED
    );

    REQUIRE(
        session.getServerConnection().getTcpState() ==
        TCPState::ESTABLISHED
    );
}

TEST_CASE(
    "TCPListener accepts multiple independent TCP connections",
    "[tcp][listener][backlog][multiple-connections]"
)
{
    Topology topology(3);

    topology.addLink(
        0,
        2,
        10.0,
        10.0,
        0.0,
        LinkMode::FULL_DUPLEX
    );

    topology.addLink(
        1,
        2,
        10.0,
        10.0,
        0.0,
        LinkMode::FULL_DUPLEX
    );

    SimulationEngine engine(topology);

    TCPListener& listener =
        engine.startTCPListen(2, 2);

    const std::uint64_t first_session =
        engine.acceptOnListener(2, 0, 1000);

    const std::uint64_t second_session =
        engine.acceptOnListener(2, 1, 2000);

    REQUIRE(
        first_session != TCPListener::INVALID_SESSION_ID
    );

    REQUIRE(
        second_session != TCPListener::INVALID_SESSION_ID
    );

    REQUIRE(first_session != second_session);

    REQUIRE(listener.getActiveConnections() == 2);
    REQUIRE(engine.getTCPSessions().size() == 2);

    auto& first =
        engine.getTCPSession(first_session);

    auto& second =
        engine.getTCPSession(second_session);

    REQUIRE(
        first.getClientConnection().getLocalNode() == 0
    );

    REQUIRE(
        first.getClientConnection().getRemoteNode() == 2
    );

    REQUIRE(
        first.getServerConnection().getLocalNode() == 2
    );

    REQUIRE(
        first.getServerConnection().getRemoteNode() == 0
    );

    REQUIRE(
        second.getClientConnection().getLocalNode() == 1
    );

    REQUIRE(
        second.getClientConnection().getRemoteNode() == 2
    );

    REQUIRE(
        second.getServerConnection().getLocalNode() == 2
    );

    REQUIRE(
        second.getServerConnection().getRemoteNode() == 1
    );

    REQUIRE(
        first.getServerConnection().getTcpState() ==
        TCPState::LISTEN
    );

    REQUIRE(
        second.getServerConnection().getTcpState() ==
        TCPState::LISTEN
    );

    REQUIRE(
        first.getClientConnection().getTcpState() ==
        TCPState::SYN_SENT
    );

    REQUIRE(
        second.getClientConnection().getTcpState() ==
        TCPState::SYN_SENT
    );
}