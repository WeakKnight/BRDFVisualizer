#pragma once
#include "Falcor.h"

using namespace Falcor;

class AccumulatePass : public std::enable_shared_from_this<AccumulatePass>
{
public:
    using SharedPtr = std::shared_ptr<AccumulatePass>;

    static AccumulatePass::SharedPtr Create(RenderContext* renderContext, uint width, uint height);
    void Execute(RenderContext* renderContext, Texture::SharedPtr inputTex);
    void OnGuiRender(Gui::Window& w);
    void Resize(uint32_t width, uint32_t height);
    inline Texture::SharedPtr GetOutputTexture() { return mOutputFrame; }

    void Clear() { mFrameCount = 0; }

private:
    uint width;
    uint height;
    void Init();

    Scene::SharedPtr            mpScene;
    ComputeProgram::SharedPtr  mpProgram;
    ComputeVars::SharedPtr      mpVars;                         ///< Program variables.
    ComputeState::SharedPtr     mpState;

    uint32_t                    mFrameCount;                ///< Number of accumulated frames. This is reset upon changes.

    Texture::SharedPtr          mpLastFrameSumLo;               ///< Last frame running sum (lo bits). Used in Double mode.
    Texture::SharedPtr          mpLastFrameSumHi;               ///< Last frame running sum (hi bits). Used in Double mode.
    Texture::SharedPtr          mOutputFrame;
};
