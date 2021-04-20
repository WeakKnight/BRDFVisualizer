import Utils.Helpers;
#include "Utils/Math/MathConstants.slangh"
import Scene.ShadingData;

#ifndef MY_NV_INTERLOCK_ON
// Setup NvApi. We need this for fp32 atomics.
#define MY_NV_INTERLOCK_ON
#define NV_SHADER_EXTN_SLOT u63
#define NV_SHADER_EXTN_REGISTER_SPACE space0
#include "NVAPI/nvHLSLExtns.h"
#endif 

struct RenderingData
{
    float3  posW;                   ///< Shading hit position in world space
    float3  V;                      ///< Direction to the eye at shading hit
    float3  L;
    float3  N;                      ///< Shading normal at shading hit

    // Primitive data
    float3  faceN;                  ///< Face normal in world space, always on the front-facing side.
    float3  diffuse;                ///< Diffuse albedo.
    float   opacity;
    bool    valid;
    float3  specular;               ///< Specular albedo.
    float   linearRoughness;        ///< This is the original roughness, before re-mapping.
    float3  emissive;
    float   metallic;
}

struct PackedBoundingBox
{
    float3 minPoint;
    float3 maxPoint;
    float Pad0;
    float Pad1;
};

RenderingData ToRenderingData(ShadingData sd, bool lightTracing = false)
{
    RenderingData result;
    result.posW = sd.posW;
    
    if (lightTracing)
    {   
        result.L = sd.V;
    }

    result.V = sd.V;
    result.N = sd.N;

    result.faceN = sd.faceN;
    result.diffuse = sd.diffuse;
    result.opacity = sd.opacity;
    result.specular = sd.specular;
    result.linearRoughness = sd.linearRoughness;
    result.emissive = sd.emissive;
    result.metallic = sd.metallic;

    result.valid = true;
    
    return result;
}

float3 ComputeRayOrigin(float3 pos, float3 normal)
{
    const float origin = 1.f / 32.f;
    const float fScale = 1.f / 65536.f;
    const float iScale = 256.f;

    // Per-component integer offset to bit representation of fp32 position.
    int3 iOff = int3(normal * iScale);
    float3 iPos = asfloat(asint(pos) + (pos < 0.f ? -iOff : iOff));

    // Select per-component between small fixed offset or above variable offset depending on distance to origin.
    float3 fOff = normal * fScale;
    return abs(pos) < origin ? pos + fOff : iPos;
}

// Workaround for isnan() not working in slang.
bool isNaN(float f)
{
    uint u = asuint(f) & ~0x80000000u; // clear out the sign bit
    return (u > 0x7F800000);           // greater than Inf is NaN
}

float2 Plastic(uint index)
{
    static float p1 = 0.7548776662466927;
    static float p2 = 0.5698402909980532;
    float2 result;
    result.x = fmod(p1 * float(index), 1);
    result.y = fmod(p2 * float(index), 1);
    return result;
}

float EvalMIS(float p0, float p1)
{   
    return p0 * p0 / (p0 * p0 + p1 * p1);
}

float3 ToLocal(float3 v, float3 N)
{
    float3 T = 0.0;
    if (abs(N.x) > 0.1)
    {
        T = normalize(cross(float3(0.0, 1.0, 0.0), N));
    }
    else
    {
        T = normalize(cross(float3(1.0, 0.0, 0.0), N));
    }
    float3 B = cross(N, T);

    return float3(dot(v, T), dot(v, B), dot(v, N));
}

float3 FromLocal(float3 v, float3 N)
{
    float3 T = 0.0;
    if (abs(N.x) > 0.1)
    {
        T = normalize(cross(float3(0.0, 1.0, 0.0), N));
    }
    else
    {
        T = normalize(cross(float3(1.0, 0.0, 0.0), N));
    }
    float3 B = cross(N, T);

    return v.x * T + v.y * B + v.z * N;
}

float2 SphericalCoordinate2UV(float3 dir)
{
    float3 p = normalize(dir);
    float2 uv;
    p /= dot(float3(1.0f), abs(p));
    // mapping to [0;1]ˆ2 texture space
    return 0.5 * float2(p.x - p.y + 1.0, -p.x - p.y + 1.0);
}

