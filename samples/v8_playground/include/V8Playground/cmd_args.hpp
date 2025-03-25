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
sreflect_struct(guid = "1bfb194d-abcd-42c0-9597-96f359a786aa" rttr = @full)
MainCommand {
    srttr_attr(CmdOption{
        .name = u8"entry",
        .short_name = u8'e',
        .help = u8"entry point for script",
    })
    skr::String entry = {};
};
}