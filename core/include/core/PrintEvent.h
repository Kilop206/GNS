#pragma once

#include "core/Event.h"
#include <string>

namespace core {
    class PrintEvent : public core::Event {
    private:
        int time;
        std::string message;

    public:
        PrintEvent(int time, const std::string& message);

        std::uint64_t getTimestamp() const noexcept;

        void execute(SimulationEngine& engine) override;
    };
}