cbuffer Constants { 
float4 fsize : packoffset(c0);
float4 offset : packoffset(c1);
float4 scolor : packoffset(c2);
float4 srctexscale : packoffset(c3);
float4 texscale : packoffset(c4);
};

SamplerState sampler_srctex : register(s0);
Texture2D srctex : register(t0);
SamplerState sampler_tex : register(s1);
Texture2D tex : register(t1);
void main( float4 fucxadd : TEXCOORD0,
           float4 fucxmul : TEXCOORD1,
           half2 tc0 : TEXCOORD2,
           out float4 fcolor : SV_Target0)
{
    fcolor       = float4(0, 0, 0, 0);
    float4 color = float4(0, 0, 0, 0);
    float2 i = float2(0, 0);
    for (i.x = -fsize.x; i.x <= fsize.x; i.x++)
    {
      for (i.y = -fsize.y; i.y <= fsize.y; i.y++)
      {
    

    color += tex.SampleLevel(sampler_tex, tc0 + (offset.xy + i) * texscale.xy, 0.0f);
    }
    } // EndBox2.

    fcolor = color * fsize.w;
    

    float4 shadowColor = scolor;
    float4 shadowColor2 = float4(0,0,0,0);
    fcolor.a = fcolor.a * fsize.z;
    fcolor.r = 0.0f;
    

    float4 base = srctex.SampleLevel(sampler_srctex, tc0 * srctexscale.xy, 0.0f);
    float4 baseValue = float4(0,0,0,base.a*0.0001); // Blend a miniscule amount of base in, so it doesn't get compiled out.
    

    float lerpval = clamp((base.a*fsize.z - fcolor.a), 0.0f, 1.0f);
    lerpval *= shadowColor.a;
    fcolor = lerp(baseValue, shadowColor, lerpval) * base.a;
    

      fcolor = (fcolor * float4(fucxmul.rgb,1)) * fucxmul.a;
      fcolor += fucxadd * fcolor.a;
    

    fcolor.rgb = fcolor.rgb * fcolor.a;
    

      fcolor = fcolor;
    
}
