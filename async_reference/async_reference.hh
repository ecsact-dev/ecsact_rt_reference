#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <optional>
#include <span>
#include "tick_manager/tick_manager.hh"
#include "callbacks/execution_callbacks.hh"
#include "callbacks/async_callbacks.hh"

namespace ecsact::async_reference::detail {
class async_reference {
public:
	inline async_reference(async_callbacks& async_callbacks)
		: async_callbacks(async_callbacks) {
	}

	inline ~async_reference() {
	}

	void enqueue_execution_options(
		ecsact_async_request_id         req_id,
		const ecsact_execution_options& options
	);

	void stream(
		ecsact_async_request_id req_id,
		ecsact_entity_id        entity,
		ecsact_component_id     component_id,
		const void*             component_data,
		const void*             indexed_fields
	);

	void execute_systems();

	void invoke_execution_events(
		const ecsact_execution_events_collector* execution_evc
	);

	int32_t get_current_tick();

	void connect(ecsact_async_request_id req_id, const char* connection_string);

	void disconnect();

private:
	std::atomic_int32_t _last_request_id = 0;

	std::optional<ecsact_registry_id> registry_id;

	tick_manager             tick_manager;
	execution_callbacks      exec_callbacks;
	detail::async_callbacks& async_callbacks;

	std::thread execution_thread;
	std::mutex  execution_m;

	std::atomic_bool is_connected = false;

	std::chrono::milliseconds delta_time = {};
};
}; // namespace ecsact::async_reference::detail
