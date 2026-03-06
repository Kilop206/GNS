#include "core/EventQueue.h"

#include <stdexcept>

bool EventQueue::EventComparator::operator()(
    const std::unique_ptr<core::Event>& a,
    const std::unique_ptr<core::Event>& b) const
{
    if (a->getTimestamp() != b->getTimestamp()) {
        return a->getTimestamp() > b->getTimestamp();
    }

    return a->getId() > b->getId();
}

void EventQueue::schedule(std::unique_ptr<core::Event> event)
{
    if (!event) {
        throw std::invalid_argument("Cannot schedule null event");
    }

    event_list_.push(std::move(event));
}

std::unique_ptr<core::Event> EventQueue::next()
{
    if (event_list_.empty()) {
        return nullptr;
    }

    auto ptr = std::move(
        const_cast<std::unique_ptr<core::Event>&>(event_list_.top())
    );

    event_list_.pop();

    return ptr;
}

bool EventQueue::hasEvents() const noexcept
{
    return !event_list_.empty();
}

std::size_t EventQueue::size() const noexcept
{
    return event_list_.size();
}

void EventQueue::clear()
{
    while (!event_list_.empty()) {
        event_list_.pop();
    }
}