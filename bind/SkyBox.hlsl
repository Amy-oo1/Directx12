 cbuffer cbPerObject : register(b0)
{
    row_major matrix g_mWorldViewProjection;
}
 
TextureCube g_EnvironmentTexture : register(t0);
SamplerState g_sam : register(s0);
 
struct SkyboxVS_Input
{
    float4 Pos : POSITION;
};
 
struct SkyboxVS_Output
{
    float4 Pos : SV_POSITION;
    float3 Tex : TEXCOORD0;
};
 
SkyboxVS_Output VSMain(SkyboxVS_Input Input)
{
    SkyboxVS_Output Output;
    
    Output.Pos = Input.Pos;
    Output.Tex = normalize(mul(Input.Pos, g_mWorldViewProjection));
    
    return Output;
}
 
float4 PSMain(SkyboxVS_Output Input) : SV_TARGET
{
    float4 color = g_EnvironmentTexture.Sample(g_sam, Input.Tex);
    return color;
}