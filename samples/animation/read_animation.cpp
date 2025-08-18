// sample to read an ozz skeleton file and print its details

#include "SkrAnim/ozz/animation.h"
#include "SkrAnim/ozz/base/maths/simd_math.h"
#include <iostream>
#include <SkrAnim/ozz/base/io/archive.h>
#include <SkrAnim/ozz/skeleton_utils.h>
#include <SkrAnim/ozz/local_to_model_job.h>
#include <SkrAnim/ozz/base/containers/vector.h>
#include "SkrContainersDef/vector.hpp"
#include <AnimDebugRuntime/util.h>

int main(int argc, char** argv)
{
    std::cout << "Hello, Sample to load ozz animation!" << std::endl;

    // read ozz file
    auto          filename = "D:/ws/data/assets/media/bin/ruby_animation.ozz";
    ozz::io::File file(filename, "rb");

    // Checks file status, which can be closed if filename is invalid.
    if (!file.opened())
    {
        std::cout << "Cannot open file " << filename << "." << std::endl;
        return EXIT_FAILURE;
    }

    // deserialize
    ozz::io::IArchive archive(&file);

    // test archive
    if (!archive.TestTag<ozz::animation::Animation>())
    {
        std::cout << "Archive doesn't contain the expected object type." << std::endl;
        return EXIT_FAILURE;
    }

    ozz::animation::Animation anim;
    archive >> anim;
    std::cout << "Animation loaded with duration: " << anim.duration() << " seconds." << std::endl;

    return 0;
}