float3 WorldToBarycentric(float3 A, float3 B, float3 C, float3 P)
{
    float3 PA = A - P;
    float3 PB = B - P;
    float3 PC = C - P;

    float AreaA = length(cross(PB, PC));
    float AreaB = length(cross(PC, PA));
    float AreaC = length(cross(PA, PB));

    float Area = length(cross(B - A, C - A));

    return float3(AreaA / Area, AreaB / Area, AreaC / Area);
}

float2 RandomPointInCircle(float radius, float u, float v)
{
	// generate a random value between 0 to Radius as the value of Cumulative Distribution Function
	float S = u;
	// S = r2 / R2, choose r based on F
	float r = sqrt(S) * radius;
	float theta = v * M_PI * 2.0f;

	float x = r * cos(theta);
	float y = r * sin(theta);

	return float2(x, y);
}

int2 SpatialSample(uint frameIndex, float u, uint ind)
{
    float2 uv = Plastic(frameIndex + u * 731 + ind);
    float2 res = RandomPointInCircle(10.0f, uv.x, uv.y);
    return res;
}

float3 UniformConeSampling(float2 XY, float cosThetaMax, out float pdf)
{
    float cosTheta = (1 - XY.x) + XY.x * cosThetaMax;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    float phi = XY.y * M_2_PI;

    pdf = 1.0 / (2.0 * M_PI * (1.0 - cosThetaMax));

    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float3 CosineWeightedSampling(float2 XY, float3 N)
{
    float3 w = N;
    float3 u = 0.0;
    if (abs(w.x) > 0.1)
    {
        u = normalize(cross(float3(0.0, 1.0, 0.0), w));
    }
    else
    {
        u = normalize(cross(float3(1.0, 0.0, 0.0), w));
    }
    float3 v = cross(w, u);
    float r1 = 2.0 * M_PI * XY.x;
    float r2 = XY.y;
    float r2s = sqrt(r2);
    float3 dir = normalize((u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1.0 - r2)));

    return dir;
}

float3 CosineWeightedSampling(float2 XY, float3 N, out float pdf)
{
    float3 w = N;
    float3 u = 0.0;
    if (abs(w.x) > 0.1)
    {
        u = normalize(cross(float3(0.0, 1.0, 0.0), w));
    }
    else
    {
        u = normalize(cross(float3(1.0, 0.0, 0.0), w));
    }
    float3 v = cross(w, u);
    float r1 = 2.0 * M_PI * XY.x;
    float r2 = XY.y;
    float r2s = sqrt(r2);
    float3 dir = normalize((u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1.0 - r2)));

    pdf = dot(N, dir) / M_PI;

    return dir;
}

float3 SamplingDiskConcentric(float2 u, float3 vplNormal, float3 vplPos, float radius)
{   
    // float2 offset;
    // //sampling a unit disk
    // u = 2.f * u - 1.f;
    // if (u.x == 0.f && u.y == 0.f) 
    // return vplPos;
    // float phi, r;
    // if (abs(u.x) > abs(u.y))
    // {
    //     r = u.x;
    //     phi = (u.y / u.x) * M_PI_4;
    // }
    // else
    // {
    //     r = u.y;
    //     phi = M_PI_2 - (u.x / u.y) * M_PI_4;
    // }
    // offset = r * float2(cos(phi), sin(phi));

    float2 p;
    float r = sqrt(u.x);
    float phi = M_2PI * u.y;
    p.x = r * cos(phi);
    p.y = r * sin(phi);

    // map point
    float3 tangent = 0.0f;
    if (abs(vplNormal.x) > 0.1)
    {
        tangent = normalize(cross(float3(0.0, 1.0, 0.0), vplNormal));
    }
    else
    {
        tangent = normalize(cross(float3(1.0, 0.0, 0.0), vplNormal));
    }
    float3 bitangent = cross(vplNormal, tangent);

    return vplPos + tangent * p.x * radius + bitangent * p.y * radius;
}

