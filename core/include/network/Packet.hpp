#include <cstdint>

namespace kns {
    class Packet {
    public:
        const int source;
        const int destination;
        int current_node;
        std::uint64_t creation_time;
        std::size_t packet_size_bytes;
        int hop_count = 0;

        Packet(int source,
               int destination,
               int current_node,
               std::uint64_t creation_time,
               int packet_size_bytes)
            : source(source),
              destination(destination),
              current_node(current_node),
              creation_time(creation_time),
              packet_size_bytes(packet_size_bytes) {}
    };
}