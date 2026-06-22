#pragma once

#ifndef TOOP_HEADLESS
#ifdef TOOP_DEBUG

#include <memory>
#include <glm/glm.hpp>
#include "DebugContext.h"
#include "Camera.h"
#include "SpherePhysics.h"
#include "cpu/Renderer.h"
#include "Config.h"
#include "cpu/DebugRenderer.h"

namespace Toop {

    class InputHandler;

    class DebugManager
    {
    public:
        void Init();
        void Shutdown();

        // gathers data from the live systems into m_context - call once per
        // frame, before any character-behavior update
        void Sync(
            const Ray& camera_ray,
            const Camera& camera,
            const SpherePhysics& sphere,
            const Renderer& renderer,
            const InputHandler& input,
            const Config& config);

        // draws everything currently toggled on in m_context
        void Draw(const glm::mat4& view, const glm::mat4& proj);

        void TakeSnapshot();
        void ClearSnapshots();

        DebugContext& GetContext() { return m_context; }
        const DebugContext& GetContext() const { return m_context; }

    private:
        DebugContext                   m_context;
        std::unique_ptr<DebugRenderer> m_draw;
    };

} // namespace Toop

#endif // TOOP_DEBUG
#endif // TOOP_HEADLESS