#include "Utils/Math/MathConstants.slangh"
import SimpleBRDF;
import Utils.Math.MathHelpers;
import CommonHelpers;

cbuffer PerFrameCB
{
    uint3 gridSize;
    float3 mappedSize;
    float3 gInputDirection;
    uint randomIndex;
    float gRoughness;
    float gMetalic;
    float3 gDiffuse;
    float3 gSpecular;
    

};

RWTexture3D<float> gOutput;

float sdSphere( float3 p, float s )
{
  return length(p)-s;
}

float sdCapsule( float3 p, float3 a, float3 b, float r )
{
  float3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

[numthreads(8, 8, 8)]
void main(uint3 launchIndex : SV_DispatchThreadID)
{
    if(any(launchIndex.xyz >= gridSize))
    {
        return;
    }
    float3 cellSize = mappedSize / gridSize;
    float3 pos = launchIndex.xyz * cellSize + 0.5f * cellSize;
    
    float2 xy = Plastic(randomIndex);
    float3 dir = sample_sphere(xy);
    
    SimpleBRDF simpleBRDF;
    simpleBRDF.setup(gDiffuse, gSpecular, gRoughness, gMetalic, gInputDirection, float3(0.0f, 0.0f, 1.0f));
    float pdf = simpleBRDF.evalPdf(dir,gInputDirection);

    //if(pdf > 0.1f) pdf = 0.0f;

    float3 a = float3(5.0,5.0,0.0);
    float3 b = a + dir * pdf * 20.0;

    float dis = sdCapsule(pos, a, b , 0.05);
    
    // float radius = min(mappedSize.x, min(mappedSize.y, mappedSize.z)) / 3.0f;

    // float3 center = mappedSize / 2.0f;

    // pos = pos - center;

    // float dis = sdSphere(pos , radius);
    
    if(randomIndex > 1)
    gOutput[launchIndex.xyz] = min(dis, gOutput[launchIndex.xyz]); 
    else
    gOutput[launchIndex.xyz] = dis;  

}    