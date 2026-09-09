#pragma once

#include <cstdint>

namespace kns
{
    /**
     * @brief Common interface for TCP congestion control algorithms.
     *
     * Congestion state is maintained in bytes.
     *
     * - cwnd: current congestion window;
     * - ssthresh: threshold between slow start and congestion avoidance;
     * - mss: maximum segment size used by the algorithm.
     *
     * Concrete implementations (Tahoe, Reno, NewReno and CUBIC)
     * define how this state evolves over time.
     */
    class CongestionControl
    {
    public:
        virtual ~CongestionControl() = default;

        /**
         * @brief Returns the current congestion window.
         */
        virtual std::uint32_t getCwnd() const noexcept = 0;

        /**
         * @brief Returns the current slow-start threshold.
         */
        virtual std::uint32_t getSsthresh() const noexcept = 0;

        /**
         * @brief Returns the maximum segment size used by the
         * congestion control algorithm.
         */
        virtual std::uint32_t getMss() const noexcept = 0;

        /**
         * @brief Checks whether a new segment may be transmitted.
         *
         * @param bytes_in_flight Number of bytes currently unacknowledged.
         * @param segment_size Size of the new segment in bytes.
         *
         * @return true if the new segment fits within the current cwnd.
         */
        virtual bool canSend(
            std::uint32_t bytes_in_flight,
            std::uint32_t segment_size
        ) const noexcept = 0;

        /**
         * @brief Notifies the algorithm that new bytes were acknowledged.
         *
         * The implementation updates cwnd and/or ssthresh according
         * to the selected congestion control algorithm.
         */
        virtual void onAck(
            std::uint32_t acknowledged_bytes
        ) noexcept = 0;

        /**
         * @brief Notifies the algorithm that packet loss was detected.
         *
         * The loss may have been detected by a timeout or by a
         * mechanism such as fast retransmit.
         */
        virtual void onLoss() noexcept = 0;

        /**
         * @brief Restores the algorithm to its initial state.
         */
        virtual void reset() noexcept = 0;
    };
}