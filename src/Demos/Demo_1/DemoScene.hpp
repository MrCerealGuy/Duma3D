#pragma once

#include "Engine/Scene/Scene.hpp"

namespace Duma3D::Demos::Demo_1
{
    Engine::Scene::Scene makeDemoScene();
    float sampleDemoTerrainHeight(float x, float z);
}
