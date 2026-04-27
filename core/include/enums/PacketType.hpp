#pragma once

namespace kns {
    enum class PacketType {
        DATA,
        SYN,
        SYN_ACK,
        ACK,
        FIN
    };
}