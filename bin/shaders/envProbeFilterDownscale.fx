#include "postfx_common.fx"

Texture2D				t_Texture			: register( t0 );
SamplerState			s_Texture			: register( s0 );
#define					iTargetWidth		((int)PSC_Custom_0.x)
#define					iTargetHeight		((int)PSC_Custom_0.y)
#define					iSphereRes			((int)PSC_Custom_0.z)
#define					iMarginRes			((int)PSC_Custom_0.w)
#define					iSourceMipIndex		((int)PSC_Custom_1.x)
#define					iExtentRes			((int)PSC_Custom_1.y)
#define					iSourceExtentRes	((int)PSC_Custom_1.z)
#define					f2SourceInvRes		(PSC_Custom_2.xy)

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

	float CalcGauss( float normFactor )
	{
		float a = 1;
		float b = 0;
		float c = 2.1f;
		float d = 6.4f;
		float e = 1.5f;

		return pow( a * exp( - pow((normFactor - b) * d, 2) / (2 * c * c) ), e );
	}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const int2 pixelCoord = (int2)i.pos.xy;
	const int segmentSize = iSphereRes + 2 * iMarginRes;
	const int2 segmentIdx = pixelCoord / segmentSize;
	const int2 segmentPixelOff = segmentIdx * segmentSize;
	const int2 segmentCoord = pixelCoord - segmentPixelOff;

	float3 result = 0;

	if ( segmentCoord.x >= iMarginRes && 
		 segmentCoord.y >= iMarginRes && 
		 segmentCoord.x < iMarginRes + iSphereRes && 
		 segmentCoord.y < iMarginRes + iSphereRes )
	{
		int2 localCrd = segmentCoord - iMarginRes;

#if IS_PREPARE
		{
			result = 1;
		}
#elif IS_COPY_TO_TEMP
		{
			int2 crd2 = segmentIdx * (iSphereRes + 2 * iExtentRes) + localCrd + iSourceExtentRes;
			result = SAMPLE_MIPMAPS( t_Texture, iSourceMipIndex, crd2 ).xyz;
		}
#else
		{
			int2 crd2 = segmentIdx * (2 * (iSphereRes + 2 * iExtentRes)) + localCrd * 2 + iSourceExtentRes;
			
			/*
			result += SAMPLE_MIPMAPS( t_Texture, iSourceMipIndex, crd2 + int2( 0, 0 ) ).xyz;
			result += SAMPLE_MIPMAPS( t_Texture, iSourceMipIndex, crd2 + int2( 1, 0 ) ).xyz;
			result += SAMPLE_MIPMAPS( t_Texture, iSourceMipIndex, crd2 + int2( 0, 1 ) ).xyz;
			result += SAMPLE_MIPMAPS( t_Texture, iSourceMipIndex, crd2 + int2( 1, 1 ) ).xyz;
			result *= 0.25;
			*/
			
			result.xyz = SAMPLE_LEVEL( t_Texture, s_Texture, (crd2 + 1) * f2SourceInvRes, iSourceMipIndex ).xyz;
		}
#endif

		{
			float2 uv = (localCrd + 0.5) / iSphereRes;
			uv = 2 * uv - 1;
			result *= dot( uv, uv ) <= 1 ? 1 : 0;
		}
	}

	{
		//return length( ((segmentCoord - iMarginRes) + 0.5) / iSphereRes - 0.5 ) <= 0.5 ? float4( 1,1,0,1 ) : float4( 1,0,0,1 );
	}


	//return t_Texture[ int2(0,0) ];
	//return segmentIdx.x == 1 && segmentIdx.y == 0 ? float4( 1,1,0,1 ) : float4( 0,0,0,1 );
	
	return float4 ( result, 1 );
}
#endif
