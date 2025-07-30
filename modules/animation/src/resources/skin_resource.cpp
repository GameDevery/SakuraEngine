#include "SkrAnim/resources/skin_resource.hpp"

namespace skr::resource
{
skr_guid_t SkinFactory::GetResourceType()
{
    using namespace skr::anim;
    return skr::type_id_of<SkinResource>();
}
} // namespace skr::resource