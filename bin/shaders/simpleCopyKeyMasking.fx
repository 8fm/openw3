#include "commonPS.fx"
#ifdef VERTEXSHADER
#include "commonVS.fx"
#endif

SYS_SAMPLER( SourceTexture, 0 );
SYS_SAMPLER( SourceTextureDepth, 1 );
SYS_SAMPLER( SourceTextureAlpha, 2 );

struct VS_OUTPUT
{
	float4 pos 		: SYS_POSITION;
	float2 tex 		: TEXCOORD0;
	float2 texAlpha : TEXCOORD1;
	float2 texDim	: TEXCOORD2;
};

#define TEX_SIZE		1024.0
#define INV_TEX_SIZE	(1.0/TEX_SIZE)

#ifdef VERTEXSHADER

#define SourceRect		VSC_Custom_0
#define SourceRectAlpha	VSC_Custom_1

VS_OUTPUT vs_main( uint num : SYS_VERTEX_ID )
{
	const float4 verts[4] = {
		{  1, 1, 0, 1 },
		{  1,-1, 0, 1 },
		{ -1, 1, 0, 1 },
		{ -1,-1, 0, 1 }
	};

	const float2 texs[4] = {
		SourceRect.zy,
		SourceRect.zw,
		SourceRect.xy,
		SourceRect.xw
	};

	const float2 texsAlpha[4] = {
		SourceRectAlpha.zy,
		SourceRectAlpha.zw,
		SourceRectAlpha.xy,
		SourceRectAlpha.xw
	};

	VS_OUTPUT o;

	o.pos		= verts[num];
	o.tex		= texs[num];
	o.texAlpha	= texsAlpha[num];
	o.texDim	= SourceRect.zw - INV_TEX_SIZE * 2.0;

	return o;
}

#endif


#ifdef PIXELSHADER

float GetColorMask( float3 rgb, float3 keyColor )
{
	return saturate( distance( rgb, keyColor ) * 32.0 - 1.0 );
}

// Sample near texels to find weighted outline color.
// Basicly finds best candidate to solid color outline.
float3 GatherColor( float2 uv, float3 keyColor, float2 dim )
{
	// return float3(0.0,0.0,0.0);

	float4 N = float4( SYS_SAMPLE( SourceTexture, min( uv + float2( 0, -INV_TEX_SIZE ) , dim ) ).rgb , 1.0 );
	float4 W = float4( SYS_SAMPLE( SourceTexture, min( uv + float2( -INV_TEX_SIZE, 0 ) , dim ) ).rgb , 1.0 );
	float4 E = float4( SYS_SAMPLE( SourceTexture, min( uv + float2(  INV_TEX_SIZE, 0 ) , dim ) ).rgb , 1.0 );
	float4 S = float4( SYS_SAMPLE( SourceTexture, min( uv + float2( 0,  INV_TEX_SIZE ) , dim ) ).rgb , 1.0 );

	N *= GetColorMask( N.xyz, keyColor );
	W *= GetColorMask( W.xyz, keyColor );
	E *= GetColorMask( E.xyz, keyColor );
	S *= GetColorMask( S.xyz, keyColor );

	return ( ( N + W + E + S ) / max( N.w + W.w + E.w + S.w , 1e-4 ) ).xyz;
}

float4 GetKeyColorWeight( float2 uv )
{
	const float3 keyColor	= SYS_SAMPLE_LEVEL( SourceTexture, uv, 0 ).rgb;
	const float  weight		= SYS_SAMPLE_LEVEL( SourceTextureDepth, uv, 0 ).x != 0.0 ? 0.0 : 1.0;

	return float4( keyColor, 1.0f ) * weight;
}

float3 GetKeyColor( float2 screenSize )
{
	const float4 keyColor = 
		GetKeyColorWeight( float2(0,0) ) +
		GetKeyColorWeight( float2(screenSize.x,0) ) +
		GetKeyColorWeight( float2(0,screenSize.y) ) +
		GetKeyColorWeight( screenSize );

	return keyColor.w > 0.0f ? keyColor.xyz / keyColor.w : float3( -1, -1, -1 );
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float3 rgb = SYS_SAMPLE_LEVEL( SourceTexture, i.tex, 0 ).rgb;

	// Mask using additional alpha
	float alpha = SYS_SAMPLE_LEVEL( SourceTextureAlpha, i.texAlpha, 0 ).a;

	// There are some problems when using [branch]. No valid alpha is split out.
	const bool isSolid = t_SourceTextureDepth[ int2( i.pos.xy ) ].x != 0;

	if( isSolid )
	{
		return float4( rgb, alpha );
	}
	else
	{
		const float3 keyColor	= GetKeyColor( i.texDim );
		const float3 outline	= GatherColor( i.tex, keyColor, i.texDim );
		
		float a = GetColorMask( rgb, keyColor ) * alpha;
		a *= keyColor.x > 0.0 ? 1.0 : 0.0;

		rgb = lerp( outline, rgb, a );

		// -- DEBUG --
		// return float4( a, 0.0, 0.0, 1.0 );
		// return float4( keyColor, 1.0 );
		// return float4( outline  , 1.0 );
		// -- DEBUG --

		return float4( rgb, a );
	}

}

#endif
