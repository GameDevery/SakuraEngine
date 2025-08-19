#include "SkrAnim/resources/animation_resource.hpp"
#include "SkrAnim/ozz/base/io/archive.h"

namespace skr
{
bool BinSerde<AnimResource>::read(SBinaryReader* r, AnimResource& v)
{
    ozz::io::SkrStream stream(r, nullptr);
    ozz::io::IArchive  archive(&stream);
    archive >> v.animation;
    return true;
}
bool BinSerde<AnimResource>::write(SBinaryWriter* w, const AnimResource& v)
{
    ozz::io::SkrStream stream(nullptr, w);
    ozz::io::OArchive  archive(&stream);
    archive << v.animation;
    return true;
}
} // namespace skr

namespace skr
{
skr_guid_t AnimFactory::GetResourceType()
{
    return skr::type_id_of<AnimResource>();
}
} // namespace skr