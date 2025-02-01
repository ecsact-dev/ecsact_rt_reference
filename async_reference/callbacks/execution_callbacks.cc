#include "async_reference/callbacks/execution_callbacks.hh"

#include <ranges>
#include <algorithm>
#include "ecsact/runtime/serialize.hh"

using namespace ecsact::async_reference::detail;

execution_callbacks::execution_callbacks() {
	collector.init_callback = &execution_callbacks::init_callback;
	collector.update_callback = &execution_callbacks::update_callback;
	collector.remove_callback = &execution_callbacks::remove_callback;
	collector.entity_created_callback =
		&execution_callbacks::entity_created_callback;
	collector.entity_destroyed_callback =
		&execution_callbacks::entity_destroyed_callback;
	collector.init_callback_user_data = this;
	collector.update_callback_user_data = this;
	collector.remove_callback_user_data = this;
	collector.entity_created_callback_user_data = this;
	collector.entity_destroyed_callback_user_data = this;
}

ecsact_execution_events_collector* execution_callbacks::get_collector() {
	return &collector;
}

auto execution_callbacks::clear() -> void {
	init_callbacks_info.clear();
	update_callbacks_info.clear();
	remove_callbacks_info.clear();
	create_entity_callbacks_info.clear();
	destroy_entity_callbacks_info.clear();
	comp_cache.clear();
}

auto execution_callbacks::append(const execution_callbacks& other) -> void {
	init_callbacks_info.insert(
		init_callbacks_info.end(),
		other.init_callbacks_info.begin(),
		other.init_callbacks_info.end()
	);
	update_callbacks_info.insert(
		update_callbacks_info.end(),
		other.update_callbacks_info.begin(),
		other.update_callbacks_info.end()
	);
	remove_callbacks_info.insert(
		remove_callbacks_info.end(),
		other.remove_callbacks_info.begin(),
		other.remove_callbacks_info.end()
	);
	create_entity_callbacks_info.insert(
		create_entity_callbacks_info.end(),
		other.create_entity_callbacks_info.begin(),
		other.create_entity_callbacks_info.end()
	);
	destroy_entity_callbacks_info.insert(
		destroy_entity_callbacks_info.end(),
		other.destroy_entity_callbacks_info.begin(),
		other.destroy_entity_callbacks_info.end()
	);

	merge_cache(other);
}

auto execution_callbacks::invoke(
	const ecsact_execution_events_collector* execution_events,
	ecsact_registry_id                       registry_id
) -> void {
	if(!has_callbacks()) {
		return;
	}

	if(execution_events == nullptr) {
		clear();
		return;
	}

	std::vector<types::callback_info> init_callbacks;
	std::vector<types::callback_info> update_callbacks;
	std::vector<types::callback_info> remove_callbacks;

	std::vector<types::entity_callback_info> entity_created_callbacks;
	std::vector<types::entity_callback_info> entity_destroyed_callbacks;

	init_callbacks = std::move(init_callbacks_info);
	update_callbacks = std::move(update_callbacks_info);
	remove_callbacks = std::move(remove_callbacks_info);

	entity_created_callbacks = std::move(create_entity_callbacks_info);
	entity_destroyed_callbacks = std::move(destroy_entity_callbacks_info);

	init_callbacks_info.clear();
	update_callbacks_info.clear();
	remove_callbacks_info.clear();

	create_entity_callbacks_info.clear();
	destroy_entity_callbacks_info.clear();

	if(execution_events->entity_created_callback != nullptr) {
		for(auto& info : entity_created_callbacks) {
			execution_events->entity_created_callback(
				info.event,
				info.entity_id,
				info.placeholder_entity_id,
				execution_events->entity_created_callback_user_data
			);
		}
	}

	if(execution_events->init_callback != nullptr) {
		for(auto& component_info : init_callbacks) {
			execution_events->init_callback(
				component_info.event,
				component_info.entity_id,
				component_info.component_id,
				get_component_cache(
					component_info.entity_id,
					component_info.component_id
				),
				execution_events->init_callback_user_data
			);
		}
	}

	if(execution_events->update_callback != nullptr) {
		for(auto& component_info : update_callbacks) {
			execution_events->update_callback(
				component_info.event,
				component_info.entity_id,
				component_info.component_id,
				get_component_cache(
					component_info.entity_id,
					component_info.component_id
				),
				execution_events->update_callback_user_data
			);
		}
	}

	if(execution_events->remove_callback != nullptr) {
		for(auto& component_info : remove_callbacks) {
			execution_events->remove_callback(
				component_info.event,
				component_info.entity_id,
				component_info.component_id,
				get_component_cache(
					component_info.entity_id,
					component_info.component_id
				),
				execution_events->remove_callback_user_data
			);
		}
	}

	if(execution_events->entity_destroyed_callback != nullptr) {
		for(auto& info : entity_destroyed_callbacks) {
			execution_events->entity_destroyed_callback(
				info.event,
				info.entity_id,
				info.placeholder_entity_id,
				execution_events->entity_destroyed_callback_user_data
			);
		}
	}

	comp_cache.clear();
}

