#include "kns/engine/EventQueue.hpp"
#include "kns/engine/Event.hpp" 

#include <stdexcept>

namespace kns {
    bool kns::EventQueue::EventComparator::operator()(const std::unique_ptr<kns::Event>& a,
        const std::unique_ptr<kns::Event>& b) const
    {
        if (a->getTimestamp() != b->getTimestamp()) {
            return a->getTimestamp() > b->getTimestamp();
        }

        return a->getId() > b->getId();
    }

    void kns::EventQueue::schedule(std::unique_ptr<kns::Event> event)
    {
        if (!event) {
            throw std::invalid_argument("Cannot schedule null event");
        }

        event_list_.push(std::move(event));
    }

    std::unique_ptr<kns::Event> kns::EventQueue::next()
    {
        if (event_list_.empty()) {
            return nullptr;
        }

        auto ptr = std::move(
            const_cast<std::unique_ptr<kns::Event>&>(event_list_.top())
        );

        event_list_.pop();

        return ptr;
    }

    bool kns::EventQueue::hasEvents() const noexcept
    {
        return !event_list_.empty();
    }

    std::size_t kns::EventQueue::size() const noexcept
    {
        return event_list_.size();
    }

    void kns::EventQueue::clear()
    {
        while (!event_list_.empty()) {
            event_list_.pop();
        }
    }
}