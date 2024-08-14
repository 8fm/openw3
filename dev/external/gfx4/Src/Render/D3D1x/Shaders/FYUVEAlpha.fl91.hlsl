sampler2D tex[3] : register(s0);
void main( float4 factor : COLOR0,
           float2 tc0 : TEXCOORD0,
           out float4 fcolor : COLOR0)
{
    float c0 = float((tex2D(tex[0], tc0).r - 16./255.) * 1.164);
    float U0 = float(tex2D(tex[1], tc0).r - 128./255.);
    float V0 = float(tex2D(tex[2], tc0).r - 128./255.);
    float4 c = float4(c0,c0,c0,c0);
    float4 U = float4(U0,U0,U0,U0);
    float4 V = float4(V0,V0,V0,V0);
    c += V * float4(1.596, -0.813, 0, 0);
    c += U * float4(0, -0.392, 2.017, 0);
    c.a = 1.0;
    fcolor = c;
    

    fcolor.a *= factor.a;
    

      fcolor = fcolor;
    
}