auto execution_callbacks::has_callbacks() const -> bool {
	if(init_callbacks_info.size() > 0) {
		return true;
	}

	if(update_callbacks_info.size() > 0) {
		return true;
	}

	if(remove_callbacks_info.size() > 0) {
		return true;
	}

	if(create_entity_callbacks_info.size() > 0) {
		return true;
	}

	if(destroy_entity_callbacks_info.size() > 0) {
		return true;
	}

	return false;
}

auto execution_callbacks::add_component_cache(
	ecsact_entity_id    entity,
	ecsact_component_id component_id,
	const void*         component_data
) -> void {
	auto serialized_comp =
		ecsact::serialize(ecsact_component{component_id, component_data});
	for(auto&& [itr, end] = comp_cache.equal_range(entity); itr != end; ++itr) {
		auto& cached_comp = itr->second;
		if(cached_comp._id == component_id) {
			cached_comp.data = serialized_comp;
			return;
		}
	}

	comp_cache.emplace(
		entity,
		types::cpp_execution_component{
			.entity_id = entity,
			._id = component_id,
			.data = serialized_comp,
		}
	);
}

auto execution_callbacks::get_component_cache(
	ecsact_entity_id    entity,
	ecsact_component_id component_id
) -> const void* {
	auto&& [start, end] = comp_cache.equal_range(entity);
	auto itr = std::find_if(start, end, [&](auto& p) -> bool {
		return p.second._id == component_id;
	});

	if(itr == end) {
		return nullptr;
	}

	return itr->second.data.data();
}

auto execution_callbacks::merge_cache( //
	const execution_callbacks& other
) -> void {
	// TODO: replace existing values with matfching comp id
	comp_cache.insert(other.comp_cache.begin(), other.comp_cache.end());
}

void execution_callbacks::init_callback(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) {
	auto self = static_cast<execution_callbacks*>(callback_user_data);

	auto result =
		std::erase_if(self->remove_callbacks_info, [&](auto& remove_cb_info) {
			return remove_cb_info.component_id == component_id &&
				remove_cb_info.entity_id == entity_id;
		});

	std::erase_if(self->update_callbacks_info, [&](auto& update_cb_info) {
		return update_cb_info.component_id == component_id &&
			update_cb_info.entity_id == entity_id;
	});

	auto info = detail::types::callback_info{};

	info.event = event;
	info.entity_id = entity_id;
	info.component_id = component_id;
	self->init_callbacks_info.push_back(info);
	self->add_component_cache(entity_id, component_id, component_data);
}

void execution_callbacks::update_callback(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) {
	auto self = static_cast<execution_callbacks*>(callback_user_data);

	auto info = detail::types::callback_info{};

	info.event = event;
	info.entity_id = entity_id;
	info.component_id = component_id;
	self->update_callbacks_info.push_back(info);
	self->add_component_cache(entity_id, component_id, component_data);
}

void execution_callbacks::remove_callback(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               callback_user_data
) {
	auto self = static_cast<execution_callbacks*>(callback_user_data);

	std::erase_if(self->init_callbacks_info, [&](auto& init_cb_info) {
		return init_cb_info.component_id == component_id &&
			init_cb_info.entity_id == entity_id;
	});

	std::erase_if(self->update_callbacks_info, [&](auto& update_cb_info) {
		return update_cb_info.component_id == component_id &&
			update_cb_info.entity_id == entity_id;
	});

	auto info = types::callback_info{};
	info.event = event;
	info.entity_id = entity_id;
	info.component_id = component_id;
	self->remove_callbacks_info.push_back(info);
	self->add_component_cache(entity_id, component_id, component_data);
}

void execution_callbacks::entity_created_callback(
	ecsact_event                 event,
	ecsact_entity_id             entity_id,
	ecsact_placeholder_entity_id placeholder_entity_id,
	void*                        callback_user_data
) {
	auto self = static_cast<execution_callbacks*>(callback_user_data);

	auto info = types::entity_callback_info{};

	info.event = event;
	info.entity_id = entity_id;
	info.placeholder_entity_id = placeholder_entity_id;

	self->create_entity_callbacks_info.push_back(info);
}

void execution_callbacks::entity_destroyed_callback(
	ecsact_event                 event,
	ecsact_entity_id             entity_id,
	ecsact_placeholder_entity_id placeholder_entity_id,
	void*                        callback_user_data
) {
	auto self = static_cast<execution_callbacks*>(callback_user_data);

	auto info = types::entity_callback_info{};

	info.event = event;
	info.entity_id = entity_id;
	info.placeholder_entity_id = placeholder_entity_id;

	self->destroy_entity_callbacks_info.push_back(info);
}
