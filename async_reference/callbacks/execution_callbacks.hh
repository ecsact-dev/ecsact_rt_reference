#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>

#include "../util/types.hh"

namespace ecsact::async_reference::detail {
class execution_callbacks {
public:
	execution_callbacks();

	auto invoke(
		const ecsact_execution_events_collector* execution_events,
		ecsact_registry_id                       registry_id
	) -> void;

	auto get_collector() -> ecsact_execution_events_collector*;

	auto append(const execution_callbacks& other) -> void;

	auto clear() -> void;

private:
	ecsact_execution_events_collector collector;

	// NOTE: This is very inefficient
	std::unordered_multimap<ecsact_entity_id, types::cpp_execution_component>
		comp_cache;

	std::vector<types::callback_info> init_callbacks_info;
	std::vector<types::callback_info> update_callbacks_info;
	std::vector<types::callback_info> remove_callbacks_info;

	std::vector<types::entity_callback_info> create_entity_callbacks_info;
	std::vector<types::entity_callback_info> destroy_entity_callbacks_info;

	auto has_callbacks() const -> bool;

	auto add_component_cache(
		ecsact_entity_id    entity,
		ecsact_component_id component_id,
		const void*         component_data
	) -> void;

	auto get_component_cache(
		ecsact_entity_id    entity,
		ecsact_component_id component_id
	) -> const void*;

	auto merge_cache(const execution_callbacks& other) -> void;

	static void init_callback(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	);

	static void update_callback(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	);

	static void remove_callback(
		ecsact_event        event,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data,
		void*               callback_user_data
	);

	static void entity_created_callback(
		ecsact_event                 event,
		ecsact_entity_id             entity_id,
		ecsact_placeholder_entity_id placeholder_entity_id,
		void*                        callback_user_data
	);

	static void entity_destroyed_callback(
		ecsact_event                 event,
		ecsact_entity_id             entity_id,
		ecsact_placeholder_entity_id placeholder_entity_id,
		void*                        callback_user_data
	);
};
} // namespace ecsact::async_reference::detail
