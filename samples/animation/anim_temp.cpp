/**
 * @file main.cpp
 * @brief Temp Sample for SkrAnim Development
 * @author sailing-innocent
 * @date 2025-06-26
 */

#include "SkrAnim/ozz/base/maths/simd_math.h"
#include <iostream>
#include <SkrAnim/ozz/skeleton.h>
#include <string>
#include <SkrAnim/ozz/base/io/archive.h>
#include <SkrAnim/ozz/skeleton_utils.h>
#include <SkrAnim/ozz/local_to_model_job.h>
#include <SkrAnim/ozz/base/containers/vector.h>
#include <AnimDebugRuntime/util.h>

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

    ozz::vector<ozz::math::Float4x4> prealloc_models_;
    prealloc_models_.resize(skeleton.num_joints());
    ozz::animation::LocalToModelJob job;
    job.input    = skeleton.joint_rest_poses();
    job.output   = ozz::make_span(prealloc_models_);
    job.skeleton = &skeleton;
    if (!job.Run())
    {
        return -1;
    }
    for (int i = 0; i < skeleton.num_joints(); ++i)
    {
        std::cout << "Joint " << i << ": " << skeleton.joint_names()[i] << std::endl;
        auto rest_pose = ozz::animation::GetJointLocalRestPose(skeleton, i);
        std::cout << "  Rest Pose: "
                  << "Translation(" << rest_pose.translation.x << ", "
                  << rest_pose.translation.y << ", " << rest_pose.translation.z
                  << "), "
                  << "Rotation(" << rest_pose.rotation.x << ", "
                  << rest_pose.rotation.y << ", " << rest_pose.rotation.z
                  << ", " << rest_pose.rotation.w << "), "
                  << "Scale(" << rest_pose.scale.x << ", "
                  << rest_pose.scale.y << ", " << rest_pose.scale.z << ")"
                  << std::endl;
        ozz::math::Float4x4 model_matrix = prealloc_models_[i];
        float               model_data[16];
        ozz::math::StorePtrU(model_matrix.cols[0], model_data);
        ozz::math::StorePtrU(model_matrix.cols[1], model_data + 4);
        ozz::math::StorePtrU(model_matrix.cols[2], model_data + 8);
        ozz::math::StorePtrU(model_matrix.cols[3], model_data + 12);

        std::cout << "  Model Matrix: \n"
                  << "[" << model_data[0] << ", " << model_data[4] << ", "
                  << model_data[8] << ", " << model_data[12] << "]\n"
                  << "[" << model_data[1] << ", " << model_data[5] << ", "
                  << model_data[9] << ", " << model_data[13] << "]\n"
                  << "[" << model_data[2] << ", " << model_data[6] << ", "
                  << model_data[10] << ", " << model_data[14] << "]\n"
                  << "[" << model_data[3] << ", " << model_data[7] << ", "
                  << model_data[11] << ", " << model_data[15] << "]"
                  << std::endl;

        skr_float4x4_t mat = animd::ozz2skr_float4x4(model_matrix);
        std::cout << "  Model Matrix SKr: "
                  << "{" << mat.rows[0][0] << ", " << mat.rows[0][1] << ", "
                  << mat.rows[0][2] << ", " << mat.rows[0][3] << "}, "
                  << "{" << mat.rows[1][0] << ", " << mat.rows[1][1] << ", "
                  << mat.rows[1][2] << ", " << mat.rows[1][3] << "}, "
                  << "{" << mat.rows[2][0] << ", " << mat.rows[2][1] << ", "
                  << mat.rows[2][2] << ", " << mat.rows[2][3] << "}, "
                  << "{" << mat.rows[3][0] << ", " << mat.rows[3][1] << ", "
                  << mat.rows[3][2] << ", " << mat.rows[3][3] << "}"
                  << std::endl;

        auto parent_id = skeleton.joint_parents()[i];
        if (parent_id >= 0)
        {
            std::cout << "  Parent: " << skeleton.joint_names()[parent_id] << std::endl;
        }
        else
        {
            std::cout << "  Parent: None" << std::endl;
        }
        std::cout << "=========================" << std::endl;
    }
    return 0;
}