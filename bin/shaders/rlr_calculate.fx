#define vDownsampleRatio		(PSC_Custom_8)

#if IS_MERGE
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "postfx_common.fx"
Texture2D			t_TextureMaskFull		: register( t0 );
Texture2D			t_TextureReflection		: register( t1 );
Texture2D			t_TextureHistory		: register( t2 );
Texture2D			t_TextureDepth			: register( t3 );
Texture2D			t_TextureExtruded		: register( t4 );

SamplerState	s_SamplerPoint		: register( s0 );
SamplerState	s_SamplerLinear		: register( s1 );

#define vAreaSize			(PSC_Custom_0.xy)
#define vReflectionInvSize	(PSC_Custom_1.zw)
#define fBlendOverAlpha		(PSC_Custom_2.x)

#ifdef PIXELSHADER
	float4x4 GetWorldToViewPrev()		{ return float4x4( PSC_Custom_3, PSC_Custom_4, PSC_Custom_5, PSC_Custom_6 ); }
#endif

#define vExtrudedCoordScale		(PSC_Custom_7.xy)
#define vExtrudedCoordMax		(PSC_Custom_7.zw)

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

float4 FetchMaskFull( int2 pixelCoord )
{
	return t_TextureMaskFull[pixelCoord * vDownsampleRatio.zw];
}

