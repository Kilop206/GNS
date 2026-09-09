#include <catch2/catch_test_macros.hpp>

#include "network/transport/tcp/TCPConnection.hpp"
#include "network/transport/tcp/congestion/CubicCongestionControl.hpp"
#include "network/transport/tcp/congestion/NewRenoCongestionControl.hpp"
#include "network/transport/tcp/congestion/RenoCongestionControl.hpp"
#include "network/transport/tcp/congestion/TahoeCongestionControl.hpp"

using namespace kns;

TEST_CASE(
    "TCPConnection creates Reno congestion control by default",
    "[tcp][congestion][connection]"
)
{
    TCPConnection connection(
        TCPState::CLOSED,
        0,
        0,
        1,
        2
    );

    REQUIRE(
        connection.getCongestionControlType() ==
        CongestionControlType::RENO
    );

    REQUIRE(
        dynamic_cast<RenoCongestionControl*>(
            &connection.getCongestionControl()
        ) != nullptr
    );
}

TEST_CASE(
    "TCPConnection can select Tahoe congestion control",
    "[tcp][congestion][connection]"
)
{
    TCPConnection connection(
        TCPState::CLOSED,
        0,
        0,
        1,
        2,
        CongestionControlType::TAHOE,
        1000
    );

    REQUIRE(
        connection.getCongestionControlType() ==
        CongestionControlType::TAHOE
    );

    REQUIRE(
        dynamic_cast<TahoeCongestionControl*>(
            &connection.getCongestionControl()
        ) != nullptr
    );

    REQUIRE(
        connection.getCongestionControl().getMss() == 1000
    );
}

TEST_CASE(
    "TCPConnection can select Reno congestion control",
    "[tcp][congestion][connection]"
)
{
    TCPConnection connection(
        TCPState::CLOSED,
        0,
        0,
        1,
        2,
        CongestionControlType::RENO,
        1000
    );

    REQUIRE(
        dynamic_cast<RenoCongestionControl*>(
            &connection.getCongestionControl()
        ) != nullptr
    );
}

TEST_CASE(
    "TCPConnection can select NewReno congestion control",
    "[tcp][congestion][connection]"
)
{
    TCPConnection connection(
        TCPState::CLOSED,
        0,
        0,
        1,
        2,
        CongestionControlType::NEW_RENO,
        1000
    );

    REQUIRE(
        connection.getCongestionControlType() ==
        CongestionControlType::NEW_RENO
    );

    REQUIRE(
        dynamic_cast<NewRenoCongestionControl*>(
            &connection.getCongestionControl()
        ) != nullptr
    );
}

TEST_CASE(
    "TCPConnection can select CUBIC congestion control",
    "[tcp][congestion][connection]"
)
{
    TCPConnection connection(
        TCPState::CLOSED,
        0,
        0,
        1,
        2,
        CongestionControlType::CUBIC,
        1000
    );

    REQUIRE(
        connection.getCongestionControlType() ==
        CongestionControlType::CUBIC
    );

    REQUIRE(
        dynamic_cast<CubicCongestionControl*>(
            &connection.getCongestionControl()
        ) != nullptr
    );
}

TEST_CASE(
    "TCPConnection exposes congestion window state",
    "[tcp][congestion][connection]"
)
{
    TCPConnection connection(
        TCPState::CLOSED,
        0,
        0,
        1,
        2,
        CongestionControlType::RENO,
        1000
    );

    REQUIRE(
        connection.getCongestionControl().getCwnd() == 1000
    );

    REQUIRE(
        connection.getCongestionControl().getMss() == 1000
    );
}

TEST_CASE(
    "TCPConnection timeout notifies congestion control",
    "[tcp][congestion][connection][timeout]"
)
{
    TCPConnection connection(
        TCPState::ESTABLISHED,
        1000,
        2000,
        0,
        1,
        CongestionControlType::RENO,
        1000,
        4000
    );

    TCPSegment first;
    first.seq = 1000;
    first.payload.assign(
        1000,
        0x41
    );

    REQUIRE(
        connection.queueSentSegment(
            first,
            10.0
        )
    );

    REQUIRE(
        connection.receive_ack(
            2000,
            11.0
        )
    );

    REQUIRE(
        connection.getCongestionControl().getCwnd() == 2000
    );

    TCPSegment second;
    second.seq = 2000;
    second.payload.assign(
        1000,
        0x42
    );

    REQUIRE(
        connection.queueSentSegment(
            second,
            12.0
        )
    );

    REQUIRE(
        connection.getSendBufferSize() == 1
    );

    REQUIRE(
        connection.getCongestionControl().getCwnd() == 2000
    );

    connection.onSendTimeout();

    REQUIRE(
        connection.getCongestionControl().getSsthresh() == 2000
    );

    REQUIRE(
        connection.getCongestionControl().getCwnd() == 1000
    );

    REQUIRE(
        connection.getCurrentRTO() == 6.0
    );
}