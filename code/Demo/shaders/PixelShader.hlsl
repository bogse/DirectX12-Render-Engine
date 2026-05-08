struct PixelShaderInput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    return float4(IN.TexCoord, 0.0f, 1.0f);
}