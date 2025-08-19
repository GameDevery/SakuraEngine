#include "SkrAnim/resources/skin_resource.hpp"

namespace skr
{
skr_guid_t SkinFactory::GetResourceType()
{
    using namespace skr;
    return skr::type_id_of<SkinResource>();
}
} // namespace skr