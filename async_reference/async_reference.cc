#include <chrono>
#include <map>
#include "ecsact/runtime/core.h"
#include "async_reference/async_reference.hh"
#include "async_reference/util/util.hh"
#include "async_reference/util/types.hh"

using namespace ecsact::async_reference::detail;

struct parsed_connection_string {
	std::string                        host;
	std::map<std::string, std::string> options;
};

static auto str_subrange(std::string_view str, auto start, auto end) {
	return str.substr(start, end - start);
}

static auto parse_connection_string( //
	std::string_view str
) -> parsed_connection_string {
	auto result = parsed_connection_string{};

	auto options_start = str.find("?");

	if(options_start == std::string::npos) {
		// No options
		result.host = str;
		return result;
	}

	result.host = str.substr(0, options_start);

	auto amp_idx = options_start;

	while(amp_idx != std::string::npos) {
		auto next_amp_idx = str.find("&", amp_idx + 1);

		auto name_str = std::string{};
		auto value_str = std::string{};
		auto eq_idx = str.find("=", amp_idx);

		if(eq_idx >= next_amp_idx) {
			name_str = str_subrange(str, amp_idx + 1, next_amp_idx);
		} else {
			name_str = str_subrange(str, amp_idx + 1, eq_idx);
			value_str = str_subrange(str, eq_idx + 1, next_amp_idx);
		}

		result.options[name_str] = value_str;

		amp_idx = next_amp_idx;
	}

	return result;
}

auto async_reference::start(
	ecsact_async_session_id session_id,
	std::string_view        connect_str
) -> void {
	auto lk = std::unique_lock{execution_m};
	auto result = parse_connection_string(connect_str);

	async_callbacks[session_id].add(ECSACT_ASYNC_SESSION_PENDING);

	if(result.options.contains("delta_time")) {
		auto tick_str = result.options.at("delta_time");

		int tick_int = std::stoi(tick_str);

		delta_time = std::chrono::milliseconds(tick_int);
	} else {
		std::this_thread::yield();
	}

	// The good and bad strings simulate the outcome of connections
	if(result.host != "good") {
		// is_connected = false;

		async_callbacks[session_id].add(types::async_error{
			.error = ECSACT_ASYNC_ERR_PERMISSION_DENIED,
			.request_ids = {ECSACT_INVALID_ID(async_request)},
		});
		async_callbacks[session_id].add(ECSACT_ASYNC_SESSION_STOPPED);
		return;
	}

	if(delta_time.count() < 0) {
		async_callbacks[session_id].add(types::async_error{
			.error = ECSACT_ASYNC_INVALID_CONNECTION_STRING,
			.request_ids = {ECSACT_INVALID_ID(async_request)},
		});
		async_callbacks[session_id].add(ECSACT_ASYNC_SESSION_STOPPED);
		return;
	}

	if(!registry_id) {
		registry_id = ecsact_create_registry("async_reference_impl_reg");
	}

	if(!running) {
		execute_systems();
	}

	sessions.emplace_hint( //
		sessions.end(),
		session_id,
		std::make_shared<session>()
	);
	async_callbacks[session_id].add(ECSACT_ASYNC_SESSION_START);
}

void async_reference::enqueue_execution_options(
	ecsact_async_session_id         session_id,
	ecsact_async_request_id         req_id,
	const ecsact_execution_options& options
) {
	auto lk = std::unique_lock{execution_m};

	if(!sessions.contains(session_id)) {
		return;
	}
	// if(!is_connected) {
	// 	async_callbacks.add(types::async_error{
	// 		.error = ECSACT_ASYNC_ERR_NOT_CONNECTED,
	// 		.request_ids = {req_id},
	// 	});
	// 	async_callbacks.add(types::async_request_complete{
	// 		.request_ids = {req_id},
	// 	});
	// 	return;
	// }

	auto cpp_options = util::c_to_cpp_execution_options(options);

	tick_manager.add_pending_options(types::pending_execution_options{
		.session_id = session_id,
		.request_id = req_id,
		.options = cpp_options,
	});
}

