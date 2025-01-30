#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <optional>
#include <string_view>
#include <memory>
#include <span>
#include <unordered_map>
#include "tick_manager/tick_manager.hh"
#include "callbacks/execution_callbacks.hh"
#include "callbacks/async_callbacks.hh"

namespace ecsact::async_reference::detail {
class async_reference {
public:
	inline async_reference() {
	}

	inline ~async_reference() {
	}

	auto enqueue_execution_options(
		ecsact_async_session_id         session_id,
		ecsact_async_request_id         req_id,
		const ecsact_execution_options& options
	) -> void;

	auto stream(
		ecsact_async_session_id session_id,
		ecsact_entity_id        entity,
		ecsact_component_id     component_id,
		const void*             component_data,
		const void*             indexed_fields
	) -> void;

	auto flush_events(
		ecsact_async_session_id                  session_id,
		const ecsact_execution_events_collector* execution_evc,
		const ecsact_async_events_collector*     async_evc
	) -> void;

	auto get_current_tick(ecsact_async_session_id session_id) -> int32_t;

	auto start(
		ecsact_async_session_id session_id,
		std::string_view        connection_string
	) -> void;

	auto stop(ecsact_async_session_id session_id) -> void;

	auto session_count() -> int32_t;

private:
	struct session {
		execution_callbacks exec_callbacks;
	};

	using sessions_t =
		std::unordered_map<ecsact_async_session_id, std::shared_ptr<session>>;
	sessions_t sessions;

	std::atomic_int32_t               _last_request_id = 0;
	std::optional<ecsact_registry_id> registry_id;
	tick_manager                      tick_manager;

	execution_callbacks main_exec_callbacks;
	std::thread         execution_thread;
	std::mutex          execution_m;

	/**
	 * Are the systems running on the execution thread?
	 */
	std::atomic_bool running;

	std::chrono::milliseconds delta_time = {};

	auto execute_systems() -> void;
};
}; // namespace ecsact::async_reference::detail
