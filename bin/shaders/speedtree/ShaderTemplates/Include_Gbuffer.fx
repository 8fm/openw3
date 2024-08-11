///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_GBUFFER
#define ST_INCLUDE_GBUFFER

#if (!ST_OPENGL)

	///////////////////////////////////////////////////////////////////////
	//  Deferred pixel shader output
	//
	//	This is an example SpeedTree G-buffer only. You will need to define
	//	your own G-buffer format here and populate the targets by modifying
	//	Template_3dGeometry_Pixel_Deferred.fx and Template_Billboard_Pixel_Deferred.fx.

	struct SMultiRenderTargets
	{
		#if (ST_XBOX_360)
			float4	m_vRenderTargets[2] : COLOR0;
		#else
			#if (ST_DEFERRED_A2C_ENABLED)
				float4	m_vNullRenderTarget	: ST_RENDER_TARGET0;	// NULL render target (alpha used for alpha-to-coverage)
				float4	m_vRenderTarget0    : ST_RENDER_TARGET1;	// [rgb] = albedo.rgb, [a] = specular power
				float4	m_vRenderTarget1    : ST_RENDER_TARGET2;	// [rg] = normal UV, [b] = specular mask [a] = transmission mask
			#else
//				float4	m_vRenderTarget0	: ST_RENDER_TARGET0;	// [rgb] = albedo.rgb, [a] = specular power
//				float4	m_vRenderTarget1    : ST_RENDER_TARGET1;	// [rg] = normal UV, [b] = specular mask [a] = transmission mask
// LAVA++
				float4	m_vRenderTarget0    : ST_RENDER_TARGET0;
				float4	m_vRenderTarget1    : ST_RENDER_TARGET1;
				float4	m_vRenderTarget2    : ST_RENDER_TARGET2;

// LAVA++
/*
// LAVA--
#ifdef __PSSL__
				uint	m_Coverage	: S_COVERAGE;
#else
				uint    m_Coverage	: SV_Coverage;
#endif
// LAVA++
*/
// LAVA--

					
// LAVA--
			#endif
		#endif
	};

	#define ST_OUTPUT_TYPE SMultiRenderTargets
#else
	#define ST_OUTPUT_TYPE void
#endif

#endif // ST_INCLUDE_GBUFFER

