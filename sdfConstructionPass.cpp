#include"sdfConstructionPass.h"

sdfConstructionPass::SharedPtr sdfConstructionPass::Create()
{
    sdfConstructionPass::SharedPtr result = sdfConstructionPass::SharedPtr(new sdfConstructionPass());

    result->CreatePass();

    return result;
}

void sdfConstructionPass::Execute(RenderContext* pRenderContext, uint dispatchGroupIndex, float3 m_Wi, float roughness, float metalic, float3 diffuse, float3 specular)
{
    auto cb = m_ComputePass["PerFrameCB"];
    cb["gridSize"] = uint3(256,256,256);
    cb["mappedSize"] = float3(10.0f,10.0f,10.0f);
    cb["gInputDirection"] = m_Wi;
    cb["gRoughness"] = roughness;
    cb["gMetalic"] = metalic;
    cb["gDiffuse"] = diffuse;
    cb["gSpecular"] = specular;


    for (int i = dispatchGroupIndex * 256; i < dispatchGroupIndex * 256 + 256; i++)
    {
        cb["randomIndex"] = uint(i);
        m_ComputePass->execute(pRenderContext, uint3(256, 256, 256));
        pRenderContext->flush(true);
    }
    

    
}

void sdfConstructionPass::CreatePass()
{
    m_SampleGenerator = SampleGenerator::create(SAMPLE_GENERATOR_UNIFORM);

    auto defineList = Program::DefineList();
    defineList.add(m_SampleGenerator->getDefines());

    Program::Desc desc;
    desc.addShaderLibrary("Samples/BRDFVisualizer/sdfConstructionPass.cs.slang").csEntry("main").setShaderModel("6_5");
    desc.setCompilerFlags(Falcor::Shader::CompilerFlags::GenerateDebugInfo | Falcor::Shader::CompilerFlags::TreatWarningsAsErrors);

    m_ComputePass = ComputePass::create(desc, defineList);

    m_Grid = Texture::create3D(256, 256, 256 , ResourceFormat::R16Float, 1,nullptr, Resource::BindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource);

    m_ComputePass["gOutput"] = m_Grid;


}
