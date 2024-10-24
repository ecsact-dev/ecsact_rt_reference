#include "async_reference/detail/c_execution_options/c_execution_options.hh"

using namespace ecsact::async_reference::detail;

ecsact_execution_options c_execution_options::c() {
	actions.reserve(actions_info.size());
	actions.clear();
	adds.reserve(adds_info.size());
	adds.clear();
	updates.reserve(updates_info.size());
	updates.clear();

	for(auto& action_info : actions_info) {
		ecsact_action action;
		action.action_data = action_info.data.data();
		action.action_id = action_info.id;

		actions.push_back(action);
	}

	for(auto& add_info : adds_info) {
		auto component = ecsact_component{};

		component.component_id = add_info.id;
		component.component_data = add_info.data.data();

		adds.push_back(component);
	}

	for(auto& update_info : updates_info) {
		auto component = ecsact_component{};

		component.component_id = update_info.id;
		component.component_data = update_info.data.data();

		updates.push_back(component);
	}

	for(auto& create_info : create_entities_components) {
		create_entity_components_length.push_back(create_info.size());
		auto& entity_components = create_entity_components.emplace_back();
		entity_components.reserve(create_info.size());
		for(auto& component_info : create_info) {
			auto component = ecsact_component{};
			component.component_data = component_info.data.data();
			component.component_id = component_info.id;
			entity_components.push_back(component);
		}

		create_entity_components_data.push_back(entity_components.data());
	}

	options.actions = actions.data();

	options.actions_length = actions.size();

	options.add_components = adds.data();
	options.add_components_entities = adds_entities.data();
	options.add_components_length = adds.size();

	options.update_components = updates.data();
	options.update_components_entities = updates_entities.data();
	options.update_components_length = updates.size();

	options.remove_components = remove_ids.data();
	options.remove_components_entities = removes_entities.data();
	options.remove_components_length = remove_ids.size();

	options.create_entities = create_entity_placeholder_ids.data();
	options.create_entities_components = create_entity_components_data.data();
	options.create_entities_length = create_entities_components.size();
	options.create_entities_components_length =
		create_entity_components_length.data();

	options.destroy_entities = destroy_entities.data();
	options.destroy_entities_length = destroy_entities.size();

	return options;
}
