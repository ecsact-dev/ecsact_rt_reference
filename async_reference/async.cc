#include <optional>
#include <stdarg.h>
#include <span>
#include "ecsact/runtime/async.h"
#include "async_reference/async_reference.hh"
#include "async_reference/callbacks/async_callbacks.hh"
#include "async_reference/request_id_factory/request_id_factory.hh"

using namespace ecsact::async_reference;

// NOTE: These are the singletons for managing the reference library state.
//       This file should be small. Declare a few variables and call a few
//       functions in the C function bodies. Keep the logic minimal.
static auto async_callbacks = detail::async_callbacks{};
static auto request_id_factory = detail::request_id_factory{};
static auto reference = std::optional<detail::async_reference>{};

ecsact_async_request_id ecsact_async_connect(const char* connection_string) {
	auto req_id = request_id_factory.next_id();
	reference.emplace(async_callbacks);
	reference->connect(req_id, connection_string);
	return req_id;
}

void ecsact_async_disconnect() {
	if(reference) {
		reference->disconnect();
		reference.reset();
	}
}

void ecsact_async_flush_events(
	const ecsact_execution_events_collector* execution_evc,
	const ecsact_async_events_collector*     async_evc
) {
	async_callbacks.invoke(async_evc);
	if(reference) {
		reference->invoke_execution_events(execution_evc);
	}
}

ecsact_async_request_id ecsact_async_enqueue_execution_options(
	const ecsact_execution_options options
) {
	auto req_id = request_id_factory.next_id();
	if(!reference) {
		async_callbacks.add(detail::types::async_error{
			.error = ECSACT_ASYNC_ERR_NOT_CONNECTED,
			.request_ids = {req_id},
		});
		return req_id;
	}

	reference->enqueue_execution_options(req_id, options);
	return req_id;
}

int32_t ecsact_async_get_current_tick() {
	if(reference) {
		return reference->get_current_tick();
	}
	return 0;
}

ecsact_async_request_id ecsact_async_stream(
	ecsact_entity_id    entity,
	ecsact_component_id component_id,
	const void*         component_data,
	const void*         indexed_fields
) {
	auto req_id = request_id_factory.next_id();
	// TODO: indexed fields
	if(indexed_fields != nullptr) {
		async_callbacks.add(detail::types::async_error{
			.error = ECSACT_ASYNC_ERR_INTERNAL,
			.request_ids = {req_id},
		});
		return req_id;
	}

	if(!reference) {
		async_callbacks.add(detail::types::async_error{
			.error = ECSACT_ASYNC_ERR_NOT_CONNECTED,
			.request_ids = {req_id},
		});
		return req_id;
	}

	reference->stream( //
		req_id,
		entity,
		component_id,
		component_data,
		indexed_fields
	);
	return req_id;
}
