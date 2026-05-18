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
}

Texture2D DiffuseTexture : register(t0);
SamplerState LinearRepeatSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
    if (EnableTexture)
        return DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord) * IN.Color;
    else
        return IN.Color;
}