/*
// Get our uniform random numbers
	float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

	// Get an orthonormal basis from the normal
	float3 B = getPerpendicularVector(hitNorm);
	float3 T = cross(B, hitNorm);

	// GGX NDF sampling
	float a2 = roughness * roughness;
	float cosThetaH = sqrt(max(0.0f, (1.0-randVal.x)/((a2-1.0)*randVal.x+1) ));
	float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
	float phiH = randVal.y * M_PI * 2.0f;

	// Get our GGX NDF sample (i.e., the half vector)
	return T * (sinThetaH * cos(phiH)) +
           B * (sinThetaH * sin(phiH)) +
           hitNorm * cosThetaH;
*/

bool GGXSampling(float2 Xi, float3 N, float3 wo, float roughness, out float3 wi, out float pdf)
{
    float a2 = roughness * roughness * roughness * roughness;
    float Phi = 2 * M_PI * Xi.x;
    float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a2 - 1) * Xi.y ) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );
    float3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;
    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize( cross( UpVector, N ) );
    float3 TangentY = cross( N, TangentX );
    // Tangent to world space
    H = TangentX * H.x + TangentY * H.y + N * H.z;

    wi = normalize(2.f * dot(wo, H) * H - wo);
    if (min(dot(N,wo), dot(N,wi)) < 1e-6f) 
    {
        return false;
    }

    float woDotH = dot(wo, H);
    pdf = DistributionGGX(N, H, roughness) * dot(N,H) / (4 * woDotH);
    
    return true;    
}

float DistributionGGX (float3 N, float3 H, float roughness)
{
    float a2    = roughness * roughness * roughness * roughness;
    float NdotH = max (dot (N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (M_1_PI * denom * denom);
}       

float3 EvalFresnelSchlick(float3 f0, float3 f90, float cosTheta)
{
    return f0 + (f90 - f0) * pow(max(1 - cosTheta, 0), 5); // Clamp to avoid NaN if cosTheta = 1+epsilon
}

float EvalFresnelSchlick(float f0, float f90, float cosTheta)
{
    return f0 + (f90 - f0) * pow(max(1 - cosTheta, 0), 5); // Clamp to avoid NaN if cosTheta = 1+epsilon
}

float3 EvalFresnelSchlick(float3 f0, float cosTheta)
{
    return f0 + (1.0 - f0) * pow(max(1 - cosTheta, 0), 5); // Clamp to avoid NaN if cosTheta = 1+epsilon
}

inline float3 EvalDiffuse(float3 albedo, float3 specular)
{
    return (1.0 - specular) * albedo / M_PI;
}

inline float3 EvalSpecular(float3 L, float3 V, float3 R, float3 N, float3 specular, float specPower)
{
    float energyConservationScaler = (specPower + 2.0) / M_2_PI;
    float NDotL = saturate(dot(N, L));

    float3 fresnel = EvalFresnelSchlick(specular, NDotL);

    return energyConservationScaler * fresnel * pow(saturate(dot(R, V)), specPower);
}

float ConvertSpecPowerToRoughness(float specPower)
{
    return clamp(sqrt(2.0f / (specPower + 2.0f)), 0.f, 1.f);
}

float ConvertRoughnessToSpecPower(float roughness)
{
    return 2.0f / max(0.02f, roughness * roughness) - 2.0f; 
}

inline float3 EvalPhongShading(float3 Le, float3 L, float3 V, float3 N, float3 albedo, float3 specular, float roughness)
{
    float3 R = reflect(-V, N);
    float specPower = ConvertRoughnessToSpecPower(roughness);
    float NDotL = saturate(dot(N, L));
    
    // float3 specularResult = EvalSpecular(L, V, R, N, specular, specPower);
    float3 albedoResult = EvalDiffuse(albedo, specular);

    // return (albedoResult + specularResult) * Le * NDotL;
    return (albedoResult) * Le * NDotL;
}

float SimpliedCemAttenuation(float d, float r)
{
    const float scaler = d * d + r * r;
    return 2.0 / (scaler + d * sqrt(scaler)); 
}
