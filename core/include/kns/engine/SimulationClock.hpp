#pragma once

namespace kns {
	class SimulationClock {
	private:
		double current_time_;
	public:
		explicit SimulationClock();
		void tick(double delta_time_);
		double now() const;
		void setTime(double new_current_time_);
	};
}