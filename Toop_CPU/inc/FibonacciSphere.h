#pragma once
#include "Config.h"
#include <vector>

namespace Toop {

    struct float3cpu
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    class FibonacciSphere
    {
    public:
        // generates num_strands filtered points on unit sphere
        // filters out bald patches defined in config
        // returns exactly num_strands points
        static std::vector<float3cpu> Generate(
            int                   num_strands,
            const BaldPatchConfig& bald_patches);

    private:
        static bool IsBalded(
            const float3cpu& dir,
            const BaldPatchConfig& bald_patches);

        static float3cpu Normalize(const float3cpu& v);

        static float Dot(const float3cpu& a, const float3cpu& b);
    };

} // namespace Toop