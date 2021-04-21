/***************************************************************************
 # Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#include "BRDFVisualizer.h"
#include "materialBallPass.h"
#include "ResourceManager.h"
#include "RenderSettings.h"
#include "BRDFVisualizationPass.h"
#include "sdfConstructionPass.h"

uint32_t mSampleGuiWidth = 250;
uint32_t mSampleGuiHeight = 200;
uint32_t mSampleGuiPositionX = 20;
uint32_t mSampleGuiPositionY = 40;
static bool generated = false;
static int dispatchTimes = 360;
static int dispatchCounter = 0;
static bool parameterChanged = true;

void BRDFVisualizer::onGuiRender(Gui* pGui)
{
    {
        Gui::Window w(pGui, "BRDF Model", { 350, 300 }, {0, 0});
        Gui::DropdownList BRDFTypes;
        //BRDFTypes.push_back({ 0, "Phong" });
        //BRDFTypes.push_back({ 1, "Blinn-Phong" });
        BRDFTypes.push_back({ 0, "UE4" });
        if (w.dropdown("BRDF Models", BRDFTypes, m_BRDFModel))
        {
        }
    }
    {
        Gui::Window w(pGui, "Local Geometry Params", { 350, 600 }, {0, 240});
        if (w.direction("wi", m_Wi)|| w.slider("roughness", m_roughness, 0.0f, 1.0f) || w.slider("metallic", m_metallic, 0.0f, 1.0f) || w.rgbColor("diffuse", m_D) || w.rgbColor("specular", m_S))
        {
            parameterChanged = true;
        }
    }

    {
        Gui::Window w(pGui, "Main View", { 950, 900 }, {280, 0});
        w.image("View", m_BRDFVisualizationPass->GetOutputTexture(), {920, 120});
        
    }

    {
        Gui::Window w(pGui, "Scene Settings", { 302, 600 }, { 1039, 0 });
        GetResourceManager()->GetScene()->renderUI(w);
    }

    {
        Gui::Window w(pGui, "Material Preview", { 303, 300 }, {1039, 480});
        w.image("Preview", m_materialBallPass->GetOutputTexture(), { 280, 0 });
    }
}

void BRDFVisualizer::onLoad(RenderContext* pRenderContext)
{
    InitResourceManager();
    //m_MainViewTex = Texture::create2D(900, 835, ResourceFormat::RGBA16Float, 1, 1, nullptr, ResourceBindFlags::AllColorViews);
    m_PreviewTex = Texture::create2D(600, 540, ResourceFormat::RGBA16Float, 1, 1, nullptr, ResourceBindFlags::AllColorViews);

    m_materialBallPass = MaterialBallPass::Create(900, 835);
    m_BRDFVisualizationPass = BRDFVisualizationPass::Create(900, 835);
    m_sdfConstructionPass = sdfConstructionPass::Create();
}

void BRDFVisualizer::onFrameRender(RenderContext* pRenderContext, const Fbo::SharedPtr& pTargetFbo)
{
    const float4 clearColor(0.38f, 0.52f, 0.10f, 1);
    pRenderContext->clearFbo(pTargetFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    pRenderContext->clearTexture(m_PreviewTex.get());
    

    if (parameterChanged)
    {
        parameterChanged = false;
        dispatchCounter = 0;
        generated = false;
        m_materialBallPass->Execute(pRenderContext, m_roughness, m_metallic, m_D, m_S);
    }

    if (!generated)
    {  
        m_sdfConstructionPass->Execute(pRenderContext, dispatchCounter, m_Wi, m_roughness, m_metallic, m_D, m_S);
        dispatchCounter++;
        if(dispatchCounter >= dispatchTimes)
        generated = true;

    }
    m_BRDFVisualizationPass->Execute(m_sdfConstructionPass->GetGrid(),pRenderContext);
    pRenderContext->flush(true);

    //m_MainViewTex = m_materialBallPass->GetOutputTexture();
}

void BRDFVisualizer::onShutdown()
{
}

bool BRDFVisualizer::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return false;
}

bool BRDFVisualizer::onMouseEvent(const MouseEvent& mouseEvent)
{
    return false;
}

void BRDFVisualizer::onHotReload(HotReloadFlags reloaded)
{
}

void BRDFVisualizer::onResizeSwapChain(uint32_t width, uint32_t height)
{
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    InitRenderSettings();

    BRDFVisualizer::UniquePtr pRenderer = std::make_unique<BRDFVisualizer>();
    SampleConfig config;
    config.windowDesc.title = "BRDF Visualizer";
    config.windowDesc.resizableWindow = false;
    config.windowDesc.width = 1600;
    config.windowDesc.height = 900;
    Sample::run(config, pRenderer);
    return 0;
}
