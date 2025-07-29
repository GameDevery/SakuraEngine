#pragma once
#include <SkrV8/bind_template/v8bt_record_base.hpp>

namespace skr
{
struct V8BTObject : V8BTRecordBase {
    inline static constexpr EV8BTKind kKind = EV8BTKind::Object;

    // kind
    EV8BTKind kind() const override;

    // convert helper
    v8::Local<v8::Value> to_v8(
        void* native_data
    ) const override final;
    bool to_native(
        void*                native_data,
        v8::Local<v8::Value> v8_value,
        bool                 is_init
    ) const override final;

    // invoke native api
    bool match_param(
        v8::Local<v8::Value> v8_param
    ) const override final;
    void push_param_native(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp,
        v8::Local<v8::Value> v8_value
    ) const override final;
    void push_param_native_pure_out(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_native(
        DynamicStack&         stack,
        const V8BTDataReturn& return_bind_tp
    ) const override final;
    v8::Local<v8::Value> read_return_from_out_param(
        DynamicStack&        stack,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // invoke v8 api
    v8::Local<v8::Value> make_param_v8(
        void*                native_data,
        const V8BTDataParam& param_bind_tp
    ) const override final;

    // field api
    v8::Local<v8::Value> get_field(
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    void set_field(
        v8::Local<v8::Value> v8_value,
        void*                obj,
        const RTTRType*      obj_type,
        const V8BTDataField& field_bind_tp
    ) const override final;
    v8::Local<v8::Value> get_static_field(
        const V8BTDataStaticField& field_bind_tp
    ) const override final;
    void set_static_field(
        v8::Local<v8::Value>       v8_value,
        const V8BTDataStaticField& field_bind_tp
    ) const override final;

    void setup(const ScriptBinderObject& binder);

protected:
    // helper
    V8BPObject* _get_or_make_proxy(void* address) const;
    V8BPObject* _new_bind_proxy(void* address, v8::Local<v8::Object> self = {}) const;

    // v8 callback
    static void _gc_callback(const ::v8::WeakCallbackInfo<V8BPObject>& data);
    static void _call_ctor(const ::v8::FunctionCallbackInfo<::v8::Value>& info);

private:
    v8::Global<::v8::FunctionTemplate> _v8_template = {};
};
} // namespace skr