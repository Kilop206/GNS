#pragma once

namespace kns {
	struct Link {
		int from;
		int to;

		double delay_ms;
		double bandwidth_mbps;

		double loss_prob;
	};
}