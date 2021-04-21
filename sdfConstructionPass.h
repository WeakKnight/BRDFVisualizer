#pragma once
#include "Falcor.h"
#include "Utils/Sampling/SampleGenerator.h"

using namespace Falcor;

class sdfConstructionPass : public std::enable_shared_from_this <sdfConstructionPass>
{
public:
    using SharedPtr = std::shared_ptr<sdfConstructionPass>;

    static sdfConstructionPass::SharedPtr Create();
    void Execute(RenderContext* renderContext, uint dispatchGroupIndex, float3 m_Wi, float roughness, float metalic, float3 diffuse, float3 specular);
    

    inline Texture::SharedPtr GetGrid() { return m_Grid; }

private:
    void CreatePass();
    SampleGenerator::SharedPtr      m_SampleGenerator;
    

    ComputePass::SharedPtr          m_ComputePass;
    Texture::SharedPtr              m_Grid;

};
