#include "postfx_common.fx"
#include "include_constants.fx"

float Lum( float3 col )
{	
	//float l0 = dot( col, RGB_LUMINANCE_WEIGHTS_GAMMA );
	float l0 = dot( (col), RGB_LUMINANCE_WEIGHTS_LINEAR );
	return l0 = max( 0.00001, l0 );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if IS_LUM_CALC
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Texture2D	t_TextureColor	: register( t0 );

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
	o.pos = i.pos;
	return o;
}
#endif

#ifdef PIXELSHADER
float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	return Lum( t_TextureColor[i.pos.xy].xyz );
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#else
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ENABLE_DEBUG_OUTPUT				0

#define CLAMP_COLOR_OFFSET_NEAR			-0.035
#define CLAMP_COLOR_RANGE_NEAR			0.035
#define CLAMP_COLOR_OFFSET_FAR			-0.15
#define CLAMP_COLOR_RANGE_FAR			0.175

#define CLAMP_VELOCITY_OFFSET			-15.0
#define CLAMP_VELOCITY_RANGE			10.0
#define CLAMP_FORCE_BY_DEPTH_OFFSET		-5
#define CLAMP_FORCE_BY_DEPTH_RANGE		5

#define ENABLE_COLOR_BASED_CLAMP					1
#define ENABLE_COLOR_BASED_CLAMP_DEPTH_EXTENSION	1
#define ENABLE_VEL_BASED_FALLBACK					1

struct history_info_t
{
	float4  subPixelOffset;
	float4x4 worldToScreenWithOffset;
	float4x4 worldToScreenNoOffset;			
	float4x4 screenToWorldWithOffset;
	float4x4 screenToWorldNoOffset;			
	float4x4 viewToScreenWithOffset;
	float4x4 viewToScreenNoOffset;			
	float4x4 screenToViewWithOffset;
	float4x4 screenToViewNoOffset;			
	float4x4 reprojectNoOffset;			
	float4x4 reprojectWithOffset;			
	float4x4 reprojectNoOffsetInv;
};


START_CB( HistoryData, 10 )
	history_info_t	history[4];
END_CB

Texture2D		t_TextureColor0		: register( t0 );
Texture2D		t_TextureColor1		: register( t1 );
Texture2D		t_TextureColor2		: register( t2 );
Texture2D		t_TextureColor3		: register( t3 );
Texture2D		t_TextureDepth		: register( t4 );
SamplerState	s_Point				: register( s0 );
SamplerState	s_Linear			: register( s1 );

#define vProjHPosScaleBias		PSC_Custom_0

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
	o.pos = i.pos;
	return o;
}
#endif

#ifdef PIXELSHADER

history_info_t	GetHistory( uint idx ) { return history[idx]; }

float2 TransformToFloatCoords( float2 pixelCoord )
{
	return (pixelCoord + 0.5) / surfaceDimensions.xy;
}

float4 SampleTextureColor( uint idx, float2 pixelCoord )
{
	//float4 value = GetColorTex(idx).SampleLevel( s_Linear, TransformToFloatCoords( pixelCoord ), 0 );

	float4 value = float4 ( 1, 0, 0, 1 );

	if ( 0 == idx )			value = SAMPLE_LEVEL( t_TextureColor0, s_Linear, TransformToFloatCoords( pixelCoord ), 0 );
	if ( 1 == idx )			value = SAMPLE_LEVEL( t_TextureColor1, s_Linear, TransformToFloatCoords( pixelCoord ), 0 );
	if ( 2 == idx )			value = SAMPLE_LEVEL( t_TextureColor2, s_Linear, TransformToFloatCoords( pixelCoord ), 0 ).xxxw;
	if ( 3 == idx )			value = SAMPLE_LEVEL( t_TextureColor3, s_Linear, TransformToFloatCoords( pixelCoord ), 0 ).xxxw;
	
	//value.xyz = saturate( value.xyz );

	// clamp input value, so that flickering (for instance from hi frequency specular) would be less visible
	// (it's always nice to merge similar signals..)
	//value.xyz = saturate( value.xyz );

	return value;
}

