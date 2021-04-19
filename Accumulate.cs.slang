/** Temporal accumulation render pass.

    There are entry points for each of the three supported accumulation modes.
    Note that for the compensated summation mode the shader _must_ be compiled
    for precise floating-point operations (no reordering).

    In all modes, the shader writes the current accumulated average to the
    output texture. The intermediate buffers are internal to the pass.
*/
cbuffer PerFrameCB
{
    uint2   gResolution;
    uint    gAccumCount;
}

// Input data to accumulate and accumulated output.
Texture2D<float4>   gCurFrame;
RWTexture2D<float4> gOutputFrame;

// Last frame data, format depends on accumulation mode.
RWTexture2D<float4> gLastFrameSum;

RWTexture2D<uint4>  gLastFrameSumLo;    // If mode is Double
RWTexture2D<uint4>  gLastFrameSumHi;    // If mode is Double



// /** Single precision standard summation.
// */
// [numthreads(16, 16, 1)]
// void accumulateSingle(uint3 dispatchThreadId : SV_DispatchThreadID)
// {
//     if (any(dispatchThreadId.xy >= gResolution)) return;
//     const uint2 pixelPos = dispatchThreadId.xy;
//     const float4 curColor = gCurFrame[pixelPos];

//     // Fetch previous sum and compute the new sum.
//     float4 sum = gLastFrameSum[pixelPos] + curColor;
//     float4 output = sum / (gAccumCount + 1);

//     gLastFrameSum[pixelPos] = sum;
//     gOutputFrame[pixelPos] = output;
// }


/** Double precision standard summation.
*/
[numthreads(16, 16, 1)]
void accumulateDouble(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (any(dispatchThreadId.xy >= gResolution)) return;
    const uint2 pixelPos = dispatchThreadId.xy;
    const float4 curColor = gCurFrame[pixelPos];

    // Fetch the previous sum in double precision.
    // There is no 'double' resource format, so the bits are stored in two uint4 textures.
    uint4 sumLo = gLastFrameSumLo[pixelPos];
    uint4 sumHi = gLastFrameSumHi[pixelPos];

    double sum[4];
    float4 output;

    for (int i = 0; i < 4; i++)
    {
        sum[i] = asdouble(sumLo[i], sumHi[i]);
        sum[i] += (double)curColor[i];
        asuint(sum[i], sumLo[i], sumHi[i]);
        output[i] = (float)(sum[i] / (double)(gAccumCount + 1));
    }

    gLastFrameSumLo[pixelPos] = sumLo;
    gLastFrameSumHi[pixelPos] = sumHi;

    gOutputFrame[pixelPos] = output;
}
