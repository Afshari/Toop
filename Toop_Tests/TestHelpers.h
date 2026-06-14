#pragma once
#include "Config.h"
#include "SpherePhysics.h"
#include <vector>
#include <string>

namespace Toop::TestHelpers {

    // --------------------------------------------------------------------------------
    // Config helpers
    // --------------------------------------------------------------------------------

    inline Config MakeDefaultConfig()
    {
        return Config::Default();
    }

    inline SpherePhysics MakeSphere()
    {
        auto config = MakeDefaultConfig();
        SpherePhysics sphere;
        sphere.Init(config.sphere, config.room, config.sim.sphere_radius);
        return sphere;
    }

    // --------------------------------------------------------------------------------
    // argc/argv builder for CLParser tests
    // --------------------------------------------------------------------------------

    struct ArgList
    {
        std::vector<std::string> strings;
        std::vector<char*>       argv;

        ArgList(std::initializer_list<const char*> args)
        {
            for (auto a : args)
                strings.push_back(a);
            for (auto& s : strings)
                argv.push_back(const_cast<char*>(s.c_str()));
        }

        int    argc() { return (int)argv.size(); }
        char** data() { return argv.data(); }
    };

    // --------------------------------------------------------------------------------
    static BaldPatchConfig MakeDefaultBaldPatches()
    {
        return Config::Default().bald_patches;
    }


} // namespace Toop::TestHelpers