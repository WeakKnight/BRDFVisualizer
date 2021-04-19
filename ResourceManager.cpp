#include "ResourceManager.h"
#include "RenderSettings.h"

static ResourceManager* s_resourceManager = nullptr;

void InitResourceManager()
{
    s_resourceManager = new ResourceManager();
}

void DestroyResourceManager()
{
    delete s_resourceManager;
}

ResourceManager* GetResourceManager()
{
    return s_resourceManager;
}

ResourceManager::ResourceManager()
{
    Sampler::Desc linearSamplerDesc;
    linearSamplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(8);
    LinearSampler = Sampler::create(linearSamplerDesc);

    Sampler::Desc pointSamplerDesc;
    pointSamplerDesc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point);
    PointSampler = Sampler::create(pointSamplerDesc);

    LoadScene(GetRenderSettings()->scene.c_str());
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::LoadScene(const char* scenePath)
{
    m_scene = Scene::create(scenePath);
    m_scene->bindSamplerToMaterials(LinearSampler);
    m_scene->setCameraController(Scene::CameraControllerType::FirstPerson);
    m_scene->toggleAnimations(false);
}

void ResourceManager::Update(RenderContext* renderContext)
{
    m_scene->getLightCollection(renderContext);
}

Scene::SharedPtr ResourceManager::GetScene()
{
    return m_scene;
}
