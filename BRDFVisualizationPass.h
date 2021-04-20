#pragma once
#include "Falcor.h"
#include "Utils/Sampling/SampleGenerator.h"

using namespace Falcor;

class BRDFVisualizationPass : public std::enable_shared_from_this <BRDFVisualizationPass>
{
public:
        using SharedPtr = std::shared_ptr<BRDFVisualizationPass>;

        static BRDFVisualizationPass::SharedPtr Create(uint32_t width, uint32_t height);
        void Resize(uint32_t width, uint32_t height);
        void Execute(Texture::SharedPtr m_Grid,RenderContext* renderContext);
        void CreatePass();

        inline Texture::SharedPtr GetOutputTexture() { return m_VisualizationResult; }

private:
    SampleGenerator::SharedPtr      m_SampleGenerator;
    Sampler::SharedPtr LinearSampler;

    ComputePass::SharedPtr          m_ComputePass;
    Texture::SharedPtr              m_VisualizationResult;


    uint32_t m_width;
    uint32_t m_height;
};
