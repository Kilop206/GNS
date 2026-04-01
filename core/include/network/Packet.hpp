#include <cstdint>

namespace kns {
    class Packet {
    public:
        int source;
        int destination;
        int current_node;
        std::uint64_t creation_time;
    };
}