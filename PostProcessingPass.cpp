#include "PostProcessingPass.h"
#include "ResourceManager.h"
#include "RenderSettings.h"

PostProcessingPass::SharedPtr PostProcessingPass::Create()
{
    RenderSettings* renderSettings = GetRenderSettings();
    PostProcessingPass::SharedPtr postProcessingPass = PostProcessingPass::SharedPtr(new PostProcessingPass());
    postProcessingPass->m_fullScreenPass = FullScreenPass::create("Samples/BRDFVisualizer/PostProcessingPass.ps.slang");

    return postProcessingPass;
}

void PostProcessingPass::Execute(RenderContext* renderContext, Texture::SharedPtr shadingResult, Fbo::SharedPtr targetFbo)
{
    m_fullScreenPass->getVars()->setTexture("_shadingResult", shadingResult);
    m_fullScreenPass->getVars()->setSampler("_pointSampler", GetResourceManager()->PointSampler);

    m_fullScreenPass["PerFrameCB"]["exposure"] = (float)pow(2.0, m_exposure);

    m_fullScreenPass->execute(renderContext, targetFbo);
}

void PostProcessingPass::OnGuiRender(Gui::Window& w)
{
    if (auto postProcessingGroup = w.group("PostProcessing"))
    {
        w.slider("Exposure", m_exposure, -10.0f, 10.0f);
    }
}
