#include "ecsact/runtime/core.h"

template<typename T>
struct id_generator {
	T last_id = static_cast<T>(-1);

	auto generate() -> T {
		const auto new_id = ++reinterpret_cast<std::underlying_type_t<T>&>(last_id);
		return static_cast<T>(new_id);
	}
};

static auto registry_id_generator = id_generator<ecsact_registry_id>{};
static auto entity_id_generator = id_generator<ecsact_entity_id>{};

auto ecsact_create_registry( //
	const char* registry_name
) -> ecsact_registry_id {
	return registry_id_generator.generate();
}

auto ecsact_destroy_registry( //
	ecsact_registry_id registry
) -> void {
}

auto ecsact_clone_registry( //
	ecsact_registry_id registry,
	const char*        registry_name
) -> ecsact_registry_id {
	return registry_id_generator.generate();
}

auto ecsact_hash_registry( //
	ecsact_registry_id registry
) -> uint64_t {
	return {};
}

auto ecsact_clear_registry( //
	ecsact_registry_id registry
) -> void {
}

auto ecsact_create_entity( //
	ecsact_registry_id registry
) -> ecsact_entity_id {
	return entity_id_generator.generate();
}

auto ecsact_ensure_entity( //
	ecsact_registry_id registry,
	ecsact_entity_id   entity
) -> void {
}

auto ecsact_entity_exists( //
	ecsact_registry_id,
	ecsact_entity_id
) -> bool {
	return true;
}

auto ecsact_destroy_entity( //
	ecsact_registry_id registry_id,
	ecsact_entity_id   entity_id
) -> void {
}

auto ecsact_count_entities( //
	ecsact_registry_id registry
) -> int {
	return 0;
}

auto ecsact_get_entities( //
	ecsact_registry_id registry,
	int                max_entities_count,
	ecsact_entity_id*  out_entities,
	int*               out_entities_count
) -> void {
}

auto ecsact_add_component( //
	ecsact_registry_id  registry_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data
) -> ecsact_add_error {
	return {};
}

auto ecsact_has_component( //
	ecsact_registry_id  registry_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         indexed_field_values
) -> bool {
	return false;
}

auto ecsact_get_component( //
	ecsact_registry_id  registry_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         indexed_field_values
) -> const void* {
	return nullptr;
}

auto ecsact_count_components( //
	ecsact_registry_id registry_id,
	ecsact_entity_id   entity_id
) -> int {
	return 0;
}

auto ecsact_get_components( //
	ecsact_registry_id   registry_id,
	ecsact_entity_id     entity_id,
	int                  max_components_count,
	ecsact_component_id* out_component_ids,
	const void**         out_components_data,
	int*                 out_components_count
) -> void {
}

auto ecsact_each_component( //
	ecsact_registry_id             registry_id,
	ecsact_entity_id               entity_id,
	ecsact_each_component_callback callback,
	void*                          callback_user_data
) -> void {
}

auto ecsact_update_component( //
	ecsact_registry_id  registry_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	const void*         indexed_field_values
) -> ecsact_update_error {
	return {};
}

auto ecsact_remove_component( //
	ecsact_registry_id  registry_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         indexed_field_values
) -> void {
}

auto ecsact_execute_systems( //
	ecsact_registry_id                       registry_id,
	int                                      execution_count,
	const ecsact_execution_options*          execution_options_list,
	const ecsact_execution_events_collector* events_collector
) -> ecsact_execute_systems_error {
	return {};
}

auto ecsact_get_entity_execution_status( //
	ecsact_registry_id    registry_id,
	ecsact_entity_id      entity_id,
	ecsact_system_like_id system_like_id
) -> ecsact_ees {
	return {};
}

auto ecsact_stream( //
	ecsact_registry_id  registry_id,
	ecsact_entity_id    entity,
	ecsact_component_id component_id,
	const void*         component_data,
	const void*         indexed_field_values
) -> ecsact_stream_error {
	return {};
}
