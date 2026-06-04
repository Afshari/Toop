#include "FibonacciSphere.h"
#include <cmath>
#include <stdexcept>

namespace Toop {

    static constexpr float kGoldenRatio = 1.6180339887498948482f;
    static constexpr float kPi = 3.14159265358979323846f;
    static constexpr int   kMaxSamples = 1000000;

    // --------------------------------------------------------------------------------
    float3cpu FibonacciSphere::Normalize(const float3cpu& v)
    {
        float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len < 1e-6f) return { 0.0f, 1.0f, 0.0f };
        return { v.x / len, v.y / len, v.z / len };
    }

    // --------------------------------------------------------------------------------
    float FibonacciSphere::Dot(const float3cpu& a, const float3cpu& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    // --------------------------------------------------------------------------------
    bool FibonacciSphere::IsBalded(
        const float3cpu& dir,
        const BaldPatchConfig& bald_patches)
    {
        // top patch
        if (dir.y > bald_patches.top_threshold)
            return true;

        // left eye
        float3cpu left_dir = Normalize({
            bald_patches.eye_left_dir.x,
            bald_patches.eye_left_dir.y,
            bald_patches.eye_left_dir.z });
        if (Dot(dir, left_dir) > bald_patches.eye_threshold)
            return true;

        // right eye
        float3cpu right_dir = Normalize({
            bald_patches.eye_right_dir.x,
            bald_patches.eye_right_dir.y,
            bald_patches.eye_right_dir.z });
        if (Dot(dir, right_dir) > bald_patches.eye_threshold)
            return true;

        return false;
    }

    // --------------------------------------------------------------------------------
    std::vector<float3cpu> FibonacciSphere::Generate(
        int                    num_strands,
        const BaldPatchConfig& bald_patches)
    {
        std::vector<float3cpu> result;
        result.reserve(num_strands);

        for (int i = 0; i < kMaxSamples && (int)result.size() < num_strands; i++)
        {
            float theta = acosf(1.0f - 2.0f * (i + 0.5f) / kMaxSamples);
            float phi = 2.0f * kPi * i / kGoldenRatio;

            float3cpu dir = {
                sinf(theta) * cosf(phi),
                cosf(theta),
                sinf(theta) * sinf(phi)
            };

            if (!IsBalded(dir, bald_patches))
                result.push_back(dir);
        }

        if ((int)result.size() < num_strands)
            throw std::runtime_error(
                "[ERROR] FibonacciSphere: not enough points after bald patch filtering. "
                "Increase kMaxSamples or reduce num_strands.");

        result.resize(num_strands);
        return result;
    }

} // namespace Toop