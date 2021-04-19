#include "RenderSettings.h"
#include "ResourceManager.h"
#include <assert.h>
#include "Json.hpp"
#include "Utils.h"
static RenderSettings* s_renderSettings = nullptr;

void InitRenderSettings()
{
    assert(s_renderSettings == nullptr);
    s_renderSettings = new RenderSettings();

    std::string content = Utils::ReadFile("settings.json");
    nlohmann::json jobject = nlohmann::json::parse(content);
    
    s_renderSettings->scene = jobject["scene"];
    s_renderSettings->width = jobject["width"];
    s_renderSettings->height = jobject["height"];
}

void DestroyRenderSettings()
{
    assert(s_renderSettings != nullptr);
    delete s_renderSettings;
}

RenderSettings* GetRenderSettings()
{
    assert(s_renderSettings != nullptr);
    return s_renderSettings;
}

void RenderSettings::OnGuiRender(Gui::Window& w)
{
   
}
