cbuffer Constants { 
float4 vfuniforms[144] : packoffset(c0);
};

void main( half4 afactor : COLOR0,
           float4 pos : SV_Position,
           out half4 factor : COLOR0,
           out float4 fucxadd : TEXCOORD0,
           out float4 fucxmul : TEXCOORD1,
           out half2 tc0 : TEXCOORD2,
           out float4 vpos : SV_Position)
{
    vpos = float4(0,0,0,1);
    vpos.x = dot(pos, vfuniforms[afactor.b*255.01f * 6 + 2+ 0.1f + 0]);
    vpos.y = dot(pos, vfuniforms[afactor.b*255.01f * 6 + 2+ 0.1f + 1]);
    

    tc0.x = dot(pos, vfuniforms[afactor.b*255.01f * 6 + 4+ 0.1f + 0]);
    tc0.y = dot(pos, vfuniforms[afactor.b*255.01f * 6 + 4+ 0.1f + 1]);
    

    fucxadd = vfuniforms[afactor.b*255.01f * 6 + 0+ 0.1f];
    fucxmul = vfuniforms[afactor.b*255.01f * 6 + 1+ 0.1f];
    

      factor = afactor;
    
}