float SampleTextureLuminance( uint idx, float2 pixelCoord )
{
	//float4 value = GetColorTex(idx).SampleLevel( s_Linear, TransformToFloatCoords( pixelCoord ), 0 );

	float value = 100;

	if ( 0 == idx )			value = ( Lum( SAMPLE_LEVEL( t_TextureColor0, s_Linear, TransformToFloatCoords( pixelCoord ), 0 ).xyz ) );
	if ( 1 == idx )			value = ( Lum( SAMPLE_LEVEL( t_TextureColor1, s_Linear, TransformToFloatCoords( pixelCoord ), 0 ).xyz ) );
	if ( 2 == idx )			value = ( SAMPLE_LEVEL( t_TextureColor2, s_Linear, TransformToFloatCoords( pixelCoord ), 0 ).x );
	if ( 3 == idx )			value = ( SAMPLE_LEVEL( t_TextureColor3, s_Linear, TransformToFloatCoords( pixelCoord ), 0 ).x );
	
	//value = sqrt( value );

	//value.xyz = saturate( value.xyz );

	// clamp input value, so that flickering (for instance from hi frequency specular) would be less visible
	// (it's always nice to merge similar signals..)
	//value.xyz = saturate( value.xyz );

	return value;
}

float SampleTextureDepth( float2 pixelCoord )
{
	return TransformDepthRevProjAware( SAMPLE_LEVEL( t_TextureDepth, s_Linear, TransformToFloatCoords( pixelCoord ), 0 ).x );
}

float4 BuildProjPositionFromDepth2(in float depth, in uint2 pixelCoord)
{
	float2 cpos = (pixelCoord + 0.5f) / screenDimensions.xy;
	cpos *= 2.0f;
    cpos -= 1.0f;
    cpos.y *= -1.0f;
    return float4(cpos, depth, 1.0f);
}

float2 ProjectHPos( float4 hpos )
{
	return (hpos.xy / hpos.w) * vProjHPosScaleBias.xy + vProjHPosScaleBias.zw;
}

float2 SnapProjectedHPos( float2 hpos, int2 pixelCoord )
{
	// snapping so that 4 texels would be bilinearly filtered.
	// this helps with the grass, which is noisy (e.h. this stuff is used
	// to determine if we should clamp the history color or not, so 
	// this way the noisy grass 'surface' becomes one of even color, so the clamping 
	// is lowered).
	return floor( hpos + 0.5 ) + 0.5;
}

