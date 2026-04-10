#include <string>

namespace kns {
    struct RunConfig {
        std::string filename;
        double loss_prob;
        int seed;
    };
}