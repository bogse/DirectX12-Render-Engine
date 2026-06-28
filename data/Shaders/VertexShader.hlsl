struct TransformMatrices
{
    matrix ModelView;
    matrix ModelViewProjection;
};

ConstantBuffer<TransformMatrices> TransformMatricesCB : register(b0);

struct VertexPositionNormalTexture
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position   : SV_Position;
    float3 PositionVS : POSITION;
    float3 Normal     : NORMAL;
    float4 Color      : COLOR;
    float2 TexCoord   : TEXCOORD;
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;
    
    OUT.Position = mul(TransformMatricesCB.ModelViewProjection, float4(IN.Position, 1.0f));
    OUT.PositionVS = mul(TransformMatricesCB.ModelView, float4(IN.Position, 1.f)).xyz;
    OUT.Normal = mul((float3x3) TransformMatricesCB.ModelView, IN.Normal);
    OUT.Color = IN.Color;
    OUT.TexCoord = IN.TexCoord;
    
    return OUT;
}
