#pragma once
#include "SkrInputSystem/input_mapping.hpp"
#include "SkrInputSystem/input_action.hpp"

namespace skr {
namespace input {

struct InputContextOptions
{
    uint32_t nothing_and_useless = 0;
};

struct SKR_INPUT_SYSTEM_API InputSystem
{
    virtual ~InputSystem() SKR_NOEXCEPT;

    [[nodiscard]] static InputSystem* Create() SKR_NOEXCEPT;
    static void Destroy(InputSystem* system) SKR_NOEXCEPT;

    virtual void update(float delta) SKR_NOEXCEPT = 0;

#pragma region InputMappingContexts
    // create
    [[nodiscard]] virtual RC<InputMappingContext> create_mapping_context() SKR_NOEXCEPT = 0;

    // remove
    virtual void remove_mapping_context(RC<InputMappingContext> ctx) SKR_NOEXCEPT = 0;

    // add
    virtual RC<InputMappingContext> add_mapping_context(RC<InputMappingContext> ctx, int32_t priority, const InputContextOptions& opts) SKR_NOEXCEPT = 0;

    // unmap and free all mapping contexts
    virtual void remove_all_contexts() SKR_NOEXCEPT = 0;
#pragma endregion

#pragma region InputMapping
    template<typename T, typename...Args>
    RC<T> create_mapping(Args&&...args) SKR_NOEXCEPT
    {
        return RC<T>::New(std::forward<Args>(args)...);
    }
#pragma endregion

#pragma region InputTrigger
    template<typename T, typename...Args>
    RC<T> create_trigger(Args&&...args) SKR_NOEXCEPT
    {
        return RC<T>::New(std::forward<Args>(args)...);
    }
#pragma endregion

#pragma region InputActions
    // create
    [[nodiscard]] virtual RC<InputAction> create_input_action(EValueType type) SKR_NOEXCEPT = 0;
#pragma endregion
};

} }
