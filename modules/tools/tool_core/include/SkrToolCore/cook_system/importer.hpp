#pragma once
#include "SkrBase/types/guid.h"
#include "SkrCore/memory/rc.hpp"
#include "SkrToolCore/fwd_types.hpp"
#ifndef __meta__
    #include "SkrToolCore/cook_system/importer.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
using namespace skr;
template <class T>
void RegisterImporter(skr::GUID guid);

sreflect_struct(guid = "76044661-E2C9-43A7-A4DE-AEDD8FB5C847"; serde = @json)
TOOL_CORE_API Importer
{
    static constexpr uint32_t kDevelopmentVersion = UINT32_MAX;
    virtual ~Importer() = default;
    virtual void* Import(skr::io::IRAMService*, CookContext* context) = 0;
    virtual void Destroy(void*) = 0;
    static uint32_t Version() { return kDevelopmentVersion; }

private:
    sattr(serde = @disable)
    mutable ::std::atomic<::skr::RCCounterType> zz_skr_rc = 0;
    sattr(serde = @disable)
    mutable ::std::atomic<::skr::RCWeakRefCounter*> zz_skr_weak_counter = nullptr;
                                                                                
public:                                                                         
    inline skr::RCCounterType skr_rc_count() const                   
    {                                                                           
        return skr::rc_ref_count(zz_skr_rc);                                    
    }                                                                           
    inline skr::RCCounterType skr_rc_add_ref() const                 
    {                                                                        
        return skr::rc_add_ref(zz_skr_rc);                                    
    }                                                                     
    inline skr::RCCounterType skr_rc_add_ref_unique() const      
    {                                                                     
        return skr::rc_add_ref_unique(zz_skr_rc);                      
    }                                                                    
    inline skr::RCCounterType skr_rc_release_unique() const           
    {                                                                        
        return skr::rc_release_unique(zz_skr_rc);                        
    }                                                             
    inline skr::RCCounterType skr_rc_weak_lock() const      
    {                                                               
        return skr::rc_weak_lock(zz_skr_rc);                           
    }                                                               
    inline skr::RCCounterType skr_rc_release() const        
    {                                                                   
        return skr::rc_release(zz_skr_rc);                              
    }                                                                    
    inline skr::RCCounterType skr_rc_weak_ref_count() const         
    {                                                                 
        return skr::rc_weak_ref_count(zz_skr_weak_counter);              
    }                                                                    
    inline skr::RCWeakRefCounter* skr_rc_weak_ref_counter() const    
    {                                                                    
        return skr::rc_get_or_new_weak_ref_counter(zz_skr_weak_counter);  
    }                                                                   
    inline void skr_rc_weak_ref_counter_notify_dead() const    
    {                                                                        
        skr::rc_notify_weak_ref_counter_dead(zz_skr_weak_counter); 
    }
};

struct TOOL_CORE_API ImporterTypeInfo
{
    skr::RC<Importer> (*Load)(const AssetMetaFile* record, skr::archive::JsonReader* object);
    uint32_t (*Version)();
};

struct ImporterRegistry
{
    virtual skr::RC<Importer> LoadImporter(const AssetMetaFile* record, skr::GUID* pGuid = nullptr) = 0;
    virtual uint32_t GetImporterVersion(skr::GUID type) = 0;
    virtual void RegisterImporter(skr::GUID type, ImporterTypeInfo info) = 0;
};

TOOL_CORE_API ImporterRegistry* GetImporterRegistry();
} // namespace skd::asset

template <class T>
void skd::asset::RegisterImporter(skr::GUID guid)
{
    auto registry = GetImporterRegistry();
    auto loader =
        +[](const AssetMetaFile* record, skr::archive::JsonReader* object) -> skr::RC<Importer> {
        auto importer = skr::RC<T>::New();
        skr::json_read(object, *importer);
        return importer;
    };
    ImporterTypeInfo info{ loader, T::Version };
    registry->RegisterImporter(guid, info);
}