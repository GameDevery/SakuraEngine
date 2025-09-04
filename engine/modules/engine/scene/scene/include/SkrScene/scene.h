#pragma once

#include "SkrScene/actor.h"

#if !defined(__meta__)
    #include "SkrScene/scene.generated.h"
#endif

namespace skr
{
struct Actor;
struct RootActor;

sreflect_struct(
    guid = "01990eb5-b4e9-741c-b764-8d1c71e0beb2";
    rttr = @enable)
SKR_SCENE_API Scene
{
public:
    Scene() {}
    ~Scene() SKR_NOEXCEPT;
    void serialize();

    skr_guid_t root_actor_guid;
    sattr(serde = @disable)
    skr::Map<skr_guid_t, skr::RC<skr::Actor>> actors;
};

// template<>
// struct skr::BinSerde<skr::Scene> {
//     static void read(SBinaryReader* r, skr::Scene& v);
//     static void write(SBinaryWriter* w, const skr::Scene& v);
// };

template <>
struct SKR_SCENE_API JsonSerde<skr::Scene>
{
    static bool read(skr::archive::JsonReader* r, skr::Scene& v);
    static bool write(skr::archive::JsonWriter* w, const skr::Scene& v);
};

} // namespace skr