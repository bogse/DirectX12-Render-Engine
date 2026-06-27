struct PixelShaderInput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD;
};

cbuffer PipelineOptionsCB : register(b1)
{
    int EnableTexture;
    int EnableMips;
}

Texture2D DiffuseTexture : register(t0);
SamplerState LinearRepeatSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{   
    if (!EnableTexture)
        return IN.Color;

    float lod = EnableMips ? DiffuseTexture.CalculateLevelOfDetail(LinearRepeatSampler, IN.TexCoord) : 0.f;
    
    return DiffuseTexture.SampleLevel(LinearRepeatSampler, IN.TexCoord, lod) * IN.Color;
}
