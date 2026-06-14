#include "pch.h"
#include "FibonacciSphere.h"
#include "Config.h"
#include <cmath>
#include "TestHelpers.h"
using namespace Toop::TestHelpers;

namespace Toop {

    // --------------------------------------------------------------------------------
    // Count tests
    // --------------------------------------------------------------------------------

    TEST(FibonacciSphereTest, GeneratesExactStrandCount)
    {
        auto bald = MakeDefaultBaldPatches();
        auto dirs = FibonacciSphere::Generate(14549, bald);
        EXPECT_EQ((int)dirs.size(), 14549);
    }

    TEST(FibonacciSphereTest, GeneratesSmallCount)
    {
        auto bald = MakeDefaultBaldPatches();
        auto dirs = FibonacciSphere::Generate(100, bald);
        EXPECT_EQ((int)dirs.size(), 100);
    }

    TEST(FibonacciSphereTest, ThrowsWhenCountTooLarge)
    {
        auto bald = MakeDefaultBaldPatches();
        EXPECT_THROW(
            FibonacciSphere::Generate(999999, bald),
            std::runtime_error);
    }

    // --------------------------------------------------------------------------------
    // Unit length tests
    // --------------------------------------------------------------------------------

    TEST(FibonacciSphereTest, AllPointsAreUnitLength)
    {
        auto bald = MakeDefaultBaldPatches();
        auto dirs = FibonacciSphere::Generate(1000, bald);

        for (const auto& d : dirs)
        {
            float len = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
            EXPECT_NEAR(len, 1.0f, 1e-5f);
        }
    }

    // --------------------------------------------------------------------------------
    // Bald patch tests
    // --------------------------------------------------------------------------------

    TEST(FibonacciSphereTest, NoPointInTopPatch)
    {
        auto bald = MakeDefaultBaldPatches();
        auto dirs = FibonacciSphere::Generate(14549, bald);

        for (const auto& d : dirs)
            EXPECT_LE(d.y, bald.top_threshold);
    }

    TEST(FibonacciSphereTest, NoPointInLeftEyePatch)
    {
        auto bald = MakeDefaultBaldPatches();
        auto dirs = FibonacciSphere::Generate(14549, bald);

        // normalize eye direction
        float ex = bald.eye_left_dir.x;
        float ey = bald.eye_left_dir.y;
        float ez = bald.eye_left_dir.z;
        float len = sqrtf(ex * ex + ey * ey + ez * ez);
        ex /= len; ey /= len; ez /= len;

        for (const auto& d : dirs)
        {
            float dot = d.x * ex + d.y * ey + d.z * ez;
            EXPECT_LE(dot, bald.eye_threshold);
        }
    }

    TEST(FibonacciSphereTest, NoPointInRightEyePatch)
    {
        auto bald = MakeDefaultBaldPatches();
        auto dirs = FibonacciSphere::Generate(14549, bald);

        float ex = bald.eye_right_dir.x;
        float ey = bald.eye_right_dir.y;
        float ez = bald.eye_right_dir.z;
        float len = sqrtf(ex * ex + ey * ey + ez * ez);
        ex /= len; ey /= len; ez /= len;

        for (const auto& d : dirs)
        {
            float dot = d.x * ex + d.y * ey + d.z * ez;
            EXPECT_LE(dot, bald.eye_threshold);
        }
    }

    // --------------------------------------------------------------------------------
    // Distribution tests
    // --------------------------------------------------------------------------------

    TEST(FibonacciSphereTest, PointsAreDistributedOnBothHemispheres)
    {
        auto bald = MakeDefaultBaldPatches();
        auto dirs = FibonacciSphere::Generate(14549, bald);

        int positive_y = 0;
        int negative_y = 0;

        for (const auto& d : dirs)
        {
            if (d.y > 0.0f) positive_y++;
            else             negative_y++;
        }

        // both hemispheres should have significant coverage
        EXPECT_GT(positive_y, 1000);
        EXPECT_GT(negative_y, 1000);
    }
    TEST(FibonacciSphereTest, PointsSpanAllQuadrants)
    {
        auto bald = MakeDefaultBaldPatches();
        auto dirs = FibonacciSphere::Generate(1000, bald);

        int pxpz = 0, pxnz = 0, nxpz = 0, nxnz = 0;

        for (const auto& d : dirs)
        {
            if (d.x >= 0 && d.z >= 0) pxpz++;
            else if (d.x >= 0 && d.z < 0) pxnz++;
            else if (d.x < 0 && d.z >= 0) nxpz++;
            else                            nxnz++;
        }

        EXPECT_GT(pxpz, 0);
        EXPECT_GT(pxnz, 0);
        EXPECT_GT(nxpz, 0);
        EXPECT_GT(nxnz, 0);
    }

} // namespace Toop