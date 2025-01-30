#pragma once

#include <mutex>

#include "../util/types.hh"

namespace ecsact::async_reference::detail {

class async_callbacks {
public:
	auto invoke(
		ecsact_async_session_id              session_id,
		const ecsact_async_events_collector* async_events
	) -> void;
	void add(const types::async_requests type);
	void add_many(const std::vector<types::async_requests>& types);

private:
	std::vector<types::async_requests> requests;

	std::mutex async_m;
};

} // namespace ecsact::async_reference::detail
