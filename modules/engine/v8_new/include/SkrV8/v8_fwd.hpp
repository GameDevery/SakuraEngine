#pragma once

namespace skr
{
// basic concepts
struct V8Isolate;
struct V8Context;
struct V8Value;
struct V8VirtualModuleNode;
struct V8VirtualModule;

// bind template
struct V8BindTemplate;
struct V8BTPrimitive;
struct V8BTEnum;
struct V8BTMapping;
struct V8BTRecordBase;
struct V8BTValue;
struct V8BTObject;
struct V8BTGeneric;

// bind template data
struct V8BTDataModifier;
struct V8BTDataField;
struct V8BTDataStaticField;
struct V8BTDataParam;
struct V8BTDataReturn;
struct V8BTDataFunctionBase;
struct V8BTDataMethod;
struct V8BTDataStaticMethod;
struct V8BTDataProperty;
struct V8BTDataStaticProperty;
struct V8BTDataCtor;
struct V8BTDataCallScript;

// bind proxy
struct V8BindProxy;
struct V8BPRecord;
struct V8BPValue;
struct V8BPObject;

// dumper
struct TSDefBuilder;
} // namespace skr