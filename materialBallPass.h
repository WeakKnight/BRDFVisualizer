#pragma once
#include "Falcor.h"

using namespace Falcor;


class materialBallPass : public std::enable_shared_from_this<materialBallPass>
{
public:
    using SharedPtr = std::shared_ptr<materialBallPass>;

    static materialBallPass::SharedPtr Create(uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void Execute(RenderContext* renderContext);
    //void OnGuiRender(Gui::Window& w);

    inline Texture::SharedPtr GetOutputTexture() { return m_Fbo->getColorTexture(0); }

private:
    void CreatePipeline();

    GraphicsProgram::SharedPtr      m_Program = nullptr;
    GraphicsVars::SharedPtr         m_ProgramVars = nullptr;
    GraphicsState::SharedPtr        m_GraphicsState = nullptr;

    RasterizerState::SharedPtr      m_RasterizerState = nullptr;
    DepthStencilState::SharedPtr    m_DepthStencilState = nullptr;

    Scene::SharedPtr m_scene = nullptr;

    Fbo::SharedPtr                  m_Fbo;
    uint32_t m_Width;
    uint32_t m_Height;

};

