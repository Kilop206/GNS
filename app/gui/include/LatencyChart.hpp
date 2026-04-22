 #include <vector>

namespace gui {
    struct CircularBuffer {
        private: 
            float maximum_latency;
            float medium_latency;
            float minimum_latency;
            std::vector<float> buffer;

        public:

            void calculateMaximumLatency();

            void calculateMinimumLatency();

            void calculateMediumLatency();

            void addLatencyToBuffer(float latency);
    };
}