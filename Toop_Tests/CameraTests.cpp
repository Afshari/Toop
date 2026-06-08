#include "pch.h"
#include "Camera.h"
#include "TestHelpers.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Toop {

    // --------------------------------------------------------------------------------
    // Init tests
    // --------------------------------------------------------------------------------

    TEST(CameraTest, DefaultPositionIsCorrect)
    {
        Camera cam;
        EXPECT_FLOAT_EQ(cam.GetPos().x, 0.0f);
        EXPECT_FLOAT_EQ(cam.GetPos().y, 2.0f);
        EXPECT_FLOAT_EQ(cam.GetPos().z, 6.0f);
    }

    TEST(CameraTest, DefaultForwardIsCorrect)
    {
        Camera cam;
        EXPECT_FLOAT_EQ(cam.GetForward().x, 0.0f);
        EXPECT_FLOAT_EQ(cam.GetForward().y, 0.0f);
        EXPECT_FLOAT_EQ(cam.GetForward().z, -1.0f);
    }

    TEST(CameraTest, DefaultSpeedIsCorrect)
    {
        Camera cam;
        EXPECT_FLOAT_EQ(cam.GetSpeed(), 0.03f);
    }

    // --------------------------------------------------------------------------------
    // LookAt tests
    // --------------------------------------------------------------------------------

    TEST(CameraTest, LookAtSetsPosition)
    {
        Camera cam;
        cam.LookAt({ 1.0f, 2.0f, 3.0f }, { 0.0f, 0.0f, 0.0f });
        EXPECT_FLOAT_EQ(cam.GetPos().x, 1.0f);
        EXPECT_FLOAT_EQ(cam.GetPos().y, 2.0f);
        EXPECT_FLOAT_EQ(cam.GetPos().z, 3.0f);
    }

    TEST(CameraTest, LookAtSetsNormalizedForward)
    {
        Camera cam;
        cam.LookAt({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 0.0f });
        float len = glm::length(cam.GetForward());
        EXPECT_NEAR(len, 1.0f, 1e-5f);
    }

    TEST(CameraTest, LookAtForwardPointsToTarget)
    {
        Camera cam;
        cam.LookAt({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 0.0f });
        EXPECT_NEAR(cam.GetForward().z, -1.0f, 1e-5f);
        EXPECT_NEAR(cam.GetForward().x, 0.0f, 1e-5f);
        EXPECT_NEAR(cam.GetForward().y, 0.0f, 1e-5f);
    }

    // --------------------------------------------------------------------------------
    // Preset tests
    // --------------------------------------------------------------------------------

    TEST(CameraTest, Preset1PlacesCameraCloseToTarget)
    {
        Camera cam;
        glm::vec3 target = { 0.0f, 1.5f, 0.0f };
        cam.SetPreset(1, target);
        float dist = glm::length(cam.GetPos() - target);
        EXPECT_NEAR(dist, 1.5f, 1e-4f);
    }

    TEST(CameraTest, Preset2PlacesCameraAtCorrectDistance)
    {
        Camera cam;
        glm::vec3 target = { 0.0f, 1.5f, 0.0f };
        cam.SetPreset(2, target);
        float dist = glm::length(cam.GetPos() - target);
        EXPECT_NEAR(dist, 3.0f, 1e-4f);
    }

    TEST(CameraTest, Preset3PlacesCameraAtCorrectDistance)
    {
        Camera cam;
        glm::vec3 target = { 0.0f, 1.5f, 0.0f };
        cam.SetPreset(3, target);
        float dist = glm::length(cam.GetPos() - target);
        EXPECT_NEAR(dist, 5.0f, 1e-4f);
    }

    TEST(CameraTest, Preset4PlacesCameraAtCorrectDistance)
    {
        Camera cam;
        glm::vec3 target = { 0.0f, 1.5f, 0.0f };
        cam.SetPreset(4, target);
        float dist = glm::length(cam.GetPos() - target);
        EXPECT_NEAR(dist, 8.0f, 1e-4f);
    }

    TEST(CameraTest, PresetAlwaysLooksAtTarget)
    {
        Camera cam;
        glm::vec3 target = { 0.0f, 1.5f, 0.0f };

        for (int p = 1; p <= 4; p++)
        {
            cam.SetPreset(p, target);
            glm::vec3 expected_forward = glm::normalize(target - cam.GetPos());
            EXPECT_NEAR(cam.GetForward().x, expected_forward.x, 1e-4f);
            EXPECT_NEAR(cam.GetForward().y, expected_forward.y, 1e-4f);
            EXPECT_NEAR(cam.GetForward().z, expected_forward.z, 1e-4f);
        }
    }

    // --------------------------------------------------------------------------------
    // Movement tests
    // --------------------------------------------------------------------------------

    TEST(CameraTest, KeyWMovesForward)
    {
        Camera cam;
        glm::vec3 pos_before = cam.GetPos();
        cam.HandleKeyW();
        glm::vec3 delta = cam.GetPos() - pos_before;
        EXPECT_GT(glm::length(delta), 0.0f);
    }

    TEST(CameraTest, KeySMovesBackward)
    {
        Camera cam;
        glm::vec3 pos_before = cam.GetPos();
        cam.HandleKeyS();
        glm::vec3 delta = cam.GetPos() - pos_before;
        EXPECT_GT(glm::length(delta), 0.0f);
    }

    TEST(CameraTest, KeyWAndSAreOpposite)
    {
        Camera cam;
        glm::vec3 pos_start = cam.GetPos();
        cam.HandleKeyW();
        glm::vec3 pos_after_w = cam.GetPos();
        cam.HandleKeyS();
        cam.HandleKeyS();
        glm::vec3 pos_after_s = cam.GetPos();

        // moving forward then back twice should be behind start
        float dist_w = glm::length(pos_after_w - pos_start);
        float dist_s = glm::length(pos_after_s - pos_start);
        EXPECT_GT(dist_s, 0.0f);
        EXPECT_NEAR(dist_w, dist_s, 1e-5f);
    }

    TEST(CameraTest, SpeedUpIncreasesSpeed)
    {
        Camera cam;
        float speed_before = cam.GetSpeed();
        cam.SpeedUp();
        EXPECT_GT(cam.GetSpeed(), speed_before);
    }

    TEST(CameraTest, SpeedDownDecreasesSpeed)
    {
        Camera cam;
        float speed_before = cam.GetSpeed();
        cam.SpeedDown();
        EXPECT_LT(cam.GetSpeed(), speed_before);
    }

    // --------------------------------------------------------------------------------
    // Matrix tests
    // --------------------------------------------------------------------------------

    TEST(CameraTest, ViewMatrixIsNotIdentity)
    {
        Camera cam;
        glm::mat4 view = cam.GetViewMatrix();
        EXPECT_NE(view, glm::mat4(1.0f));
    }

    TEST(CameraTest, ProjectionMatrixIsNotIdentity)
    {
        Camera cam;
        glm::mat4 proj = cam.GetProjectionMatrix(16.0f / 9.0f);
        EXPECT_NE(proj, glm::mat4(1.0f));
    }

    TEST(CameraTest, ProjectionMatrixChangesWithAspect)
    {
        Camera cam;
        glm::mat4 proj1 = cam.GetProjectionMatrix(16.0f / 9.0f);
        glm::mat4 proj2 = cam.GetProjectionMatrix(4.0f / 3.0f);
        EXPECT_NE(proj1, proj2);
    }

    // --------------------------------------------------------------------------------
    // Mouse tests
    // --------------------------------------------------------------------------------

    TEST(CameraTest, MouseViewChangesForward)
    {
        Camera cam;
        glm::vec3 forward_before = cam.GetForward();
        cam.HandleMouseView(10.0f, 0.0f);
        EXPECT_NE(cam.GetForward(), forward_before);
    }

    TEST(CameraTest, MouseOrbitKeepsDistanceToCenter)
    {
        Camera cam;
        glm::vec3 center = { 0.0f, 1.5f, 0.0f };
        float dist_before = glm::length(cam.GetPos() - center);
        cam.HandleMouseOrbit(10.0f, 5.0f, center);
        float dist_after = glm::length(cam.GetPos() - center);
        EXPECT_NEAR(dist_before, dist_after, 0.01f);
    }

    TEST(CameraTest, ForwardRemainsNormalizedAfterMouseView)
    {
        Camera cam;
        for (int i = 0; i < 100; i++)
            cam.HandleMouseView(5.0f, 3.0f);
        float len = glm::length(cam.GetForward());
        EXPECT_NEAR(len, 1.0f, 1e-4f);
    }

    TEST(CameraTest, WheelMovesAlongForward)
    {
        Camera cam;
        glm::vec3 pos_before = cam.GetPos();
        cam.HandleWheel(1.0f);
        glm::vec3 delta = cam.GetPos() - pos_before;
        EXPECT_GT(glm::length(delta), 0.0f);
    }

} // namespace Toop