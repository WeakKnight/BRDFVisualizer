#pragma once
#include "Falcor.h"

using namespace Falcor;

class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

    void LoadScene(const char* scenePath);
    Scene::SharedPtr GetScene();
    /*
    Basic Linear Sampler With Default Parameters For General Use
    */
    Sampler::SharedPtr LinearSampler;
    /*
    Basic Point Sampler With Default Parameters For General Use
    */
    Sampler::SharedPtr PointSampler;

    void Update(RenderContext* renderContext);

private:
    Scene::SharedPtr m_scene = nullptr;
};

void InitResourceManager();
void DestroyResourceManager();

ResourceManager* GetResourceManager();

