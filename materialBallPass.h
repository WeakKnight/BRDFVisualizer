#pragma once
#include "Falcor.h"
#include "Utils/Sampling/SampleGenerator.h"

using namespace Falcor;


class MaterialBallPass : public std::enable_shared_from_this<MaterialBallPass>
{
public:
    using SharedPtr = std::shared_ptr<MaterialBallPass>;

    static MaterialBallPass::SharedPtr Create(uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void Execute(RenderContext* renderContext, float roughness, float metalic, float3 diffuse, float3 specular);
    //void OnGuiRender(Gui::Window& w);

    inline Texture::SharedPtr GetOutputTexture() { return m_Fbo->getColorTexture(0); }

private:
    void CreatePipeline();
    SampleGenerator::SharedPtr      m_SampleGenerator;

    GraphicsProgram::SharedPtr      m_Program = nullptr;
    GraphicsVars::SharedPtr         m_ProgramVars = nullptr;
    GraphicsState::SharedPtr        m_GraphicsState = nullptr;

    RasterizerState::SharedPtr      m_RasterizerState = nullptr;
    DepthStencilState::SharedPtr    m_DepthStencilState = nullptr;

    Fbo::SharedPtr                  m_Fbo;
    uint32_t m_Width;
    uint32_t m_Height;

};

