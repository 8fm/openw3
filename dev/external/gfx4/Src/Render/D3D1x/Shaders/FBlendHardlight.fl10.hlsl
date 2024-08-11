SamplerState sampler_dsttex : register(s0);
Texture2D dsttex : register(t0);
SamplerState sampler_srctex : register(s1);
Texture2D srctex : register(t1);
void main( half2 tc0 : TEXCOORD0,
           half2 tc1 : TEXCOORD1,
           out float4 fcolor : SV_Target0)
{
        float4 src = srctex.Sample(sampler_srctex, tc0);
        float4 dst = dsttex.Sample(sampler_dsttex, tc1);
        float3 screen = 1.0f - 2.0*(1.0f - src.rgb)*(1.0f - dst.rgb);
        float3 mult = 2.0 * src.rgb * dst.rgb;
        float3 select = step(float3(0.5, 0.5, 0.5), dst.rgb);
        fcolor.rgb = (mult * select + screen * (float3(1,1,1) - select)) * src.a + src.rgb * (1 - dst.a) + dst.rgb * (1 - src.a);
        fcolor.a   = src.a + (1.0-src.a)*dst.a;
    
}
