#include "materialBallPass.h"
#include "RenderSettings.h"
#include "ResourceManager.h"

MaterialBallPass::SharedPtr MaterialBallPass::Create(uint32_t width, uint32_t height)
{
    MaterialBallPass::SharedPtr result = MaterialBallPass::SharedPtr(new MaterialBallPass());

    result->Resize(width, height);
    result->CreatePipeline();

    return result;
}

void MaterialBallPass::Resize(uint32_t width, uint32_t height)
{
    Fbo::Desc desc;
    desc.setSampleCount(0);
    desc.setColorTarget(0, Falcor::ResourceFormat::RGBA16Float);    // outputTexture
    desc.setDepthStencilTarget(ResourceFormat::D32Float);
    m_Fbo = Fbo::create2D(width, height, desc);

    m_Width = width;
    m_Height = height;
}

void MaterialBallPass::Execute(RenderContext* pRenderContext)
{
    Camera::SharedPtr camera = GetResourceManager()->GetScene()->getCamera();
    //camera->setFrameWidth(m_Width);
    //camera->setFrameHeight(m_Height);
    camera->setAspectRatio((float)m_Width / (float)m_Height);
    m_GraphicsState->setFbo(m_Fbo);

    auto cb = m_ProgramVars["PerFrameCB"];
    cb["ViewProj"] = camera->getViewProjMatrix();
    cb["CameraPos"] = camera->getPosition();
    Scene::RenderFlags renderFlags = Scene::RenderFlags::UserRasterizerState;

    const float4 clearColor(0.0f, 0.0f, 0.0f, 1);
    pRenderContext->clearFbo(m_Fbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    GetResourceManager()->GetScene()->rasterize(pRenderContext, m_GraphicsState.get(), m_ProgramVars.get(), renderFlags);
    
}

void MaterialBallPass::CreatePipeline()
{
    Program::Desc desc;
    desc.addShaderLibrary("Samples/BRDFVisualizer/materialBallPass.slang").vsEntry("vsMain").psEntry("psMain");
    desc.setShaderModel("6_2");
    m_Program = GraphicsProgram::create(desc);

    

    RasterizerState::Desc rasterStateDesc;
    rasterStateDesc.setCullMode(RasterizerState::CullMode::Back);
    m_RasterizerState = RasterizerState::create(rasterStateDesc);

    DepthStencilState::Desc depthStancilDesc;
    depthStancilDesc.setDepthFunc(ComparisonFunc::Less).setDepthEnabled(true);
    m_DepthStencilState = DepthStencilState::create(depthStancilDesc);

    m_Program->addDefines(GetResourceManager()->GetScene()->getSceneDefines());
    m_ProgramVars = GraphicsVars::create(m_Program->getReflector());

    m_GraphicsState = GraphicsState::create();

    m_GraphicsState->setRasterizerState(m_RasterizerState);
    m_GraphicsState->setDepthStencilState(m_DepthStencilState);

    m_GraphicsState->setProgram(m_Program);
    
}
