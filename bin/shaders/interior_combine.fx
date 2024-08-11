#if IS_VOLUMES_COMBINE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "postfx_common.fx"

Texture2D		t_TextureVolumes	: register( t0 );

#define			v_RenderArea		((int2)PSC_Custom_0.xy)
#define			v_ClampMax			((int2)PSC_Custom_0.zw)

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
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

bool IsReliableValue( float2 value )
{
	return !(value.x < 1 && value.x == value.y);
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const int2 pixelCoord = (int2)i.pos.xy;

	const float2 origValue = t_TextureVolumes[pixelCoord].xy;

	float2 resultValue = origValue;

	[branch]
	if ( !IsReliableValue( resultValue ) )
	{
		const int2 offsets[4] = { int2( -1, 0 ), int2( 1, 0 ), int2( 0, -1 ), int2( 0, 1 ) };
		bool hasReliableValue = false;
		[unroll]
		for ( int off_i=0; off_i<4; ++off_i )
		{
			if ( !hasReliableValue )
			{
				const int2 currCoord = clamp( pixelCoord + offsets[off_i], 0, v_ClampMax );			
				const float2 currValue = t_TextureVolumes[currCoord].xy;
				hasReliableValue = IsReliableValue( currValue );
				resultValue = currValue;
			}
		}
	}

	return float4 ( resultValue, 0, 0 );
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#else
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "postfx_common.fx"

Texture2D		t_TextureDepth		: register( t0 );
Texture2D		t_TextureInterior	: register( t1 );

#define v_HalfresClampMax	(PSC_Custom_1.xy)
#define v_FullresClampMax	(PSC_Custom_1.zw)

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
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

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const int2 pixelCoord = (int2)i.pos.xy;
	const int2 interiorCoord = min( pixelCoord / WEATHER_VOLUMES_SIZE_DIV, v_HalfresClampMax );
	
	int2 bestInteriorOffset = 0;

	// bilateral upscale
	#if 1
	{	
		const int2 depthCompareCoord = interiorCoord * WEATHER_VOLUMES_SIZE_DIV;
		const float refDepth = DeprojectDepth( t_TextureDepth[ pixelCoord ].x );

		float bestDepthDiff = abs( refDepth - DeprojectDepth( t_TextureDepth[ depthCompareCoord ] ) );
	
		const int2 offsets[4] = { int2(-1,0), int2(0,-1), int2(1,0), int2(0,1) };
		[unroll] for ( int i=0; i<4; ++i )
		{
			int2 currDepthCoord = depthCompareCoord + offsets[i] * WEATHER_VOLUMES_SIZE_DIV;
			currDepthCoord = clamp( currDepthCoord, 0, v_FullresClampMax );
			float currDepthDiff = abs( refDepth - DeprojectDepth( t_TextureDepth[ currDepthCoord ] ) );
			if ( currDepthDiff <= bestDepthDiff )
			{
				bestDepthDiff = currDepthDiff;
				bestInteriorOffset = offsets[i];
			}
		}	
	}
	#endif

	const int2 bestInteriorCoord = clamp( interiorCoord + bestInteriorOffset, 0, v_HalfresClampMax );
	return t_TextureInterior[ bestInteriorCoord ];
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
