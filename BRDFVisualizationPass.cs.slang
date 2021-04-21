#include "Utils/Math/MathConstants.slangh"
import SimpleBRDF;

cbuffer PerFrameCB
{
    uint2 ImageSize;
    int pMaxSteps;
    float pMaxDepth;
    float pTolerance;
    float3 pCameraOrigin;
    float3 pDirectionalLight;
    float3 pSkyboxColor;
    float3 pDiffuseColor;
    float3 pSpecularColor;
    float pSpecularExponent;
    float pAmbientCoefficient;

    uint3 gridSize;
    float3 mappedSize;
    

};

RWTexture2D<float4> gOutput;
Texture3D<float> m_Grid;
SamplerState _LinearSampler;

float sceneSDF(in float3 p) {
    
    float sdf1 = p.z;
    float sdf2;

    p = p + float3(5.0f,5.0f,0.0f);

    if(p.x < mappedSize.x && p.x > 0.0f && p.y < mappedSize.y && p.y > 0.0f &&p.z < mappedSize.z && p.z > 0.0f)
    {
        sdf2 = m_Grid.SampleLevel( _LinearSampler, p/mappedSize, 0);
    }    
    else
    {
        float3 clampedPos;
        clampedPos.x = (p.x, 0.0, mappedSize.x);
        clampedPos.y = (p.y, 0.0, mappedSize.y);
        clampedPos.z = (p.z, 0.0, mappedSize.z);
          
        //float3 q = abs(p) - mappedSize;
        //sdf2 = length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);

        sdf2 = m_Grid.SampleLevel( _LinearSampler, clampedPos/mappedSize, 0);

        
    }

    // Join 2 sdf
    //return sdf2;
    return min(sdf1,sdf2);
}

float4 color(float3 normal, float3 direction) {
    
    // Calculate lighting
    float3 halfway = -normalize(direction + pDirectionalLight);
    float specularBase = max(0.0, dot(normal, halfway));
    
    float3 diffuse = pDiffuseColor * max(0.0, dot(normal, -pDirectionalLight));
    float3 specular = pSpecularColor * pow(specularBase, pSpecularExponent);
    float3 ambient = pAmbientCoefficient * pDiffuseColor;
    
    // Combine components
    return float4(diffuse + specular + ambient, 1.0);
}

float march(float3 origin, float3 direction) 
{
    float depth = 0.0;
    
    for (int n = 0; n < pMaxSteps; n++) {
        
        // Get distance to nearest object
        float distance = sceneSDF(origin + depth * direction);
        
        // Return when distance is within tolerance
        if (distance <= pTolerance) {
            return depth;
        }
        
        // Increment ray
        depth += distance;
        
        // Return if depth exceeds max depth
        if (depth > pMaxDepth) {
            return pMaxDepth;
        }
    }
    
    // Return max depth after too many steps
    return pMaxDepth;
}

float3 ScreenSpaceDither( float2 vScreenPos )
{
    float3 vDither = float3( dot( float2( 171.0, 231.0 ), vScreenPos.xy ) );
    vDither.rgb = frac( vDither.rgb / float3( 103.0, 71.0, 97.0 ) );
    
    return vDither.rgb / 255.0;
}

[numthreads(16, 16, 1)]
void main(uint3 launchIndex : SV_DispatchThreadID)
{
    if(any(launchIndex.xy >= ImageSize))
    {
        return;
    }
    
    float3 cameraLookAt = normalize(float3(0.0f) - pCameraOrigin);
    float3 right = float3(1.0f, 0.0f, 0.0f);
    float3 up = cross(cameraLookAt,right);
  
    float3 planeOrigin = pCameraOrigin + cameraLookAt * 0.1f + 0.1f * up - 0.1f * right;
    
    float pixelXoffset = 0.2f/ ImageSize.x;
    float pixelYoffset = 0.2f/ ImageSize.y;
    planeOrigin = planeOrigin + 0.5f * pixelYoffset * (- up) + 0.5f * pixelXoffset * right;

    float3 rayDir = planeOrigin + launchIndex.x * pixelXoffset * right + launchIndex.y * pixelYoffset * (- up) - pCameraOrigin;
    rayDir = normalize(rayDir);

    // Calculate scene depth
    float depth = march(pCameraOrigin, rayDir);



    gOutput[launchIndex.xy] = float4(1.0f, 0.0f, 0.0f, 1.0f);
    
    // Render color
    if (depth < pMaxDepth) {
        gOutput[launchIndex.xy] = float4(float3(depth/pMaxDepth) + ScreenSpaceDither(launchIndex.xy), 1.0f);
    }
    else {
        gOutput[launchIndex.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }


    
}