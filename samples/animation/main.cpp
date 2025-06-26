/**
 * @file main.cpp
 * @brief Temp Sample for SkrAnim Development
 * @author sailing-innocent
 * @date 2025-06-26
 */

#include <iostream>
#include <SkrAnim/ozz/skeleton.h>

int main(int argc, char** argv)
{
    std::cout << "Hello, SkrAnim!" << std::endl;
    ozz::animation::Skeleton skeleton;
    std::cout << "Skeleton created with " << skeleton.num_joints() << " joints." << std::endl;
    return 0;
}