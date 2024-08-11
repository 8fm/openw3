#include "postfx_common.fx"
#include "include_constants.fx"
#include "include_utilities.fx"
#include "include_sharedConsts.fx"

Texture2D sColorTexture				: register( t0 );
Texture2D sColorTexture2			: register( t1 );
Texture2D sColorTexture3			: register( t2 );
Texture2D<uint2> g_UnsignedTexture	: register( t3 );

#define vBaseParams 		PSC_Custom_0
#define vZoomParams 		PSC_Custom_1
#define vParam0 			PSC_Custom_2
#define vParam1 			PSC_Custom_3
#define vParam2 			PSC_Custom_4

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

int2 CalcPixelCoord( float2 pos )
{
	int2 vpos = (int2)pos.xy;
	return vpos / vZoomParams.x + vZoomParams.yz;
}

#define NUM_COMPLEXITY_COLORS 8
float3 GetComplexityEnvProbeColor( uint complexity )
{
	float3 complexityColor = 0;
	if ( 0 == complexity )
	{
		complexityColor = float3 ( 0, 0, 0 );
	}
	else if ( 1 == complexity )
	{
		complexityColor = float3 ( 0, 5, 107 );
	}
	else if ( 2 == complexity )
	{
		complexityColor = float3 ( 0, 11, 240 );
	}
	else if ( 3 == complexity )
	{
		complexityColor = float3 ( 2, 152, 167 );
	}
	else if ( 4 == complexity )
	{
		complexityColor = float3 ( 46, 136, 38 );
	}
	else if ( 5 == complexity )
	{
		complexityColor = float3 ( 220, 216, 3 );
	}
	else if ( 6 == complexity )
	{
		complexityColor = float3 ( 220, 117, 0 );
	}
	else if ( 7 == complexity )
	{
		complexityColor = float3 ( 219, 18, 0 );
	}
	else
	{
		complexityColor = float3 ( 255, 255, 255 );
	}

	return complexityColor / 255;
}

