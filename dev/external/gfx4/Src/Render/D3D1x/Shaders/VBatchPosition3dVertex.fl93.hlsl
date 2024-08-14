float4x4 vfmuniforms[24] : register(c0);
void main( float4 acolor : COLOR0,
           float4 pos : POSITION0,
           float vbatch : COLOR2,
           out float4 color : COLOR0,
           out float4 vpos : POSITION0)
{
    vpos = mul(pos, vfmuniforms[vbatch * 255.01f * 1 + 0+ 0.1f]);
    

    color = acolor;
    
}
