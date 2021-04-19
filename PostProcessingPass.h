#pragma once
#include "Falcor.h"

using namespace Falcor;

class PostProcessingPass : public std::enable_shared_from_this<PostProcessingPass>
{
public:
    using SharedPtr = std::shared_ptr<PostProcessingPass>;
    static PostProcessingPass::SharedPtr Create();
    void Execute(RenderContext* renderContext, Texture::SharedPtr shadingResult, Fbo::SharedPtr targetFbo);
    void OnGuiRender(Gui::Window& w);

private:
    FullScreenPass::SharedPtr m_fullScreenPass;
    float m_exposure = 0.0f;
};

