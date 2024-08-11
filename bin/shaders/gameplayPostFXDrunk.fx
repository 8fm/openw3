#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"
#include "include_constants.fx"

SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );

#define DrunkScale			PSC_Custom_0.x
#define EffectForce			PSC_Custom_0.y
#define texelSize			PSC_Custom_0.zw

#define imageCenter			PSC_Custom_2.xy

#define	rotMatrix		float2x2( PSC_Custom_1.xy  , PSC_Custom_1.zw )
#define	rotMatrixInv	float2x2( PSC_Custom_1.wz  , PSC_Custom_1.yx )


struct VS_INPUT
{
    float4 pos      : POSITION0;
};

struct VS_OUTPUT
{
    float4 vec_v   : TEXCOORD0;
    float4 _pos    : SYS_POSITION;
};

struct PS_INPUT
{
    float4 vec_v   : TEXCOORD0;
	float4 vpos    : SYS_POSITION;
};


#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o = (VS_OUTPUT)0;

	o._pos  = i.pos;
	
	return o;
}

#endif

#ifdef PIXELSHADER

float2 RotateCoord( float2 coord , float2x2 mat )
{
	return mul( mat , coord ) + imageCenter;
}

#define offsetSqr 0.707

static const float2 samples[4] = { 
	float2(1.0,0.0),
	float2(offsetSqr,offsetSqr),
	float2(0.0,1.0),
	float2(-offsetSqr,offsetSqr),
};

float4 ps_main( PS_INPUT i ) : SYS_TARGET_OUTPUT0
{
	// - - - - - - - - - - - -
	// Read input targets
    float2 coord 		= (i.vpos.xy + HALF_PIXEL_OFFSET ) * PSC_ViewportSize.zw;

	// Get rotated coords.
	float2 coordToCenter = coord - imageCenter;
	float warpScale = EffectForce*-0.1f + 1.0f;

	// float2 coordRot		= RotateCoord( coordToCenter * warpScale , rotMatrix );
	// float2 coordRotInv	= RotateCoord( coordToCenter * warpScale , rotMatrixInv );

	float2 off = PSC_Custom_1.xy*0.05 * length( coordToCenter );

	float2 coordRot		=  off + ( coordToCenter * warpScale ) + imageCenter;
	float2 coordRotInv	= -off + ( coordToCenter * warpScale ) + imageCenter;

	float2 coordPulse = coordToCenter * ( 1.0 + PSC_Custom_1.y * 0.02 );
	coord = imageCenter + coordPulse;

	coordPulse *= 8.0 * EffectForce * texelSize;

	const float rotScale = EffectForce * min( 10.0 * dot( coordToCenter , coordToCenter ) , 1.0f );

	// Sample all the samples !!!
	float4 smp_color =( t_TextureColor.Sample( s_TextureColor, coord ) +
						t_TextureColor.Sample( s_TextureColor, coord + coordPulse ) +
						t_TextureColor.Sample( s_TextureColor, coord + coordPulse*2.0f ) ) / 3.0f;

	float4 smp_colorRot = float4( 0.0 , 0.0 , 0.0 , 0.0 ); 
	float4 smp_colorRotInv = float4( 0.0 , 0.0 , 0.0 , 0.0 ); 

	const float2  blurScale = ( DrunkScale * rotScale * 5.0f ) * texelSize ;

	[unroll]
	for( int i = 0; i < 4; ++i )
	{
		smp_colorRot		+= t_TextureColor.Sample( s_TextureColor,  samples[i] * blurScale + coordRot );	
		smp_colorRot		+= t_TextureColor.Sample( s_TextureColor, -samples[i] * blurScale + coordRot );	

		smp_colorRotInv		+= t_TextureColor.Sample( s_TextureColor,  samples[i] * blurScale + coordRotInv );
		smp_colorRotInv		+= t_TextureColor.Sample( s_TextureColor, -samples[i] * blurScale + coordRotInv );
	}

	smp_colorRot	*= 0.0625;
	smp_colorRotInv *= 0.0625;

	float4 finalColor = lerp( smp_color , smp_colorRot + smp_colorRotInv , rotScale );

	return lerp( smp_color , finalColor , EffectForce );
}
#endif 
