#include <cstdint>

namespace kns {
    class Packet {
    public:
        const int source;
        const int destination;
        int current_node;
        std::uint64_t creation_time;
    };
}