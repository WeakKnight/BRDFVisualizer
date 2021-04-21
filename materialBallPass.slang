import Scene.Shading;
import Scene.Raster;
import SimpleBRDF;
import CommonHelpers;

cbuffer PerFrameCB
{
	float4x4 ViewProj;
	float3 CameraPos;
    float gRoughness;
    float gMetalic;
    float3 gDiffuse;
    float3 gSpecular;
};

struct VertexIn
{
    // Packed vertex attributes, see PackedStaticVertexData
    float3 pos                      : POSITION;
    float3 packedNormalTangent      : PACKED_NORMAL_TANGENT;

	StaticVertexData unpack()
    {
        PackedStaticVertexData v;
        v.position = pos;
        v.packedNormalTangent = packedNormalTangent;
        return v.unpack();
    }
};

struct VertexOut
{
	linear float3 normalW    : NORMAL;
    linear float3 posW       : POSW;
	
	float4 posSV : SV_POSITION;
};

float3 ACESFilmic(float3 x)
{
    x = x * 0.6;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

VertexOut vsMain(VertexIn vIn)
{
	StaticVertexData staticVertexData = vIn.unpack();

	VertexOut vOut;
	vOut.normalW = staticVertexData.normal;

	float4 transformedPos = float4(vIn.pos.xyz, 1.0);

	vOut.posSV = mul(transformedPos, ViewProj);
	vOut.posW = transformedPos.xyz;

	return vOut;
}


float4 psMain(VertexOut vOut): SV_Target0
{
	const float3 color = float3(0.3f, 0.4f, 0.8f);
    const float3 lightPos = float3(-0.2f, 9.6f, 7.8f);
    float3 viewDir = normalize(CameraPos - vOut.posW);
    float3 lightDir = normalize(lightPos - vOut.posW);
    float3 normal = vOut.normalW;


    SimpleBRDF simpleBRDF;
    simpleBRDF.setup(gDiffuse, gSpecular, gRoughness, gMetalic, lightDir, normal);
    

    float3 wi = ToLocal(lightDir, normal);
    float3 wo = ToLocal(viewDir, normal);
    float3 brdfValue = simpleBRDF.evalFr(wo, wi);

    return float4(brdfValue * color + float3(0.1f), 1.0);;
}
