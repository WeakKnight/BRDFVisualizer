#pragma once
#include <Falcor.h>
using namespace Falcor;

class RenderSettings
{
public:
    void OnGuiRender(Gui::Window& w);
    std::string scene;
    uint32_t width = 1600U;
    uint32_t height = 900U;
private:
};

void InitRenderSettings();
void DestroyRenderSettings();

RenderSettings* GetRenderSettings();

