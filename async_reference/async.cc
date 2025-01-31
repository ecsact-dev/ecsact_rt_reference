#include <optional>
#include <stdarg.h>
#include <span>
#include <map>
#include "ecsact/runtime/async.h"
#include "async_reference/async_reference.hh"
#include "async_reference/callbacks/async_callbacks.hh"
#include "async_reference/request_id_factory/request_id_factory.hh"

using namespace ecsact::async_reference;

// NOTE: These are the singletons for managing the reference library state.
//       This file should be small. Declare a few variables and call a few
//       functions in the C function bodies. Keep the logic minimal.
static auto async_callbacks =
	std::map<ecsact_async_session_id, detail::async_callbacks>{};
static auto request_id_factory = detail::request_id_factory{};
static auto last_session_id = ecsact_async_session_id{};
static auto reference = std::optional<detail::async_reference>{};

static auto generate_next_session_id() -> ecsact_async_session_id {
	return static_cast<ecsact_async_session_id>(
		++reinterpret_cast<int32_t&>(last_session_id)
	);
}

auto ecsact_async_start( //
	const void* option_data,
	int32_t     option_data_size
) -> ecsact_async_session_id {
	if(!reference) {
		reference.emplace(async_callbacks);
	}

	auto session_id = generate_next_session_id();
	auto connect_str = std::string_view{};
	if(option_data && option_data_size > 0) {
		connect_str = std::string_view{
			static_cast<const char*>(option_data),
			static_cast<size_t>(option_data_size)
		};
	}

	reference->start(session_id, connect_str);

	return session_id;
}

auto ecsact_async_stop(ecsact_async_session_id session_id) -> void {
	if(!reference) {
		return;
	}

	reference->stop(session_id);

	if(reference->session_count() == 0) {
		reference = std::nullopt;
	}
}

auto ecsact_async_flush_events(
	ecsact_async_session_id                  session_id,
	const ecsact_execution_events_collector* execution_evc,
	const ecsact_async_events_collector*     async_evc
) -> void {
	if(!reference) {
		async_callbacks[session_id].invoke(session_id, async_evc);
		return;
	}

	reference->flush_events(session_id, execution_evc, async_evc);
	async_callbacks[session_id].invoke(session_id, async_evc);
}

auto ecsact_async_enqueue_execution_options(
	ecsact_async_session_id        session_id,
	const ecsact_execution_options options
) -> ecsact_async_request_id {
	auto req_id = request_id_factory.next_id();
	if(!reference) {
		async_callbacks[session_id].add(detail::types::async_error{
			.error = ECSACT_ASYNC_ERR_NOT_CONNECTED,
			.request_ids = {req_id},
		});
		return req_id;
	}

	reference->enqueue_execution_options(session_id, req_id, options);

	return req_id;
}

auto ecsact_async_get_current_tick( //
	ecsact_async_session_id session_id
) -> int32_t {
	if(!reference) {
		return 0;
	}
	return reference->get_current_tick(session_id);
}

auto ecsact_async_stream(
	ecsact_async_session_id session_id,
	ecsact_entity_id        entity,
	ecsact_component_id     component_id,
	const void*             component_data,
	const void*             indexed_fields
) -> void {
	if(indexed_fields != nullptr) {
		async_callbacks[session_id].add(detail::types::async_error{
			.error = ECSACT_ASYNC_ERR_INTERNAL,
			.request_ids = {ECSACT_INVALID_ID(async_request)},
		});
		return;
	}

	if(!reference) {
		async_callbacks[session_id].add(detail::types::async_error{
			.error = ECSACT_ASYNC_ERR_NOT_CONNECTED,
			.request_ids = {ECSACT_INVALID_ID(async_request)},
		});
		return;
	}

	reference->stream( //
		session_id,
		entity,
		component_id,
		component_data,
		indexed_fields
	);
}
