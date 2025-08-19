#include "SkrAnim/resources/skeleton_resource.hpp"
#include "SkrAnim/ozz/base/io/archive.h"

namespace skr
{
bool BinSerde<skr::SkeletonResource>::read(SBinaryReader* r, skr::SkeletonResource& v)
{
    ozz::io::SkrStream stream(r, nullptr);
    ozz::io::IArchive archive(&stream);
    archive >> v.skeleton;
    return true;
}
bool BinSerde<skr::SkeletonResource>::write(SBinaryWriter* w, const skr::SkeletonResource& v)
{
    ozz::io::SkrStream stream(nullptr, w);
    ozz::io::OArchive archive(&stream);
    archive << v.skeleton;
    return true;
}
} // namespace skr

namespace skr
{
skr_guid_t SkelFactory::GetResourceType()
{
    return ::skr::type_id_of<skr::SkeletonResource>();
}
} // namespace skr