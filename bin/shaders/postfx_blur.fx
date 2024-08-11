#include "postfx_common.fx"


#define vTexCoordTransform VSC_Custom_0
#define vCenterOfBlurSS    VSC_Custom_1
#define vRadialBlurParams  PSC_Custom_0
#define vSineWaveParams    PSC_Custom_1

SamplerState	s_TextureColor	: register( s0 );
Texture2D		t_TextureColor	: register( t0 );


struct VS_INPUT
{
	float4 pos			: POSITION0;

};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coord       	   : TEXCOORD0;
	float3 dirToCenterOfBlur   : TEXCOORD1;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	o.coord = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransform );
	
	o.dirToCenterOfBlur.xy = i.pos.xy - ( vCenterOfBlurSS.xy );
	o.dirToCenterOfBlur.xy = float2(-o.dirToCenterOfBlur.x, o.dirToCenterOfBlur.y);
	//o.dirToCenterOfBlur.xy = float2(o.dirToCenterOfBlur.y, o.dirToCenterOfBlur.x);
	o.dirToCenterOfBlur.z = vCenterOfBlurSS.z ;

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{

    float4 sum = 0.0f;
    const int num_samples = 16;
    
    float2 dir = i.dirToCenterOfBlur.xy * vRadialBlurParams.w * 0.01;
    float dither = 0;//frac(texel * 100);

    float len = length(i.dirToCenterOfBlur.xy);
    float distance = pow(len, vRadialBlurParams.z );

    float4 original = t_TextureColor.Sample( s_TextureColor, i.coord );
    sum = original;
    for ( int samples = 1; samples < num_samples; ++samples)
    {
	float wave = abs(cos(-PSC_TimeVector.x * vSineWaveParams.y + len * vSineWaveParams.z)) * vSineWaveParams.x + (1.0 - vSineWaveParams.x);
	sum += t_TextureColor.Sample( s_TextureColor, i.coord + dir * distance * (samples + dither) * wave );

    }

    float4 result    = sum / num_samples;

    /*if (length(i.dirToCenterOfBlur.xy) < 0.01)
    {
       result = float4(1,0,1,1);
    }
    */
  
    return result;
}
#endif
