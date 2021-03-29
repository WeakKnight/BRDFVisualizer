import Scene.Shading;
import Scene.Raster;

cbuffer PerFrameCB
{
	float4x4 ViewProj;
	float3 CameraPos;
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
	const float3 color = 0.5f;
    const float3 lightPos = float3(-0.2f, 9.6f, 7.8f);

    float3 ambient = 0.05f * color;

    float3 lightDir = normalize(lightPos - vOut.posW);
    float3 normal = vOut.normalW;

    float diff = max(dot(lightDir, normal), 0.0);
    float3 diffuse = diff * color;

    // specular
    float3 viewDir = normalize(CameraPos - vOut.posW);
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    float3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
     
    float3 specular = float3(0.3) * spec;
    
	
    return float4(ambient + diffuse + specular, 1.0);;
}
