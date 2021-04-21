#include "BRDFVisualizationPass.h"

BRDFVisualizationPass::SharedPtr BRDFVisualizationPass::Create(uint32_t width, uint32_t height)
{
    BRDFVisualizationPass::SharedPtr result = BRDFVisualizationPass::SharedPtr(new BRDFVisualizationPass());

    result->Resize(width, height);
    result->CreatePass();

    return result;
}

void BRDFVisualizationPass::Resize(uint32_t width, uint32_t height)
{

    m_VisualizationResult = Texture::create2D(width, height, ResourceFormat::RGBA16Float, 1, 1, nullptr, Resource::BindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource);
    m_width = width;
    m_height = height;
}


void BRDFVisualizationPass::Execute(Texture::SharedPtr m_Grid, RenderContext* pRenderContext)
{
    auto cb = m_ComputePass["PerFrameCB"];
    cb["pMaxSteps"] = (uint)1000;
    cb["pMaxDepth"] = 300.0f;
    cb["pTolerance"] = 0.0001f;
    cb["pCameraOrigin"] = float3(0.0f, 8.0f, 4.0f);
    cb["pDirectionalLight"] = glm::normalize(float3(5.0f, -5.0f, -5.0f));
    cb["pSkyboxColor"] = float3(0.0f);
    cb["pDiffuseColor"] = float3(0.4f, 0.05f, 0.05f);
    cb["pSpecularColor"] = float3(0.6f, 0.5f, 0.5f);
    cb["pSpecularExponent"] = 8.0f;
    cb["pAmbientCoefficient"] = 0.1f;
    cb["ImageSize"] = uint2(m_width, m_height);

    cb["gridSize"] = uint3(256, 256, 256);
    cb["mappedSize"] = float3(10.0f, 10.0f, 10.0f);

    m_ComputePass["m_Grid"] = m_Grid;
    m_ComputePass["gOutput"] = m_VisualizationResult;
    
    m_ComputePass->execute(pRenderContext, uint3(m_width, m_height, 1));
}

void BRDFVisualizationPass::CreatePass()
{
    m_SampleGenerator = SampleGenerator::create(SAMPLE_GENERATOR_UNIFORM);

    auto defineList = Program::DefineList();
    defineList.add(m_SampleGenerator->getDefines());
    
    Program::Desc desc;
    desc.addShaderLibrary("Samples/BRDFVisualizer/BRDFVisualizationPass.cs.slang").csEntry("main").setShaderModel("6_5");
    desc.setCompilerFlags(Falcor::Shader::CompilerFlags::GenerateDebugInfo | Falcor::Shader::CompilerFlags::TreatWarningsAsErrors);

    m_ComputePass = ComputePass::create(desc, defineList);

    Sampler::Desc LinearSamplerDesc;
    LinearSamplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    LinearSampler = Sampler::create(LinearSamplerDesc);

    m_ComputePass->getVars()->setSampler("_LinearSampler", LinearSampler);

}
