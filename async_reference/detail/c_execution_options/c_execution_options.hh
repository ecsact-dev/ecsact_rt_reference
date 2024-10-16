#pragma once

#include <vector>

#include "ecsact/runtime/common.h"

namespace ecsact::async_reference::detail {

template<typename T>
struct data_info {
	T id;

	std::vector<std::byte> data;
};

struct c_execution_options {
	/**
	 * Holds the data lifetime used to reconstruct components and actions
	 * Do NOT use this, it's an implementation detail
	 */
	std::vector<data_info<ecsact_action_id>>    actions_info;
	std::vector<data_info<ecsact_component_id>> adds_info;
	std::vector<ecsact_entity_id>               adds_entities;
	std::vector<data_info<ecsact_component_id>> updates_info;
	std::vector<ecsact_entity_id>               updates_entities;
	std::vector<ecsact_component_id>            remove_ids;
	std::vector<ecsact_entity_id>               removes_entities;
	std::vector<ecsact_entity_id>               destroy_entities;

	std::vector<ecsact_placeholder_entity_id> create_entity_placeholder_ids;
	std::vector<std::vector<data_info<ecsact_component_id>>>
		create_entities_components;

	ecsact_execution_options c();

private:
	ecsact_execution_options options;

	std::vector<ecsact_action>    actions;
	std::vector<ecsact_component> adds;
	std::vector<ecsact_component> updates;

	std::vector<ecsact_component*>             create_entity_components_data;
	std::vector<std::vector<ecsact_component>> create_entity_components;
	std::vector<int32_t>                       create_entity_components_length;
};

} // namespace ecsact::async_reference::detail
