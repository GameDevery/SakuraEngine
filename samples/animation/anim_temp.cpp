/**
 * @file main.cpp
 * @brief Temp Sample for SkrAnim Development
 * @author sailing-innocent
 * @date 2025-06-26
 */

#include <iostream>
#include <SkrAnim/ozz/skeleton.h>
#include <string>
#include <SkrAnim/ozz/base/io/archive.h>

int main(int argc, char** argv)
{
    std::cout << "Hello, SkrAnim!" << std::endl;
    ozz::animation::Skeleton skeleton;
    std::cout << "Skeleton created with " << skeleton.num_joints() << " joints." << std::endl;

    // read ozz file
    auto          filename = "D:/ws/data/assets/media/bin/ruby_skeleton.ozz";
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
    if (!archive.TestTag<ozz::animation::Skeleton>())
    {
        std::cout << "Archive doesn't contain the expected object type." << std::endl;
        return EXIT_FAILURE;
    }
    // Create Runtime Skeleton
    archive >> skeleton;

    // print skeleton info
    std::cout << "Skeleton loaded with " << skeleton.num_joints() << " joints." << std::endl;

    return 0;
}