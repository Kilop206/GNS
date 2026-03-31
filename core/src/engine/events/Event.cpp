#include "engine/events/Event.hpp"
#include "engine/core/SimulationEngine.hpp"

namespace kns {

	// Initialize the static member variable nextId_ to 0.
	std::uint64_t kns::Event::nextId_ = 0;

	// Default implementation of the execute method for the Event class.
	void kns::Event::execute(kns::SimulationEngine& engine) {
	}

	// Constructor for the Event class that initializes the timestamp and assigns a unique ID to each event.
	kns::Event::Event(std::uint64_t timestamp)
		: timestamp_(timestamp)
		, id_(nextId_++)
	{
	}

	// Getter method for the timestamp of the event.
	std::uint64_t kns::Event::getTimestamp() const noexcept {
		return timestamp_;
	}

	// Getter method for the unique ID of the event.
	std::uint64_t kns::Event::getId() const noexcept {
		return id_;
	}

}