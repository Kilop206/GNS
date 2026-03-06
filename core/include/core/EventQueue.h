#pragma once

#include <memory>
#include <queue>
#include <vector>
#include "Event.h"

class EventQueue {
public:
    void schedule(std::unique_ptr<core::Event> event);
    std::unique_ptr<core::Event> next();

    bool hasEvents() const noexcept;
    std::size_t size() const noexcept;
    void clear();

private:
    struct EventComparator {
        bool operator()(const std::unique_ptr<core::Event>& a,
            const std::unique_ptr<core::Event>& b) const;
    };

    std::priority_queue<
        std::unique_ptr<core::Event>,
        std::vector<std::unique_ptr<core::Event>>,
        EventComparator
    > event_list_;
};