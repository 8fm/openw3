#include "postfx_common.fx"


#ifdef PIXELSHADER
#define iSourceTextureRes		((uint)PSC_Custom_0.x)
#define iSphereResolution		((uint)PSC_Custom_0.y)
#define iExtent					((uint)PSC_Custom_0.z)
#define iSegmentSize			(iSphereResolution + 2 * iExtent)

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if IS_INTERIOR_FALLBACK

TextureCube				t_Texture				: register( t0 );
SamplerState			s_Texture				: register( s0 );

#define					bIsSky					(PSC_Custom_1.x > 0)

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
	const uint2 pixelCoord = i.pos.xy;

	if ( bIsSky )
	{		
		return float4 ( 0, 0, 0, 1 );
	}

	float3 dir;
	{
		const uint2 segIndex = pixelCoord / iSegmentSize;
		const uint2 segCoord = pixelCoord % iSegmentSize;

		float2 uv = (0.5 + (float2)segCoord - (float)iExtent) / (float)iSphereResolution;;
		uint sphereFaceIndex = segIndex.x;

		float4 coord = ParaboloidToCube( uv, sphereFaceIndex );
		dir = coord.xyz;	
	}

	float3 resultColor = SAMPLE_LEVEL( t_Texture, s_Texture, dir, 0 ).xyz;

	resultColor *= resultColor; // gamma -> linear approx

	return float4 ( resultColor, 1 );

}
#endif

#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if !IS_INTERIOR_FALLBACK

TEXTURE2D_ARRAY	t_TextureDepth	: register( t0 );
#if !IS_SKYFACTOR
TEXTURE2D_ARRAY	t_TextureColor	: register( t1 );
#endif

#if IS_DEPTH || IS_DEPTH_AND_SKY
# define vCameraPosition		PSC_Custom_1
# define mScreenToWorld			transpose( float4x4 ( PSC_Custom_2, PSC_Custom_3, PSC_Custom_4, PSC_Custom_5 ) )
#endif

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
uint3 GetFaceFetchData( float3 dir )
{
	uint resultFaceIndex = 0;
	float2 resultUV = 0;

	dir = normalize( dir );
	float3 absDir = abs( dir );
	if ( absDir.x > max( absDir.y, absDir.z ) )
	{
		if ( dir.x >= 0 )
		{
			resultUV = dir.yz / absDir.x * 0.5 + 0.5;
			resultUV = float2( 1 - resultUV.y, resultUV.x );
			resultFaceIndex = 1;
		}
		else
		{
			resultUV = dir.yz / absDir.x * 0.5 + 0.5;
			resultUV = float2( resultUV.y, resultUV.x );
			resultFaceIndex = 0;			
		}
	}
	else if ( absDir.y >= max( absDir.x, absDir.z ) )
	{
		if ( dir.y > 0 )
		{
			resultUV = dir.xz / absDir.y * 0.5 + 0.5;	
			resultUV.x = 1 - resultUV.x;
			resultFaceIndex = 3;
		}
		else
		{
			resultUV = dir.xz / absDir.y * 0.5 + 0.5;		
			resultUV = float2( 1 - resultUV.x, 1 - resultUV.y );
			resultFaceIndex = 2;
		}
	}
	else //if ( absDir.z > max( absDir.x, absDir.y ) )
	{
		if ( dir.z > 0 )
		{
			resultUV = dir.xy / absDir.z * 0.5 + 0.5;
			resultFaceIndex = 5;
		}
		else
		{
			resultUV = dir.xy / absDir.z * 0.5 + 0.5;
			resultUV = float2 ( 1 - resultUV.x, resultUV.y );
			resultFaceIndex = 4;
		}
	}

	return uint3( resultFaceIndex, clamp( iSourceTextureRes * resultUV, 0, iSourceTextureRes - 1 ) );
}

float FetchDepthTexture( int idx, int2 pixelCoord )
{
	return t_TextureDepth[int3(pixelCoord, idx)].x;
}

#if !IS_SKYFACTOR
float4 FetchColorTexture( int idx, int2 pixelCoord )
{
	return t_TextureColor[int3(pixelCoord, idx)];
}
float3 FetchAlbedoTexture( uint idx, uint2 pixelCoord )
{
	return DecodeTwoChannelColorArray( t_TextureColor, idx, pixelCoord );
}
#endif

float4 ps_main( float4 vpos : SYS_POSITION ) : SYS_TARGET_OUTPUT0
{
	const uint2 pixelCoord = vpos.xy;

	float4 result = 0;

	uint faceFetchIndex;
	uint2 faceFetchPixelCoord;
	{
		const uint2 segIndex = pixelCoord / iSegmentSize;
		const uint2 segCoord = pixelCoord % iSegmentSize;

		float2 uv = (0.5 + (float2)segCoord - (float)iExtent) / (float)iSphereResolution;;
		uint sphereFaceIndex = segIndex.x;

		float4 coord = ParaboloidToCube( uv, sphereFaceIndex );
		float3 dir = coord.xyz;	
		uint3 faceFetchData = GetFaceFetchData( dir );

		faceFetchIndex = faceFetchData.x;
		faceFetchPixelCoord = faceFetchData.yz;
	}

	const float depth = FetchDepthTexture( faceFetchIndex, faceFetchPixelCoord );
	const float skyFactor = IsSkyByProjectedDepth( depth ) ? 1.f : 0.f;

#if IS_ALBEDO
	{
		const float3 albedo = pow( abs( FetchAlbedoTexture( faceFetchIndex, faceFetchPixelCoord ).xyz ), 2.2 );
		result = float4 ( albedo * (1 - skyFactor), 1 );
	}
#elif IS_NORMALS
	{
		result = float4 ( FetchColorTexture( faceFetchIndex, faceFetchPixelCoord ).xyz, 1 );
	}
#elif IS_DEPTH || IS_DEPTH_AND_SKY
	{
		const float3 worldSpacePosition = PositionFromDepthFullCustom( depth, faceFetchPixelCoord, iSourceTextureRes, mScreenToWorld );
		const float dist = length( worldSpacePosition - vCameraPosition.xyz );
		result = float4 ( dist, 0, 0, 1 );
		#if IS_DEPTH_AND_SKY
			result.y = skyFactor;
		#endif
	}
#elif IS_SKYFACTOR
	{		
		result = float4 ( skyFactor, skyFactor, skyFactor, 1 );
	}
#else
# error Invalid
#endif

	return result;
}

#endif

#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////