float3 ApplyComplexityLegend( float3 color, uint2 vpos )
{
	const uint height = 20;
	const uint width = 30;
	const uint gap = 1;			
		
	if ( vpos.x <= width + 2 )
	{
		const uint legendNum = NUM_COMPLEXITY_COLORS;
		[unroll] //< without unroll I'm getting some flickering of legend's colors in novigrad (compiler bug?)
		for ( uint legend_i=0; legend_i<legendNum; ++legend_i )
		{
			uint y0 = (uint)(screenDimensions.y / 2) - (height + gap) * legendNum / 2 + legend_i * (height + gap);
			uint y1 = y0 + height;

			if ( vpos.y > y0 && vpos.y < y1 && vpos.x < width )
			{
				color = GetComplexityEnvProbeColor( legend_i );
			}
			else if ( vpos.y >= y0 - 3 && vpos.y <= y1 + 3 )
			{
				color = float3 ( 0.5, 0.5, 0.5 );
			}
		}
	}

	return color;
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float mode  = vBaseParams.x;
	float gamma = vBaseParams.y;
	int2  vpos  = CalcPixelCoord( i.pos.xy );
	
	float4 orig_color   = sColorTexture[vpos];
	float4 result_color = orig_color;
	
	if ( 0 == mode ) // color channels
	{
		result_color.x = dot( orig_color, vParam0 );
		result_color.y = dot( orig_color, vParam1 );
		result_color.z = dot( orig_color, vParam2 );
		result_color.w = 1.0;
	}
	else if ( 10 == mode ) // interleaved color
	{
		float4 encoded_value = 0;
		result_color.xyz = DecodeTwoChannelColor( sColorTexture, vpos, encoded_value );
		result_color.w   = 1.0;
	}	
	else if ( 11 == mode ) // albedo normalized
	{
		float3 albedo = pow( sColorTexture[vpos].xyz, 2.2 );
		float3 specularity = pow( sColorTexture2[vpos].xyz, 2.2 );
		result_color.xyz = pow( saturate( albedo * (1 - specularity) ), 1.0 / 2.2 );
		result_color.w   = 1.0;
	}	
	else if ( 20 == mode ) // normal worldspace
	{
		result_color.xyz = normalize( orig_color.xyz * 2.0 - 1.0 ) * 0.5 + 0.5;
		result_color.w   = 1.0;
	}	
	else if ( 21 == mode ) // normal viewspace
	{
		float3 N = orig_color.xyz * 2.0 - 1.0;
		N = normalize( N );
		//N = mul( N, (float3x3)worldToView );
		N = mul( (float3x3)worldToView, N );
		N = normalize( N );
		N.y *= -1; //< flip Y to match our normalmaps
		N.z *= -1; //< we want to make it look like a normalmap (so the Z pointing into the camera)
		result_color.xyz = N * 0.5 + 0.5;
		result_color.w   = 1.0;
	}	
	else if ( 30 == mode ) // difference of two textures
	{
		const float tex_gamma = vParam0.x;
		float4 ref_color = sColorTexture2[vpos];
		result_color.xyz = pow( max( 0, pow( max( 0, orig_color.xyz ), tex_gamma ) - pow( max( 0, ref_color.xyz ), tex_gamma ) ), 1.0 / tex_gamma );
		result_color.w   = 1.0;
	}	
	else if ( 40 == mode || 41 == mode ) // dimmers
	{
		float zw = orig_color.x;
		float3 worldSpacePosition = PositionFromDepthRevProjAware( zw, vpos );
		float3 worldSpaceNormal = normalize(sColorTexture3[vpos].xyz - 0.5);
		float3 baseColor = lerp( float3 ( 1, 0.5, 0.5 ), float3 ( 0.5, 1, 0.5 ), abs( dot( normalize( float3( 1, 1, -1 ) ), worldSpaceNormal ) ) );

		float3 dimmerValue = 1;
		if ( 40 == mode )
		{
			dimmerValue = CalcDimmersFactorTransparency( worldSpacePosition, vpos );
		}
		else
		{
			float3 viewVec = worldSpacePosition.xyz - cameraPosition.xyz;
			float  viewDist = length( viewVec );
			float3 posStart = cameraPosition.xyz;
			float3 posEnd = posStart + viewVec / max( 0.01, viewDist ) * min( viewDist, 100 );

			const int numSamples = 50;

			float randOffset = PosBasedRandomColorData( float3(vpos.xy, 0) ).x;
			randOffset -= floor( randOffset );
	
			// without 'loop' shader get's extra big on ps4,
			// (252 vgpr's, and some scratch pad requirements)
			[loop]
			for ( int i=0; i<numSamples; ++i )
			{
				float3 currWorldPosition = lerp( posStart, posEnd, (i + randOffset) / (numSamples - 0.0) );
				dimmerValue = min( dimmerValue, CalcDimmersFactorTransparency( currWorldPosition, vpos ) );
			}
		}

		result_color.xyz = baseColor * dimmerValue;
		result_color.w = 1.0;
	}	
	else if ( 50 == mode )
	{
		const float3 albedo = sColorTexture2[vpos].xyz;
		const float3 worldSpacePosition = PositionFromDepthRevProjAware( sColorTexture[vpos].x, vpos );
		const float3 baseColor = lerp( dot( float3( 0.3, 0.5, 0.2 ), albedo ), 1, 0.1 );		
		const uint complexity = CalcEnvProbes_MipLodComplexity( worldSpacePosition );
		const float3 complexityColor = GetComplexityEnvProbeColor( complexity );

		result_color = float4 ( complexityColor * baseColor, 1 );
		result_color.xyz = ApplyComplexityLegend( result_color.xyz, vpos );		
	}
	else if ( 60 == mode )
	{
		const uint stencilValue = GetStencilValue( g_UnsignedTexture[ vpos ] );
		const uint stencilMask = (uint)(vParam0.x + 0.001);

		if ( 0xff == (0xff & stencilMask) )
		{
			float3 val = float3( 0.45, 0.325, 0.2 );
			val /= dot( 1, val );
			
			result_color.xyz = 0;
			result_color.x += (stencilValue & (1 << 0))  ? val.x : 0;
			result_color.y += (stencilValue & (1 << 1))  ? val.x : 0;
			result_color.z += (stencilValue & (1 << 2))  ? val.x : 0;
			result_color.xy += (stencilValue & (1 << 3)) ? val.y : 0;
			result_color.xz += (stencilValue & (1 << 4)) ? val.y : 0;
			result_color.yz += (stencilValue & (1 << 5)) ? val.y : 0;
			result_color.x += (stencilValue & (1 << 6))  ? val.z : 0;
			result_color.y += (stencilValue & (1 << 7))  ? val.z : 0;

			if ( 0 != (0xff & stencilValue) )
			{
				result_color.xyz = lerp( 0.5, 1.0, result_color.xyz );
			}
		}
		else
		{
			result_color.xyz = (stencilMask & stencilValue) ? float3 ( 1, 1, 1 ) : float3 ( 0, 0, 0 );
		}

		result_color.w = 1;
	}
	else if ( 70 == mode || 71 == mode )
	{		
		const uint2 volumeVPos = vpos / WEATHER_VOLUMES_SIZE_DIV;
		float2 volumeTexture = sColorTexture2[volumeVPos].xy;
		float3 overlayColor = 0.5f*sColorTexture[vpos].xyz;

		overlayColor.xyz = dot( overlayColor.xyz, 0.5f );

		if ( 71 == mode ) // interior factor
		{			
			float zw = sColorTexture3[vpos].x;
			float3 worldPosition = PositionFromDepthRevProjAware( zw, vpos );		
			float volumeFadeValue = CalculateVolumeCutCustomVolumeTexture( volumeTexture, worldPosition );
			
			result_color.xyz = overlayColor.xyz * lerp( float3( 0.5, 1.25, 0.5 ), float3( 1.25, 0.5, 0.5 ), volumeFadeValue );
			result_color.w = 1;
		}
		else // interior volume
		{
			float mappedVolume_x = 1 - pow( saturate( volumeTexture.x ), 0.5 );
			float mappedVolume_y = 1 - pow( saturate( volumeTexture.y ), 0.5 );
		
			float zw = sColorTexture3[vpos].x;
			float3 worldPosition = PositionFromDepthRevProjAware( zw, vpos );
			const float encodeRange = interiorRangeParams.z;
			const float3 worldVec = worldPosition.xyz - cameraPosition.xyz;
			float distanceToCamera = length( worldVec );
			distanceToCamera /= encodeRange;
			result_color.x = mappedVolume_x;
			result_color.y = mappedVolume_y;
			result_color.z = 0.0f;
			result_color.w = 1.0f;

			if( distanceToCamera > volumeTexture.x && distanceToCamera < volumeTexture.y)
			{
				result_color.xyz *=2.0f;
			}
			else
			{
				result_color.xyz *= 0.3f;
			}
			result_color.xyz += overlayColor.xyz;
			
		}
		
		result_color.xyz = saturate(result_color.xyz);
	}
	else if ( 80 == mode )
	{		
		const float4 vDownsampleRatio = vParam0;
		const uint2 rlr_vpos = vpos * vDownsampleRatio.xy;
		const uint2 rlr_size = screenDimensions.xy * vDownsampleRatio.zw;
		
		float4 rlr_color = float4( 1, 0, 0, 1 );
		if ( vpos.x < rlr_size.x * vDownsampleRatio.z && vpos.y < rlr_size.y * vDownsampleRatio.w )
		{
			rlr_color = sColorTexture[rlr_vpos];
			if ( rlr_color.w > 0 )
			{
				rlr_color /= rlr_color.w;
			}
		}
		
		result_color = float4 ( rlr_color.xyz, 1 );
		result_color.xyz *= 0.2;
	}
	else if ( 90 == mode )
	{
		float nearPlane	= 1 / cameraNearFar.y;
		float farPlane	= 1 / ( cameraNearFar.x + cameraNearFar.y );
		float mappedDepth = log( DeprojectDepthRevProjAware( sColorTexture[vpos].x ) - nearPlane + 1 ) / log( farPlane + 1 );
		result_color =  float4( mappedDepth, mappedDepth, mappedDepth, 1.0f );
	}
	else if ( 100 == mode )
	{
		float3 col = 0;

		float zw = orig_color.x;
		if ( !IsSkyByProjectedDepthRevProjAware( zw ) )
		{
			float interiorFactor = DecodeGlobalShadowBufferInteriorFactor( sColorTexture3[vpos] );
			float3 worldSpacePosition = PositionFromDepthRevProjAware( zw, vpos );
			float3 N = normalize( sColorTexture2[vpos].xyz - 0.5 );
			
			float3 albedo = 1;
			float3 ambient = CalcEnvProbeAmbient( worldSpacePosition, N, true, 0, interiorFactor );		
			col = albedo * ambient;
		}

		result_color = float4( col, 1 );
	}
	else if ( 101 == mode )
	{
		float3 col = 0;

		float zw = orig_color.x;
		if ( !IsSkyByProjectedDepthRevProjAware( zw ) )
		{
			float interiorFactor = DecodeGlobalShadowBufferInteriorFactor( sColorTexture3[vpos] );
			float3 worldSpacePosition = PositionFromDepthRevProjAware( zw, vpos );
			float3 N = normalize( sColorTexture2[vpos].xyz - 0.5 );
			float roughness = sColorTexture2[vpos].w;
		
			float3 camPos = cameraPosition.xyz;

			float3 specularity = 1;
			float3 reflection = CalcEnvProbeReflection_NormalBasedMipLod( worldSpacePosition, normalize( camPos - worldSpacePosition ), N, roughness, vpos, true, interiorFactor );
			col = specularity * reflection;
		}
		
		result_color = float4( col, 1 );
	}
	else if ( 111 == mode ) // lights_overlay
	{
		float dens = 0.0f;
		float3 camPos = cameraPosition.xyz;
		float zw = orig_color.x;
		float3 worldSpacePosition = PositionFromDepthRevProjAware( zw, vpos );
		float3 camDir = normalize( worldSpacePosition - camPos );
		float ddd = length( worldSpacePosition - camPos );
		float dep = saturate(length(camPos - worldSpacePosition)*0.01f);
		[loop]
		for ( int i = 0; i < lightNum; i++)
		{
			float3 center = lights[i].positionAndRadius.xyz;
			float3 del = worldSpacePosition - center;
			float radius = lights[i].positionAndRadius.w;
			float3 v = camPos - center;
			float vdir = dot(v,camDir);
			float pierw = (vdir*vdir) - ( dot(v,v)-(radius*radius) );
			if ( pierw>=0 )
			{
				float tnear = -vdir+sqrt( pierw );
				float tfar = -vdir-sqrt( pierw );
				float val = sign( max(tnear,tfar) );
	
				float3 del = worldSpacePosition - center;
				{
					if( min(tnear,tfar)>ddd )
					{
						dens += saturate( val*0.2f );
					}
					else
					{
						dens += saturate( val );
					}
				}
			}	
			if( abs(radius - length(del))<0.005f )
			{
				dens += 100.0f;
			}
		}
		dens = saturate( dens/ 50.0f );
		float col = PackYCbCr( sColorTexture2[vpos].xyz ).x *0.3f;
		
		result_color.x = saturate(col+dens);
		result_color.y = col;
		result_color.z = col;
		result_color.w = 1.0;
	}	
	else if ( 112 == mode ) // lights_overlay_per_pixel
	{
		float dens = 0.0f;
		float3 camPos = cameraPosition.xyz;
		float zw = orig_color.x;
		float3 worldSpacePosition = PositionFromDepthRevProjAware( zw, vpos );
		float dep = saturate(length(camPos - worldSpacePosition)*0.01f);
		[loop]
		for ( int i = 0; i < lightNum; i++)
		{
			float3 center = lights[i].positionAndRadius.xyz;
			float3 del = worldSpacePosition - center;
			float radius = lights[i].positionAndRadius.w;
			if( dot(del,del)<radius*radius )
			{
				dens += 1.0f;
			}
		}
		float col = PackYCbCr( sColorTexture2[vpos].xyz ).x*0.3f;
		dens = saturate( dens/ 50.0f );
		result_color.x = saturate(col+dens);
		result_color.y = col;
		result_color.z = col;
		result_color.w = 1.0;
	}	
	else // 'invalid mode' overlay
	{
		if ( (0 != (vpos.x & 16)) != (0 != (vpos.y & 16)) )
		{
			result_color.xyz = float3( 1, 0, 0 );
		}
	}

	return float4 ( pow( abs(result_color.xyz), gamma ), result_color.w );
}
#endif
