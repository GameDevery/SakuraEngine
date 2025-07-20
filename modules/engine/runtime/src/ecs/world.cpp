#include "SkrRT/ecs/world.hpp"

namespace skr::ecs
{

CreationBuilder& CreationBuilder::add_component(sugoi_type_index_t type)
{
	if(Components.contains(type))
	{
		return *this;
	}
	Components.add(type);
	Fields.add(-1);
	return *this;
}

CreationBuilder& CreationBuilder::add_meta_entity(Entity Entity)
{
	if (MetaEntities.contains(Entity))
	{
		return *this;
	}
	MetaEntities.add(Entity);
	return *this;
}

CreationBuilder& CreationBuilder::add_component(sugoi_type_index_t type, intptr_t Field)
{
	if (auto index = Components.find(type))
	{
		Fields[index.index()] = Field;
		return *this;
	}
	Components.add(type);
	Fields.add(Field);
	return *this;
}

void CreationBuilder::commit() SKR_NOEXCEPT
{
    Components.sort();
    MetaEntities.sort();
}

void World::initialize() SKR_NOEXCEPT
{
    storage = sugoiS_create();
}

void World::finalize() SKR_NOEXCEPT
{
    sugoiS_release(storage);
    storage = nullptr;
}

sugoi_storage_t* World::get_storage() SKR_NOEXCEPT
{
    return storage;
}

} // namespace skr::ecs