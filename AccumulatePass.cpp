#include "ResourceManager.h"
#include "RenderSettings.h"
#include "AccumulatePass.h"

AccumulatePass::SharedPtr AccumulatePass::Create(RenderContext* renderContext, uint width, uint height)
{
    AccumulatePass::SharedPtr result = AccumulatePass::SharedPtr(new AccumulatePass());
    result->Resize(width,height);
    result->mpScene = GetResourceManager()->GetScene();
    result->Init();
    return result;
}

void AccumulatePass::Execute(RenderContext* pRenderContext, Texture::SharedPtr inputTex)
{
    if (mFrameCount == 0)
    {
        pRenderContext->clearUAV(mpLastFrameSumLo->getUAV().get(), uint4(0));
        pRenderContext->clearUAV(mpLastFrameSumHi->getUAV().get(), uint4(0));
        pRenderContext->clearUAV(mOutputFrame->getUAV().get(), float4(0.f));
    }

    mpVars["PerFrameCB"]["gResolution"] = uint2(width,height);
    mpVars["PerFrameCB"]["gAccumCount"] = mFrameCount++;
    mpVars["gCurFrame"] = inputTex;

    uint3 numGroups = div_round_up(uint3(width, height, 1u), mpProgram->getReflector()->getThreadGroupSize());
    pRenderContext->dispatch(mpState.get(), mpVars.get(), numGroups);
}

void AccumulatePass::OnGuiRender(Gui::Window& w)
{
    if (auto postProcessingGroup = w.group("AccumulatePass"))
    {
        const std::string text = std::string("Frames accumulated ") + std::to_string(mFrameCount);
        w.text(text.c_str());
    }
}

void AccumulatePass::Resize(uint32_t width, uint32_t height)
{
    this->width = width;
    this->height = height;

    mOutputFrame = Texture::create2D(width, height, ResourceFormat::RGBA32Float, 1U, 1U, nullptr, ResourceBindFlags::UnorderedAccess | ResourceBindFlags::ShaderResource);
    mpLastFrameSumLo = Texture::create2D(width, height, ResourceFormat::RGBA32Uint, 1U, 1U, nullptr, ResourceBindFlags::UnorderedAccess | ResourceBindFlags::ShaderResource);
    mpLastFrameSumHi = Texture::create2D(width, height, ResourceFormat::RGBA32Uint, 1U, 1U, nullptr, ResourceBindFlags::UnorderedAccess | ResourceBindFlags::ShaderResource);
}

void AccumulatePass::Init()
{
    mpProgram = ComputeProgram::createFromFile("Samples/BRDFVisualizer/Accumulate.cs.slang", "accumulateDouble", Program::DefineList(), Shader::CompilerFlags::TreatWarningsAsErrors);
    mpVars = ComputeVars::create(mpProgram->getReflector());
    mpState = ComputeState::create();
    //mpLastFrameSum = Texture::create2D(width, height, ResourceFormat::RGBA32Float, 1U, 1U, nullptr, ResourceBindFlags::UnorderedAccess | ResourceBindFlags::ShaderResource);
  
    mpVars["gLastFrameSumLo"] = mpLastFrameSumLo;
    mpVars["gLastFrameSumHi"] = mpLastFrameSumHi;
    mpVars["gOutputFrame"] = mOutputFrame;
    mpState->setProgram(mpProgram);
}
