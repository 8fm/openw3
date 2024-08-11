#include "postfx_common.fx"


Texture2D		sTexture		: register( t0 );

#define vCoordOffset		(-PSC_Custom_0.xy)
#define vAlphaOverride		((float2)PSC_Custom_0.zw)
#define fResolution			PSC_Custom_3.x
#define bFlipHorizontal		(PSC_Custom_1.x > 0)
#define bFlipVertical		(PSC_Custom_1.y > 0)
#define fRotation			PSC_Custom_1.z


struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos			: SYS_POSITION;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	
	return o;
}
#endif

#ifdef PIXELSHADER

float4 ps_main( float4 vpos : SYS_POSITION ) : SYS_TARGET_OUTPUT0
{
	uint2 pixelCoord = vpos.xy + vCoordOffset;


	//return ((int)vpos.y & 1) ? float4( 1,1,1,1 ) : float4 ( 0,0,0,1 );

	//return ((int)vpos.x & 1) == ((int)vpos.y & 1) ? float4( 1,1,1,1 ) : float4 ( 0,0,0,1 );

	//pixelCoord /= 4;

	if ( bFlipHorizontal )
	{
		pixelCoord.x = fResolution - 1 - pixelCoord.x;
	}
	if ( bFlipVertical )
	{
		pixelCoord.y = fResolution - 1 - pixelCoord.y;
	}

	if ( fRotation > 0 )
	{
		pixelCoord.xy = uint2( pixelCoord.y, fResolution - 1 - pixelCoord.x );
	}
	else if ( fRotation < 0 )
	{
		pixelCoord.xy = uint2( fResolution - 1 - pixelCoord.y, pixelCoord.x );
	}

	float4 result = 0;

#if ENABLE_DOWNSAMPLE
	result += sTexture[ 2 * pixelCoord ];
	result += sTexture[ 2 * pixelCoord + int2( 1, 0 ) ];
	result += sTexture[ 2 * pixelCoord + int2( 0, 1 ) ];
	result += sTexture[ 2 * pixelCoord + int2( 1, 1 ) ];
	result *= 0.25;
#else
	result = sTexture[pixelCoord];
#endif

	/*
	{
		if ( 0 == PSC_Custom_2.y )
		{
			result = float4 ( 1, 0, 0, 1 );
		}
		else if ( 3 == PSC_Custom_2.y )
		{
			result = float4 ( 0, 1, 0, 1 );
		}
		else if ( 4 == PSC_Custom_2.y )
		{
			result = float4 ( 0, 0, 1, 1 );
		}
	}
	*/
	
	result.a = result.a * vAlphaOverride.x + vAlphaOverride.y;	

	return result;
}

#endif
