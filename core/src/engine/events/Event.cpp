#include "engine/events/Event.hpp"
#include "engine/core/SimulationEngine.hpp"

namespace kns {
	std::uint64_t kns::Event::nextId_ = 0;

	void kns::Event::execute(kns::SimulationEngine& engine) {
	}

	kns::Event::Event(std::uint64_t timestamp)
		: timestamp_(timestamp)
		, id_(nextId_++)
	{
	}

	std::uint64_t kns::Event::getTimestamp() const noexcept {
		return timestamp_;
	}

	std::uint64_t kns::Event::getId() const noexcept {
		return id_;
	}

}