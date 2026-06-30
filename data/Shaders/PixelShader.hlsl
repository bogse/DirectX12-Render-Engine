struct PixelShaderInput
{
    float4 Position   : SV_Position;
    float3 PositionVS : POSITION;
    float3 Normal     : NORMAL;
    float4 Color      : COLOR;
    float2 TexCoord   : TEXCOORD;
};

struct Material
{
    float4 Emissive;
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float  SpecularPower;
    float3 Padding;
};

struct DirectionalLight
{
    float4 DirectionWS;
    float4 DirectionVS;
    float4 Color;
};

struct LightProperties
{
    uint NumDirectionalLights;
};

struct LightResult
{
    float4 Diffuse;
    float4 Specular;
};

struct PipelineOptions
{
    int EnableTexture;
    int EnableMips;
};

ConstantBuffer<Material> MaterialCB : register(b0, space1);
ConstantBuffer<LightProperties> LightPropertiesCB : register(b1);

StructuredBuffer<DirectionalLight> DirectionalLights : register(t0);

Texture2D DiffuseTexture : register(t1);
SamplerState LinearRepeatSampler : register(s0);

ConstantBuffer<PipelineOptions> PipelineOptionsCB : register(b0, space9);

float ComputeDiffuseFactor(float3 N, float3 L)
{
    return saturate(dot(N, L));
}

float ComputeSpecularFactor(float3 V, float3 N, float3 L)
{
    float3 H = normalize(L + V);
    float NdotH = saturate(dot(N, H));
    
    return pow(NdotH, max(1.f, MaterialCB.SpecularPower));
}

LightResult EvaluateDirectionalLight(DirectionalLight light, float3 V, float3 N)
{
    LightResult result = (LightResult)0;
    
    // Invert the light direction because lighting calculations expect
    // a vector pointing FROM the surface To the light source.
    float3 L = normalize(-light.DirectionVS.xyz);
    
    result.Diffuse  = light.Color * ComputeDiffuseFactor(N, L);
    result.Specular = light.Color * ComputeSpecularFactor(V, N, L);
    
    return result;
}

LightResult CalculateTotalLighting(float3 P, float3 N)
{
    // Lighting calculations are performed in View Space.
    // P is the position of the pixel, so normalize(-P) gives us
    // the direction vector tracking directly back to the camera eye.
    float3 V = normalize(-P);
    
    LightResult totalResult = (LightResult)0;
    
    for (uint i = 0; i < LightPropertiesCB.NumDirectionalLights; ++i)
    {
        LightResult result = EvaluateDirectionalLight(DirectionalLights[i], V, N);
        
        totalResult.Diffuse  += result.Diffuse;
        totalResult.Specular += result.Specular;
    }

    return totalResult;
}

float4 main(PixelShaderInput IN) : SV_Target
{
    float4 baseColor = IN.Color;
    
    if (PipelineOptionsCB.EnableTexture)
    {
        float lod = PipelineOptionsCB.EnableMips ? DiffuseTexture.CalculateLevelOfDetail(LinearRepeatSampler, IN.TexCoord) : 0.f;
        baseColor = DiffuseTexture.SampleLevel(LinearRepeatSampler, IN.TexCoord, lod);
    }

    float3 N = normalize(IN.Normal);
    float3 P = IN.PositionVS.xyz;
    
    LightResult lighting = CalculateTotalLighting(P, N);
    
    float4 emissive = MaterialCB.Emissive;
    float4 ambient = MaterialCB.Ambient;
    float4 diffuse = MaterialCB.Diffuse * lighting.Diffuse;
    float4 specular = MaterialCB.Specular * lighting.Specular;
    
    return ((ambient + diffuse) * baseColor) + specular + emissive;
}
