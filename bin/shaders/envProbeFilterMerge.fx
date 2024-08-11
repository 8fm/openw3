#include "postfx_common.fx"

Texture2D				t_Texture				: register( t0 );
SamplerState			s_Texture				: register( s0 );
#define					iTargetWidth			((int)PSC_Custom_0.x)
#define					iTargetHeight			((int)PSC_Custom_0.y)
#define					iSphereRes				((int)PSC_Custom_0.z)
#define					iMarginRes				((int)PSC_Custom_0.w)
#define					iExtentRes				((int)PSC_Custom_1.x)

#if !IS_PREPARE
Texture2D				t_MergeSum				: register( t1 );
#define					iMergeSumMipIndex		((int)PSC_Custom_2.x)
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

#ifdef IS_PREPARE
// m_shaderEnvProbeFilterPrepareMerge currently writes to 32 bit render target
#pragma PSSL_target_output_format(default FMT_32_AR)
#endif

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	int2 pixelCoord = (int2)i.pos.xy;
	int2 segIdx = pixelCoord / (iSphereRes + 2 * iExtentRes);	
	{
		pixelCoord = pixelCoord % (iSphereRes + 2 * iExtentRes);

		const int rejectionSize = iExtentRes - iMarginRes;
		const int rejectionRange = (iSphereRes + 2 * iExtentRes) - rejectionSize;
		[branch]
		if ( pixelCoord.x < rejectionSize || pixelCoord.y < rejectionSize || pixelCoord.x >= rejectionRange || pixelCoord.y >= rejectionRange )
		{
			return float4 ( 0, 0, 0, 1);
		}

		pixelCoord = (pixelCoord - iExtentRes) + iMarginRes;
		pixelCoord += segIdx * (iSphereRes + 2 * iMarginRes);
	}

	const int2 sphereCoord = pixelCoord % (iSphereRes + 2 * iMarginRes) - iMarginRes;

	const int segmentSize = iSphereRes + 2 * iMarginRes;
	const int2 segmentIdx = segIdx;
	const int2 segmentPixelOff = int2( 0, segmentIdx.y * segmentSize );
	
	//return float4( sphereCoord.xy / (float)iSphereRes, 0, 1 );
	//return float4( sphereCoord.x < (float)iSphereRes ? float2(1,1) : float2(0,0), 0, 1 );

	float2 crdSecond;
	float secondAlpha = 0;
	{
		const int sphereFaceIndex = pixelCoord.x >= iSphereRes + 2 * iMarginRes ? 1 : 0;
		const int secondSphereIndex = 1 - sphereFaceIndex;
		const float4 secondDir = ParaboloidToCube( (sphereCoord + 0.5) / iSphereRes, sphereFaceIndex );
		
//		secondAlpha = secondDir.w;
		secondAlpha = 1;

		float2 scrd = CubeToParaboloid( secondDir.xyz, secondSphereIndex );

		{
			crdSecond = scrd * iSphereRes + iMarginRes;
			float2 clampedCrdSecond =  clamp( crdSecond, 0.5, iSphereRes + 2 * iMarginRes - 0.5 );
			if ( any( crdSecond != clampedCrdSecond ) )
			{
				secondAlpha = 0;
				crdSecond = clampedCrdSecond;
			}
			crdSecond = (crdSecond + segmentPixelOff + float2( secondSphereIndex * (iSphereRes + 2 * iMarginRes), 0 ) ) / float2( iTargetWidth, iTargetHeight );
		}
	}

	float3 v0 = t_Texture[ pixelCoord ].xyz;
	float3 v1 = secondAlpha * SAMPLE_LEVEL( t_Texture, s_Texture, crdSecond, 0 ).xyz;

	/*
	{
		const int sphereFaceIndex = pixelCoord.x >= iSphereRes + 2 * iMarginRes ? 1 : 0;
		float4 v = ParaboloidToCube( (sphereCoord + 0.5) / iSphereRes, sphereFaceIndex );
		float2 sv = CubeToParaboloid( v.xyz, sphereFaceIndex );
		//sv.y = 1 - sv.y;
		float2 crd = (sv * iSphereRes + iMarginRes + float2( sphereFaceIndex * (iSphereRes + 2 * iMarginRes), 0 )) / float2( iTargetWidth, iTargetHeight );

		v0 = SAMPLE_LEVEL( t_Texture, s_Texture, crd, 0 );
	}
	*/

	//v0.xyz /= max( 0.001, v0.w );
	//v1.xyz /= max( 0.001, v1.w );

	//v0.xyz *= v0.w;
	//v1.xyz *= v1.w;


	//v1 = 0;

	/*
	if ( length( (sphereCoord.xy + 0.5) / iSphereRes - 0.5 ) >= 0.5 )
	{
	//	return 0;
	}
	*/

	{
	//	return length( (sphereCoord + 0.5) / iSphereRes - 0.5 ) <= 0.5 ? float4( 1,1,0,1 ) : float4( 1,0,0,1 );
	}

	float3 sum = v0 + v1;

#if !IS_PREPARE
	sum.xyz /= max( 0.00001, SAMPLE_MIPMAPS( t_MergeSum, iMergeSumMipIndex, (int2)i.pos.xy ).x );
	//sum.xyz /= max( 0.0001, sum.w );
#endif

	//sum.xyz *= 6;

	return float4( sum.xyz, 1 );
}
#endif