bool IsReflectionEnabledForPixelCoord( int2 pixelCoord )
{
	float4 mask = FetchMaskFull( pixelCoord );
	return mask.w > 0;
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const uint2 pixelCoord = (uint2)i.pos.xy;

	float3 currReflection = t_TextureReflection[ pixelCoord ].xyz;

	float3 history = t_TextureHistory[ pixelCoord ].xyz;

	float3 clampMin = currReflection;
	float3 clampMax = currReflection;
	const int r = 1;
	[unroll]
	for ( int i=-r; i<=r; ++i )
	[unroll]
	for ( int j=-r; j<=r; ++j )
	{
// 		if ( r==abs(i) && r==abs(j) )
// 		{
// 			continue;
// 		}

		float4 c = t_TextureReflection[ pixelCoord + int2(i,j) ];
		clampMin = min( clampMin, c.xyz );
		clampMax = max( clampMax, c.xyz );
	} 
	
	float3 result = 0;
	float3 clampedHistory = clamp( history, clampMin, clampMax );

	float clampAmount = 0;
	if ( IsReflectionEnabledForPixelCoord( pixelCoord ) )
	{
		clampAmount = 1;		
	}
	else
	{
		// calculate custom clampAmount 
		//
		// Removed because it's extra cost and you can see the level shapes while rotation camera on extruct areas of the reflection :)
		// The level shape shouldn't be a problem since it's only a fallback area, but still noting this for the record..
		//
		/*
		{
			const float sceneDepth = t_TextureDepth[ pixelCoord ];
			const float3 worldPos = PositionFromDepthRevProjAware( sceneDepth, pixelCoord, vAreaSize ).xyz;
			float4 projPos = mul( projectionMatrix, mul( worldToView, float4( worldPos, 1 ) ) );
			projPos.xyz /= projPos.w;

			float4 prevProjPos = mul( projectionMatrix, mul( GetWorldToViewPrev(), float4 ( worldPos.xyz, 1 ) ) );
			prevProjPos.xyz /= prevProjPos.w;
		
			clampAmount = saturate( length( projPos.xy - prevProjPos.xy ) / 0.15 );
		}
		*/
		
		// fetch extruded reflection
// 		{
// 			const float2 extrudedCoord = min( vExtrudedCoordMax, (pixelCoord + 0.5) * vExtrudedCoordScale );
// 			currReflection.xyz = SAMPLE_LEVEL( t_TextureExtruded, s_SamplerLinear, extrudedCoord, 0 ).xyz;
// 		}
	}

	history = lerp( history, clampedHistory, clampAmount );

	//result = lerp( history, currReflection, 1 );
	result = lerp( history, currReflection, fBlendOverAlpha );

	//result = clampAmount;
	
	return float4 ( result, 1 );
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
#elif IS_EXTRUDE
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "postfx_common.fx"

Texture2D		t_TextureColor		: register( t0 );

SamplerState	s_SamplerPoint		: register( s0 );
SamplerState	s_SamplerLinear		: register( s1 );

#define vSourceAreaSize		(PSC_Custom_0.xy)
#define vSourceTextureSize	(PSC_Custom_0.zw)
#define vTargetAreaSize		(PSC_Custom_1.xy)
#define iSourceMipLevel		((int)PSC_Custom_2.x)

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
	const uint2 pixelCoord = (uint2)i.pos.xy;
	
	const float2 coordStep = 2.f / vSourceTextureSize;
	//const float2 coord = (pixelCoord + 0.5) / vTargetAreaSize * (vSourceAreaSize / vSourceTextureSize);
	const float2 coord = (2 * pixelCoord + 1) / vSourceTextureSize;
	const float2 clampMax = (vSourceAreaSize - 1.0) / vSourceTextureSize;
	
	float4 result = 0;

	[unroll]
	for ( int i=0; i<2; ++i )
	[unroll]
	for ( int j=0; j<2; ++j )
	{
		float4 col = SAMPLE_LEVEL( t_TextureColor, s_SamplerLinear, min( clampMax, coord + coordStep * (int2(i,j) - 0.5) ), 0 );
		//col.xyz *= col.w;

		col /= max( 0.0001, col.w );

		result += col;// * col.w;
	}

	// Invalidate result in case we grabbed data that is not on a reflection area edge.
	// We want only edges to be extruded.
	result *= 0 == iSourceMipLevel && result.w >= 4 ? 0 : 1;

	result /= max( 0.001, result.w );	
	
	return result;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
#elif IS_MERGE_EXTRUDED
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "postfx_common.fx"
Texture2D			t_TextureMaskFull		: register( t0 );
Texture2D			t_TextureExtruded		: register( t1 );

SamplerState	s_SamplerPoint		: register( s0 );
SamplerState	s_SamplerLinear		: register( s1 );

#define vFullTargetAreaSize		PSC_Custom_0.xy
#define vSourceTextureSize		PSC_Custom_0.zw
#define iMipStart				((int)PSC_Custom_1.x)
#define iMipEnd					((int)PSC_Custom_1.y)
#define iMipsOffset				((int)PSC_Custom_1.z)

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
float4 ps_main( VS_OUTPUT input ) : SYS_TARGET_OUTPUT0
{
	const uint2 pixelCoord = (uint2)input.pos.xy;
	
	if ( t_TextureMaskFull[ pixelCoord * vDownsampleRatio.zw ].w > 0 )
	{
		discard;
	}

	float4 result = 0;
	const float2 coordOrig = (pixelCoord + 0.5) * PSC_Custom_2.xy;
	for ( int i=iMipStart; i>=iMipEnd; --i )
	{
		const float2 clampMax = (((int2)vFullTargetAreaSize >> i) - 0.5) / ((int2)vSourceTextureSize >> i);
		const float2 coord = min( clampMax, coordOrig );

		float4 currValue = SAMPLE_LEVEL( t_TextureExtruded, s_SamplerLinear, coord, i );
		currValue.xyz /= max( 0.0001, currValue.w );

		result = lerp( result, float4(currValue.xyz,1), currValue.w );
	}

	result.xyz /= max( 0.0001, result.w );
 
	return float4 ( result.xyz, 1 );
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
#else
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ENABLE_ENVPROBE_FEEDBACK							1
#define ENABLE_SMOOTH_LOCAL_REFLECTION_BORDER				0
#define ENABLE_SMOOTH_LOCAL_REFLECTION_BORDER_VERT_ONLY		1

#include "postfx_common.fx"
#define FORWARD
#include "include_constants.fx"
#include "include_sharedConsts.fx"
#include "include_localReflection.fx"
 
Texture2D			t_TextureMaskFull		: register( t0 );
Texture2D			t_TextureColor			: register( t1 );
Texture2D			t_TextureDepth			: register( t2 );
Texture2D			t_TextureHistory		: register( t3 );
Texture2D			t_TextureDepthFull		: register( t4 );
Texture2D			t_RLRSky				: register( t5 );

SamplerState	s_SamplerPoint		: register( s0 );
SamplerState	s_SamplerLinear		: register( s1 );

#define SAMPLER_MASK					s_SamplerLinear
#define SAMPLER_PREV_REFLECTION			s_SamplerLinear

#define			vFeedbackDimensions		PSC_Custom_0
#define			vCameraPosPrev			(PSC_Custom_2.xyz)
#define			fHistorySurfaceMul		(PSC_Custom_3.x)
#define			fColorSurfaceMul		(PSC_Custom_3.y)
#define			vRLRSkyParams			PSC_Custom_4
#define			vHistoryInvResolution	PSC_Custom_5.zw 

#ifdef PIXELSHADER
	float4x4 GetWorldToViewPrev()		{ return float4x4( PSC_Custom_10, PSC_Custom_11, PSC_Custom_12, PSC_Custom_13 ); }
#endif


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
float4 GetLocalReflectionParam0()
{
	return float4( 0.5 * vFeedbackDimensions.x / vFeedbackDimensions.z, -0.5 * vFeedbackDimensions.y / vFeedbackDimensions.w, 0.5 * vFeedbackDimensions.xy / vFeedbackDimensions.zw );
}

float4 GetLocalReflectionParam1()
{
	return float4 ( vFeedbackDimensions.xy / vFeedbackDimensions.zw, vFeedbackDimensions.xy );
}

float2 CoordNormForViewPos( float3 viewSpacePosition )
{
	float4 p_h = mul( projectionMatrix, float4 ( viewSpacePosition, 1.0 ) );
	float2 crd_h = p_h.xy / p_h.w; 
	return crd_h * GetLocalReflectionParam0().xy + GetLocalReflectionParam0().zw;
}				

float3 WorldPosForCoordNorm( float3 coordNorm )
{
	float _z = TransformDepthRevProjAware( coordNorm.z );
	float3 v = float3( (coordNorm.xy - GetLocalReflectionParam0().zw) / GetLocalReflectionParam0().xy, _z );
	float4 h = mul( screenToWorldRevProjAware, float4( v, 1 ) );
	return h.xyz / h.w;
}

float DepthForPixelCoord( uint2 pixelCoord )
{
	float value = t_TextureDepth[pixelCoord].x;
	return value;
}

float DepthHiResForPixelCoord( uint2 pixelCoord )
{
	float value = t_TextureDepthFull[pixelCoord * vDownsampleRatio.zw].x;
	return value;
}

float DepthForNormCoord( float2 crd_norm )
{
	// ace_optimize: this piece of shit is suboptimal (using min instead of clamp here to be a bit less suboptimal, but still we should do something about it)
	crd_norm = min( crd_norm, (vFeedbackDimensions.xy - 1.5) / vFeedbackDimensions.zw );
	float value = TransformDepthRevProjAware( SAMPLE_LEVEL( t_TextureDepth, s_SamplerPoint, crd_norm, 0 ).x );
	return value;
}

float4 ColorForNormCoordPoint( float2 crd_norm )
{
	crd_norm /= fColorSurfaceMul;
	crd_norm = clamp( crd_norm, 0.5 / vFeedbackDimensions.xy, (vFeedbackDimensions.xy - 1.5) / (vFeedbackDimensions.zw * fColorSurfaceMul) );
	return SAMPLE_LEVEL( t_TextureColor, s_SamplerPoint, crd_norm, 0 );
}

float4 ColorForNormCoord( float2 crd_norm )
{
	crd_norm /= fColorSurfaceMul;
	crd_norm = clamp( crd_norm, 0.5 / vFeedbackDimensions.xy, (vFeedbackDimensions.xy - 1.5) / (vFeedbackDimensions.zw * fColorSurfaceMul) );
	return SAMPLE_LEVEL( t_TextureColor, s_SamplerPoint, crd_norm, 0 );
}

float4 FetchMaskFull( int2 pixelCoord )
{
	return t_TextureMaskFull[pixelCoord * vDownsampleRatio.zw];
}

bool IsReflectionEnabledForPixelCoord( int2 pixelCoord, out bool outIsGlobalReflection )
{
	float4 mask = FetchMaskFull( pixelCoord );
	outIsGlobalReflection = mask.z > 0.5;
	return mask.w > 0;
}

float CalcBorderFadeFactor( float3 coord, bool scaleByX, bool scaleByY )
{
	float sx = 0.15;
	float sy = 0.5;
	
	float2 ooo = saturate( 1 - abs(coord.xy / GetLocalReflectionParam1().xy- 0.5) / 0.5 );
	float str_scale = 1;
	if ( scaleByX )
		str_scale *= saturate( ooo.x / sx );
	if ( scaleByY )
		str_scale *= saturate( ooo.y / sy );	

	str_scale = 1- pow( 1 - str_scale, 2 );
	return str_scale;
}

float4 CalculateLocalReflection( float3 worldPos, float3 worldNormal, float alphaScale, out float4 outFinalCoord, out float outBorderBendFactorReverse, out float3 outDirReflectedWorld )
{
	outFinalCoord				= 0;
	outBorderBendFactorReverse	= 0;
	outDirReflectedWorld		= float3( 0, 0, 1 );

	float3 viewPos				= mul( worldToView, float4 ( worldPos, 1.0 ) ).xyz;
	float3 viewDir				= normalize( viewPos );

	// ---
	
	const float3 dir_reflected_world = CalcDirReflectedWorld( worldPos, worldNormal );
	const float3 dir_reflected = mul( (float3x3)worldToView, dir_reflected_world );
	outDirReflectedWorld = dir_reflected_world;

	// ---

	float3 pos_orig			= viewPos;
	float3 pos_reflected	= viewPos + 100 * dir_reflected;
	float2 crd_orig 		= CoordNormForViewPos( viewPos );
	float2 crd_reflected 	= CoordNormForViewPos( pos_reflected );	
	float3 v_curr 			= float3( crd_orig,			ProjectDepth( pos_orig.z ) );
	float3 v_reflected 		= float3( crd_reflected, 	ProjectDepth( pos_reflected.z ) );

	// ---

	float3 v_step		= v_reflected - v_curr;
	{
	#if ENABLE_REFLECTION_MANIPULATION_SIDES_BEND
		{
			float4 hpos = mul( projectionMatrix, float4 ( viewPos, 1 ) );
			hpos /= hpos.w;
			outBorderBendFactorReverse = ( (1 - abs(hpos.x) - 2.0 / vFeedbackDimensions.x) / 0.25 );
			v_step.x *= saturate( outBorderBendFactorReverse );
		}
	#endif

		float3 v_step_abs = abs( v_step );
		v_step *= 1 / (v_step_abs.x >= v_step_abs.y ? v_step_abs.x * vFeedbackDimensions.z : v_step_abs.y * vFeedbackDimensions.w);
	}

	// Scale the step however you want to increase performance
	v_step *= 1;

	// ---

	int num_samples = 0;
	if ( dir_reflected.z > 0 )
	{
		num_samples = max( vFeedbackDimensions.x, vFeedbackDimensions.y );
		num_samples = min( num_samples, (v_step.x > 0 ? GetLocalReflectionParam1().x - v_curr.x : v_curr.x) / abs(v_step.x) );
		num_samples = min( num_samples, (v_step.y > 0 ? GetLocalReflectionParam1().y - v_curr.y : v_curr.y) / abs(v_step.y) );
		num_samples = min( num_samples, 3 + (v_step.z > 0 ? 1 - v_curr.z : 0) / abs(v_step.z) );
	}

	// ---

	const float compareExtent = 2.0 * abs(v_step.z);
	float3 crd_final_norm = v_curr;	
	bool did_found = false;	
	int iter_num = 0;
	
	[loop]
	for ( int i=1; i<num_samples; i+=4 )
	{	
		float3 v0 = v_curr + i * v_step;
		float3 v1 = v0 + v_step;
		float3 v2 = v1 + v_step;
		float3 v3 = v2 + v_step;

		float4 d = float4 (
			DepthForNormCoord( v0.xy ),
			DepthForNormCoord( v1.xy ),
			DepthForNormCoord( v2.xy ),
			DepthForNormCoord( v3.xy ) );

		iter_num += 1;

		float4 vz = float4( v0.z, v1.z, v2.z, v3.z );
		bool4 cmp = abs((vz - d) - compareExtent) < compareExtent;

		if ( any(cmp) )
		{			
			did_found = true;
			int bestIter = cmp.x ? 0 : (cmp.y ? 1 : (cmp.z ? 2 : 3));
			float3 _v = (v_curr + (i + bestIter) * v_step);
			crd_final_norm = _v;			
			break;
		}		
	}

	float4 refl_color = 0;
	float str_scale = 0;
	[branch]
	if ( did_found )
	{
		str_scale = 1;
	#if ENABLE_SMOOTH_LOCAL_REFLECTION_BORDER
		str_scale *= saturate( alphaScale );	
		str_scale *= CalcBorderFadeFactor( crd_final_norm, true, true );
	#elif ENABLE_SMOOTH_LOCAL_REFLECTION_BORDER_VERT_ONLY
		str_scale *= saturate( alphaScale );	
		str_scale *= CalcBorderFadeFactor( crd_final_norm, false, true );
	#endif

		refl_color = ColorForNormCoord( crd_final_norm.xy );

		outFinalCoord = float4 ( crd_final_norm, 1 );
	}
	
	return float4 ( refl_color.xyz, str_scale );
}

float4 GuardInvalidValues( float4 val )
{
	// Get rid of nan'a. Either from these calculations or from history buffer.
	// This stuff can happen, and when it does it propagates due to local reflection temporal blending.
	if ( isnan( dot( 0.333, val ) ) )
	{
		val = 0;
	}
    
	// max 0 is here as a black water fix for xbone, in case of qnan's - they are not 
	// found by the isnan function. maxing qnan with 0 produces 0.
	// maxing of W component is here just in case.
    val = max( 0, val );	

	return val;
}

void CalcForPixelCoord( uint2 pixelCoord, out float4 outReflection, out float4 outHistory )
{	
	float4 reflectionColor = 0;
	bool isGlobalReflectionResult = false;

	float2 resultHistoryCoord = (pixelCoord.xy + 0.5) * vHistoryInvResolution;
	int resultHistoryMipIndex = 0;

	bool discardHistory = false;

	[branch]
	if ( IsReflectionEnabledForPixelCoord( pixelCoord, isGlobalReflectionResult ) )
	{
		float depthValue = DepthForPixelCoord( pixelCoord );
		#if ENABLE_OCEAN
		{
			// Depth is sampled from full res texture because of the global water. We want the tracing to start
			// at places where the water is (we assume here that it's reflective - it's always valid assumption),
			// but we don't want to hit the water. So we're using the depth buffer (fullres), which already has the water
			// (buffer used for the actual tracing is lowRes and doesn't contain water depth).

			const float oceanDepthValue = DepthHiResForPixelCoord( pixelCoord );

			// merged here because of numerical precision
			depthValue = IsReversedProjectionCamera() ? max( depthValue, oceanDepthValue ) : min( depthValue, oceanDepthValue );
		}
		#endif

		const float3 worldPos = PositionFromDepthRevProjAware( depthValue, pixelCoord, vFeedbackDimensions.xy );
		const float3 worldNormal = float3( 0, 0, cameraPosition.z >= worldPos.z ? 1 : -1 );		
	
		float4 finalCoord = 0;
		float3 dirReflectedWorld = float3( 0, 0, 1 );
		float sideBendFactorReverse = 1;
		reflectionColor = CalculateLocalReflection( worldPos, worldNormal, 1, finalCoord, sideBendFactorReverse, dirReflectedWorld );

		// We're using premultiplied value because of history buffer fetching is linearly blended,
		// so when fetching position between a valid reflection texel and blank texel we restore
		// the original intensity by dividing by W.
		// It also allows us to apply the reflection around the edges of non reflective objects in a better way - once
		// again div by W removed the effect of blending with pure black.	
		//reflectionColor.xyz *= reflectionColor.w;
		const float tracedReflectionAmount = reflectionColor.w;

	#if ENABLE_ENVPROBE_FEEDBACK			
		{
			float4 probe_value = SampleRLREnvProbe( isGlobalReflectionResult, worldPos, dirReflectedWorld, t_RLRSky, s_SamplerLinear, vRLRSkyParams );
			reflectionColor = lerp( probe_value, reflectionColor, reflectionColor.w );
		}
	#endif

		reflectionColor.w = 1;

		float smartReprojectionAmount = 1;
		#if ENABLE_REFLECTION_MANIPULATION_SIDES_BEND
		{
			// Taking sides_bend into account into reprojection seems quite complex (no time to handle it correctly at this moment), so 
			// let's just fallback to original, naive reprojection in these areas. 
			// This results with better stability (we still have some reprojection benefits, especially with still cameras), but
			// as a drawback, some ghosting may be more visible during camera movement at the borders of the screen.

			smartReprojectionAmount = saturate( (sideBendFactorReverse - 1) / 0.4 ); // >= 0 ? 1 : 0;
		}
		#endif

		// calculate reprojected coord for the history buffer
		float2 historyCoordOrig = CoordNormForViewPos( mul( GetWorldToViewPrev(), float4 ( worldPos.xyz, 1 ) ).xyz ); //< simple reprojection
		[branch]
		if ( 1 == finalCoord.w && smartReprojectionAmount > 0 )
		{
			// let's calculate here some more fancy reprojection, which is needed becasue
			// we're dealing with reflections here, so a simple reprojection produced 
			// some ugly 'sticking to the surface of the reflection.
			// here we're trying to determine what would be the world position on o reflective surface,
			// so that a ray would hit the desired point (which we got from the actual tracing function)

			const float4 finalPosWorld = float4( WorldPosForCoordNorm( finalCoord.xyz ), 1 );
			const float  targetZ = worldPos.z;
				
			float isectT = 0;
			{
				#if ENABLE_REFLECTION_MANIPULATION_STEEPNESS
				{
					// Here we're doing the reprojection. E.g. we don't want to know the reprojected point of the point for which we just 
					// calculated reflection, since in case of reflections it behaves ok only for camera rotations.
					// Instead we want to know what was the point that resulted in reflecting the finalPosition (traced reflectio point)
					// in the previous frame, and that's a huge difference.
					//
					// So the incoming angle (alpha = a) can be solved in Mathematica by Solve[q * Cot[a] + w * Cot[a / 2] == d, a ]
					//
					// Note! We have a fixed outcoming angle divider of value 2. Other values produce really big and ugly solutions (it can't be easily parametrized).
					// Note! Using a fixed targetZ is a kind of an approximation in case of animated or non horizontal surface, but we will have only horizontal reflections and the
					//       animated geometry surfaces we have to just accept with slight ghosting and flickering since it seems to me as impossible to handle efficiently.

					const float q = vCameraPosPrev.z - targetZ;
					const float w = finalPosWorld.z - targetZ;
					const float d = length( vCameraPosPrev.xy - finalPosWorld.xy );
					const float alpha = 2 * atan( (-d + sqrt( d*d + q*q + 2 * q * w )) / q );
					isectT = (q / tan( alpha )) / d;
				}
				#else
				{
					isectT = (vCameraPosPrev.z - targetZ) / (vCameraPosPrev.z - targetZ + finalPosWorld.z - targetZ);
				}
				#endif
			}
			 
			const float3 worldPosIntersection = float3( lerp( vCameraPosPrev.xy, finalPosWorld.xy, isectT ), targetZ );
			const float4 _viewPos = mul( GetWorldToViewPrev(), float4 ( worldPosIntersection, 1 ) );
			float2 historyCoordPredicted = CoordNormForViewPos( _viewPos.xyz );

			historyCoordOrig = lerp( historyCoordOrig, historyCoordPredicted, smartReprojectionAmount );
		}

		const float2 historyCoordClamped = clamp( historyCoordOrig, 0.5 / vFeedbackDimensions.zw, (vFeedbackDimensions.xy - 1.5) / vFeedbackDimensions.zw );
		if ( historyCoordClamped.x == historyCoordOrig.x && historyCoordClamped.y == historyCoordOrig.y )
		{
			resultHistoryCoord = historyCoordClamped / fHistorySurfaceMul;			
		}
		else
		{
			discardHistory = true;
		}

		reflectionColor.w = 1;
	}
	else
	{
		reflectionColor = 0;		

		float depthValue = DepthForPixelCoord( pixelCoord );		
		const float3 worldPos = PositionFromDepthRevProjAware( depthValue, pixelCoord, vFeedbackDimensions.xy );
		const float2 historyCoordOrig = CoordNormForViewPos( mul( GetWorldToViewPrev(), float4 ( worldPos.xyz, 1 ) ).xyz ); //< simple reprojection
		const float2 historyCoordClamped = clamp( historyCoordOrig, 0.5 / vFeedbackDimensions.zw, (vFeedbackDimensions.xy - 1.5) / vFeedbackDimensions.zw );

		resultHistoryCoord = historyCoordClamped;
	}
	
	// output results
	outReflection = GuardInvalidValues( reflectionColor );
	outHistory = discardHistory ? reflectionColor : GuardInvalidValues( SAMPLE_LEVEL( t_TextureHistory, SAMPLER_PREV_REFLECTION, resultHistoryCoord, resultHistoryMipIndex ) );
}

struct PS_Result
{
	float4 reflection : SYS_TARGET_OUTPUT0;
	float4 history    : SYS_TARGET_OUTPUT1;
};

PS_Result ps_main( VS_OUTPUT i )
{
	const uint2 pixelCoord = (uint2)i.pos.xy;

	PS_Result result;
	CalcForPixelCoord( pixelCoord, result.reflection, result.history );

	return result;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