float4 SampleFallbackColor( float4 hpos )
{
	return SampleTextureColor( 0, ProjectHPos( hpos ) + 1 * 0.25 * sign(GetHistory(1).subPixelOffset.xy - GetHistory(0).subPixelOffset.xy) );
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float2 pixelCoord = (uint2)i.pos.xy;

#if ENABLE_DEBUG_OUTPUT
	bool isDebugOutput = false;
	if ( !(pixelCoord.x < screenDimensions.x / 2) )
	{
		pixelCoord.x -= screenDimensions.x / 2;
		isDebugOutput = true;
	}
#endif

	const float4 colorOrig = SampleTextureColor( 0, pixelCoord );		
	const float  depth = SampleTextureDepth( pixelCoord );
	const float4 hpos = BuildProjPositionFromDepth2( depth, pixelCoord );
	
	float4 result = float4( colorOrig.xyz, 1 );
	
	float3 _debugDisplay = 0;
	float resultAlpha = 1;
	{
		const history_info_t hist = GetHistory( 1 );
		
		float4 screenPrev = mul( GetHistory( 1 ).reprojectNoOffset, hpos );
		float2 pixelCoordPrev = ProjectHPos( screenPrev );			
		float2 pixelCoordPrevClamped = clamp( pixelCoordPrev, 0.5, screenDimensions.xy - 1.5 );
		[branch]
		if ( any( pixelCoordPrev != pixelCoordPrevClamped ) || screenPrev.z / screenPrev.w < 0 || screenPrev.z / screenPrev.w > 1.01 )
		{
			return SampleFallbackColor( hpos );
		}
			
		const float4 orig_hist_color = SampleTextureColor( 1, pixelCoordPrev );
		float4 hist_color = orig_hist_color;

		float vel_base_clamp = 0;
#if ENABLE_VEL_BASED_FALLBACK
		{
			float2 hpos0 = ProjectHPos( hpos );
			float2 hpos1 = ProjectHPos( mul( GetHistory( 1 ).reprojectNoOffset, hpos ) );
			float vel = distance( hpos0, hpos1 );
			
			vel_base_clamp = saturate( (vel + CLAMP_VELOCITY_OFFSET) / CLAMP_VELOCITY_RANGE );	
		}
#endif
			
		{
			float clamp_amount = 0;

			float4 cmin3 = 999999;
			float4 cmax3 = 0;
			{
				for ( int i=0; i<=1; ++i )
				for ( int j=0; j<=1; ++j )
				{						
					float2 crd = pixelCoord + float2(i,j) * (sign(GetHistory(1).subPixelOffset.xy - GetHistory(0).subPixelOffset.xy) );
					float4 smp = SampleTextureColor( 0, crd );

					cmin3 = min( cmin3, smp );
					cmax3 = max( cmax3, smp );
				}
			}

			#if ENABLE_COLOR_BASED_CLAMP
			clamp_amount = 1;
			[branch]
			if ( vel_base_clamp < 1 )
			{
				float diff = 0;

// 				[unroll]
// 				for ( int i=0; i<4; ++i )
// 				{
// 					if ( 1 == i || 3 == i )
// 					{
// 						continue;
// 					}
// 					
// 					//float2 off = -ii;
// 					float2 off = 0;
// 					if ( 0 == i )		off = float2( 1, 0 );
// 					if ( 1 == i )		off = float2( 0, -2 );
// 					if ( 2 == i )		off = float2( -2, -1 );
// 					if ( 3 == i )		off = float2( -1, 1 );
				[unroll]
				for ( int i=0; i<2; ++i )
				{
					float2 off = 0;
					if ( 0 == i )		off = float2( 1, 0 );
					if ( 1 == i )		off = float2( -2, -1 );
					
					float l0 = SampleTextureLuminance( 0, SnapProjectedHPos( ProjectHPos( hpos ), pixelCoord ) + off );
					float l1 = SampleTextureLuminance( 2, SnapProjectedHPos( ProjectHPos( mul( GetHistory( 2 ).reprojectNoOffset, hpos ) ), pixelCoord ) + off );
					float l2 = SampleTextureLuminance( 1, SnapProjectedHPos( ProjectHPos( mul( GetHistory( 1 ).reprojectNoOffset, hpos ) ), pixelCoord ) + off );
					float l3 = SampleTextureLuminance( 3, SnapProjectedHPos( ProjectHPos( mul( GetHistory( 3 ).reprojectNoOffset, hpos ) ), pixelCoord ) + off );

					float currDiff = abs( l0 - l1 ) / max( 0.0001, min( l0, l1 ) );
					{
						currDiff = max( currDiff, abs( l2 - l3 ) / max( 0.0001, min( l2, l3 ) ) );
					}

					diff = max( diff, currDiff );
				}

				// clamp by depth
				float clamp_offset = CLAMP_COLOR_OFFSET_FAR;
				float clamp_range = CLAMP_COLOR_RANGE_FAR;
				#if ENABLE_COLOR_BASED_CLAMP_DEPTH_EXTENSION
				{
					const float clamp_plane_t = saturate( (DeprojectDepth(depth) + CLAMP_FORCE_BY_DEPTH_OFFSET) / CLAMP_FORCE_BY_DEPTH_RANGE );
					clamp_offset = lerp( CLAMP_COLOR_OFFSET_NEAR, clamp_offset, clamp_plane_t );
					clamp_range = lerp( CLAMP_COLOR_RANGE_NEAR, clamp_range, clamp_plane_t );					
				}
				#endif

				clamp_amount = saturate( (diff + clamp_offset) / clamp_range );
			}
			#endif
			
			_debugDisplay = clamp_amount;

			hist_color.xyz = clamp( hist_color.xyz, cmin3.xyz, cmax3.xyz );
			hist_color.xyz = lerp( orig_hist_color.xyz, hist_color.xyz, clamp_amount );
		}

		result += float4( hist_color.xyz, 1 );

		float4 _c0 = SampleFallbackColor( hpos );
		result.xyz = lerp( result.xyz/result.w, _c0.xyz, vel_base_clamp );
		result.w = 1;
	}

	//////////////////
	
#if ENABLE_DEBUG_OUTPUT
	if ( isDebugOutput )	
	{
		result = 1;
		result.xyz = _debugDisplay;
		//colorResult.xyz = edgeFactor;
		//colorResult.xyz = length( pixelCoord.xy - pixelCoordAccum.xy );		
	}
	else
#endif
	{
		result = result.w > 0.001 ? result / result.w : float4( 0, 0, 0, 1 );
	}

	result.a = 1;

	return result; //t_TextureColor0[pixelCoord] * float4( 1, 0.5, 0.5, 1 );
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
