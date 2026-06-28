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

struct PipelineOptions
{
    int EnableTexture;
    int EnableMips;
};

ConstantBuffer<Material> MaterialCB : register(b0, space1);
Texture2D DiffuseTexture : register(t0);
SamplerState LinearRepeatSampler : register(s0);
ConstantBuffer<PipelineOptions> PipelineOptionsCB : register(b1);

static const float3 LightDirVS = normalize(float3(0.5f, -0.5f, 0.7f));

float4 main(PixelShaderInput IN) : SV_Target
{
    float4 baseColor = IN.Color;
    
    if (PipelineOptionsCB.EnableTexture)
    {
        float lod = PipelineOptionsCB.EnableMips ? DiffuseTexture.CalculateLevelOfDetail(LinearRepeatSampler, IN.TexCoord) : 0.f;
        baseColor = DiffuseTexture.SampleLevel(LinearRepeatSampler, IN.TexCoord, lod);
    }

    float3 N = normalize(IN.Normal);
    float3 L = -LightDirVS;
    float3 V = normalize(-IN.PositionVS);
    float3 H = normalize(L + V);
    
    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));
    
    float specularIntensity = pow(ndoth, MaterialCB.SpecularPower);
    
    float4 emissive = MaterialCB.Emissive;
    float4 ambient = MaterialCB.Ambient;
    float4 diffuse = MaterialCB.Diffuse * ndotl;
    float4 specular = MaterialCB.Specular * specularIntensity;
    
    return ((ambient + diffuse) * baseColor) + specular + emissive;
}