void async_reference::execute_systems() {
	running = true;
	execution_thread = std::thread([this] {
		using namespace std::chrono_literals;
		using clock = std::chrono::high_resolution_clock;
		using nanoseconds = std::chrono::nanoseconds;
		using std::chrono::duration_cast;

		auto execution_duration = nanoseconds{};

		while(running) {
			if(delta_time.count() > 0) {
				const auto sleep_duration = delta_time - execution_duration;
				std::this_thread::sleep_for(sleep_duration);
			}

			auto lk = std::unique_lock{execution_m};
			auto event = tick_manager.validate_pending_options();

			std::visit(
				[&](auto&& event) {
					for(auto&& [session_id, async_callbacks] : async_callbacks) {
						async_callbacks.add(event);
					}
				},
				event
			);

			if(auto err = std::get_if<types::async_error>(&event)) {
				for(auto&& [session_id, async_callbacks] : async_callbacks) {
					async_callbacks.add(types::async_request_complete{
						// NOTE: this is stupid.
						.session_id = session_id,
						.request_ids = err->request_ids,
					});
					async_callbacks.add(ECSACT_ASYNC_SESSION_STOPPED);
				}

				running = false;
				break;
			}

			auto start = clock::now();
			auto cpp_options = tick_manager.move_and_increment_tick();
			auto collector = main_exec_callbacks.get_collector();
			auto c_exec_options = detail::c_execution_options{};

			if(cpp_options) {
				util::cpp_to_c_execution_options(
					c_exec_options,
					*cpp_options,
					*registry_id
				);
			}
			auto options = c_exec_options.c();

			auto systems_error = ecsact_execute_systems( //
				*registry_id,
				1,
				&options,
				collector
			);

			for(auto&& [session_id, session] : sessions) {
				session->exec_callbacks.append(main_exec_callbacks);
			}

			main_exec_callbacks.clear();

			auto end = clock::now();
			execution_duration = duration_cast<nanoseconds>(end - start);

			if(systems_error != ECSACT_EXEC_SYS_OK) {
				running = false;
				for(auto&& [session_id, async_callbacks] : async_callbacks) {
					async_callbacks.add(systems_error);
					async_callbacks.add(ECSACT_ASYNC_SESSION_STOPPED);
				}
				return;
			}
		}
	});
}

void async_reference::stream(
	ecsact_async_session_id session_id,
	ecsact_entity_id        entity,
	ecsact_component_id     component_id,
	const void*             component_data,
	const void*             indexed_fields
) {
	if(registry_id) {
		auto lk = std::unique_lock{execution_m};
		ecsact_stream(
			registry_id.value(),
			entity,
			component_id,
			component_data,
			indexed_fields
		);
	}
}

void async_reference::flush_events(
	ecsact_async_session_id                  session_id,
	const ecsact_execution_events_collector* execution_evc,
	const ecsact_async_events_collector*     async_evc
) {
	if(registry_id) {
		auto lk = std::unique_lock{execution_m};
		auto itr = sessions.find(session_id);
		if(itr != sessions.end()) {
			itr->second->exec_callbacks.invoke(execution_evc, *registry_id);
		}
	}
}

auto async_reference::get_current_tick( //
	ecsact_async_session_id session_id
) -> int32_t {
	auto lk = std::unique_lock{execution_m};
	if(sessions.contains(session_id)) {
		return tick_manager.get_current_tick();
	}
	return 0;
}

auto async_reference::stop(ecsact_async_session_id session_id) -> void {
	auto itr = sessions.find(session_id);
	if(itr != sessions.end()) {
		auto lk = std::unique_lock{execution_m};
		async_callbacks[session_id].add(ECSACT_ASYNC_SESSION_STOPPED);
		sessions.erase(itr);
	}
	if(sessions.empty()) {
		running = false;
		for(auto&& [other_session_id, async_callbacks] : async_callbacks) {
			if(other_session_id != session_id) {
				async_callbacks.add(ECSACT_ASYNC_SESSION_STOPPED);
			}
		}
		if(execution_thread.joinable()) {
			execution_thread.join();
		}
	}
}

auto async_reference::session_count() -> int32_t {
	auto lk = std::unique_lock{execution_m};
	return static_cast<int32_t>(sessions.size());
}
