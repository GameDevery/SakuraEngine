#pragma once
#include "SkrBase/config.h"
#include "SkrContainers/string.hpp"

namespace skd::asset
{
class ImporterFactory
{
public:
    virtual ~ImporterFactory() = default;
    virtual bool CanImport(const skr::String& path) const = 0;
    virtual skr::span<skr::GUID> GetAssetTypes() const = 0;
    virtual int ImportAs(const skr::String& path, skr::GUID type) = 0;
    virtual int Update() = 0;
    virtual skr::String GetName() const = 0;
    virtual skr::String GetDescription() const = 0;
};
} // namespace skd::asset