#pragma once
#include "SkrScene/scene.h"

namespace skr
{

Scene::~Scene() SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"Scene Delete");
}

void Scene::serialize()
{
    for (auto& actor : actors)
    {
        actor.value->serialize();
    }
}

inline bool Parse(skr::archive::JsonReader& reader, const char8_t* key, bool required = true)
{
    bool exist = true;
    auto parse = reader.Key(key);
    parse.error_then([&](skr::archive::JsonReadError e) {
        exist = false;
        if (!required && (e == skr::archive::JsonReadError::KeyNotFound))
            return;
        SKR_LOG_FATAL(u8"Parse scene file failed, error code %d");
    });
    return exist;
}

bool JsonSerde<Scene>::read(skr::archive::JsonReader* r, Scene& v)
{

    r->StartObject();

    auto& reader = *r;
    if (Parse(reader, u8"root_actor", true))
    {
        skr::json_read(r, v.root_actor_guid);
    }
    auto root = skr::ActorManager::GetInstance().GetRoot();
    root.lock()->InitWorld();

    size_t actors_count;
    reader.Key(u8"actors");
    // First Round, initialize actor instance with type guid and actor guid
    reader.StartArray(actors_count);
    for (size_t i = 0; i < actors_count; i++)
    {
        reader.StartObject();
        reader.Key(u8"actor_guid");
        skr::GUID actor_guid;
        skr::json_read(&reader, actor_guid);
        auto actor_iter = v.actors.find(actor_guid);

        skr::RCWeak<Actor> actor_ref;

        // check if actor already exists
        if (actor_iter)
        {
            actor_ref = actor_iter.value();
        }
        else
        {
            reader.Key(u8"actor_rttr_type_guid");
            skr::GUID actor_attr_type_guid;
            skr::json_read(&reader, actor_attr_type_guid);
            // create actor with type guid
            auto actor = skr::ActorManager::GetInstance().CreateActor(actor_attr_type_guid);
            actor->Initialize(actor_guid);
            v.actors.add(actor_guid, actor);
            actor_iter = v.actors.find(actor_guid);
            actor_ref = actor_iter.value();
        }
        reader.Key(u8"actor_data");
        skr::json_read(&reader, *actor_ref.lock()); // from json to serialized
        reader.EndObject();
    }
    reader.EndArray();

    for (auto& actor : v.actors)
    {
        actor.value->deserialize();
    }


    r->EndObject();
    return true;
}

bool JsonSerde<Scene>::write(skr::archive::JsonWriter* w, const Scene& v)
{
    w->StartObject();
    w->Key(u8"root_actor");
    if (!skr::JsonSerde<skr_guid_t>::write(w, v.root_actor_guid)) return false;
    w->Key(u8"actors");
    // write actors in list
    w->StartArray();
    for (auto& [k, actor] : v.actors)
    {
        w->StartObject();
        w->Key(u8"actor_guid");
        if (!skr::JsonSerde<skr_guid_t>::write(w, k)) return false;
        w->Key(u8"actor_rttr_type_guid");
        if (!skr::JsonSerde<skr_guid_t>::write(w, actor->GetRTTRTypeGUID())) return false;
        w->Key(u8"actor_data");
        if (!skr::JsonSerde<Actor>::write(w, *actor.get())) return false;
        w->EndObject();
    }
    w->EndArray();
    w->EndObject();
    return true;
}

} // namespace skr