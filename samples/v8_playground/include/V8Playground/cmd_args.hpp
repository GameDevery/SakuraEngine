#pragma once
#include "SkrContainers/string.hpp"
#include <SkrBase/config.h>
#include <SkrBase/meta.h>
#include <SkrCore/cli.hpp>
#ifndef __meta__
    #include "V8Playground/cmd_args.generated.h"
#endif

namespace v8_play
{
sreflect_struct(guid = "c30115c5-4945-468f-a579-523202d4f42b" rttr = @full)
    SubCommandDumpDefine {
    srttr_attr(CmdOption{
        .short_name = u8'o',
        .help       = u8"output .d.ts file",
    })
    skr::String output = {};

    srttr_attr(CmdExec{})
    void exec()
    {
        SKR_LOG_FMT_INFO(u8"Sub Command Dump Define Called");
        SKR_LOG_FMT_INFO(u8"output: {}", output.c_str());
    }
};

sreflect_struct(guid = "1bfb194d-abcd-42c0-9597-96f359a786aa" rttr = @full)
MainCommand {
    srttr_attr(CmdOption{
        .name       = u8"entry",
        .short_name = u8'e',
        .help       = u8"entry point for script",
    })
    skr::String entry = {};

    srttr_attr(CmdOption{
        .help = u8"use strict mode for script",
    }) bool strict_mode = false;

    srttr_attr(CmdOption{
        .help = u8"just test optional arg",
    })
    skr::Optional<skr::String> test_optional = {};

    srttr_attr(CmdOption{
        .name = u8"...",
        .help = u8"test rest args",
    })
    skr::Vector<skr::String> rest_args = {};

    srttr_attr(CmdSub{
        .short_name = u8'd',
        .help       = u8"dump .d.ts files",
        .usage      = u8"V8Playground dump_def [options]",
    })
    SubCommandDumpDefine dump_def = {};

    srttr_attr(CmdExec{})
    void exec()
    {
        SKR_LOG_FMT_INFO(u8"Main Command Called");
        SKR_LOG_FMT_INFO(u8"entry: {}", entry.c_str());
        SKR_LOG_FMT_INFO(u8"strict mode: {}", strict_mode ? u8"true" : u8"false");
        SKR_LOG_FMT_INFO(u8"test optional: {}", test_optional.has_value() ? test_optional->c_str() : u8"nullopt");
        SKR_LOG_FMT_INFO(u8"rest args: {}", skr::String::Join(rest_args, u8", "));
    }
};
} // namespace v8_play