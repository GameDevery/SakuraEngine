#pragma once
#include "SkrInputSystem/input_value.hpp"
#include "SkrContainers/stl_function.hpp" // IWYU pragma: keep
#include "SkrCore/memory/rc.hpp"

namespace skr
{
namespace input
{

struct InputModifier;
struct InputTrigger;

template <typename ValueType>
using ActionEvent = skr::stl_function<void(const ValueType&)>;

using ActionEventId = skr_guid_t;

static const ActionEventId kEventId_Invalid = { 0xbbd09231, 0xa76b, 0x4c0f, { 0x83, 0x2e, 0x11, 0x7f, 0xd6, 0xac, 0x5c, 0x1b } };

struct SKR_INPUT_SYSTEM_API InputAction {
    SKR_RC_IMPL()

    inline InputAction(EValueType type) SKR_NOEXCEPT
        : value_type(type)
    {
    }

    virtual ~InputAction() SKR_NOEXCEPT;

    template <typename ValueType>
    ActionEventId bind_event(const ActionEvent<ValueType>& event, ActionEventId id = kEventId_Invalid) SKR_NOEXCEPT;

    virtual ActionEventId bind_event(const ActionEvent<InputValueStorage>& event, ActionEventId id = kEventId_Invalid) SKR_NOEXCEPT = 0;

    virtual const InputValueStorage& get_value() const SKR_NOEXCEPT = 0;

    virtual bool unbind_event(ActionEventId id) SKR_NOEXCEPT = 0;

    virtual void add_trigger(RC<InputTrigger> trigger) SKR_NOEXCEPT = 0;

    virtual void remove_trigger(RC<InputTrigger> trigger) SKR_NOEXCEPT = 0;

    virtual void add_modifier(RC<InputModifier> modifier) SKR_NOEXCEPT = 0;

    virtual void remove_modifier(RC<InputModifier> modifier) SKR_NOEXCEPT = 0;

    const EValueType value_type = EValueType::kBool;

protected:
    friend struct InputMapping;
    friend struct InputSystem;
    friend struct InputSystemImpl;
    virtual void set_value(InputValueStorage value) SKR_NOEXCEPT        = 0;
    virtual void clear_value() SKR_NOEXCEPT                             = 0;
    virtual void accumulate_value(InputValueStorage value) SKR_NOEXCEPT = 0;
    virtual void process_modifiers(float delta) SKR_NOEXCEPT            = 0;
    virtual void process_triggers(float delta) SKR_NOEXCEPT             = 0;
};

template <>
SKR_INPUT_SYSTEM_API ActionEventId
InputAction::bind_event<float>(const ActionEvent<float>& event, ActionEventId id) SKR_NOEXCEPT;

template <>
SKR_INPUT_SYSTEM_API ActionEventId
InputAction::bind_event<skr_float2_t>(const ActionEvent<skr_float2_t>& event, ActionEventId id) SKR_NOEXCEPT;

template <>
SKR_INPUT_SYSTEM_API ActionEventId
InputAction::bind_event<skr_float3_t>(const ActionEvent<skr_float3_t>& event, ActionEventId id) SKR_NOEXCEPT;

template <>
SKR_INPUT_SYSTEM_API ActionEventId
InputAction::bind_event<bool>(const ActionEvent<bool>& event, ActionEventId id) SKR_NOEXCEPT;

} // namespace input
} // namespace skr