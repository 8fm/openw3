/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "..\redSystem\crt.h"
#include "../redMath/float16compressor.h"

#include "../gpuApiUtils/gpuApiRenderCommon.h"
namespace GpuApi
{
	// ----------------------------------------------------------------------

	void InitSystemPrimitiveBuffers( Bool assumeRefsPresent )
	{
		if (!assumeRefsPresent)
		{
			SDeviceData &dd = GetDeviceData();

			// Maintain one big vertex and index buffer to mimic Draw[Indexed][Instance]PrimitiveUP functionalities of DX9
			{
				if ( dd.m_drawPrimitiveUPVertexBuffer.isNull() )
				{
					// Create for the first time
					dd.m_drawPrimitiveUPVertexBuffer = GpuApi::CreateBuffer( g_drawPrimitiveUPBufferSize, BCC_Vertex, BUT_Dynamic, BAF_CPUWrite );
					GpuApi::SetBufferDebugPath(dd.m_drawPrimitiveUPVertexBuffer, "drawPrimitiveUP_VB");
				}

				if ( dd.m_drawPrimitiveUPIndexBuffer.isNull() )
				{
					// Create for the first time
					dd.m_drawPrimitiveUPIndexBuffer = GpuApi::CreateBuffer( g_drawPrimitiveUPBufferSize, BCC_Index16Bit, BUT_Dynamic, BAF_CPUWrite );
					GpuApi::SetBufferDebugPath(dd.m_drawPrimitiveUPIndexBuffer, "drawPrimitiveUP_IB");
				}
			}
		}
	}

	void ShutSystemPrimitiveBuffers( Bool dropRefs )
	{
		if (dropRefs)
		{
			SDeviceData &dd = GetDeviceData();
			GpuApi::SafeRelease(dd.m_drawPrimitiveUPIndexBuffer);
			GpuApi::SafeRelease(dd.m_drawPrimitiveUPVertexBuffer);
		}
	}

	void BeginRender()
	{
		GPUAPI_ASSERT( !g_IsInsideRenderBlock );
		g_IsInsideRenderBlock = true;

		SDeviceData &dd = GetDeviceData();

		g_frameIndex++;

#ifdef RED_PLATFORM_WINPC
		if ( dd.m_GPUVendorInterface )
		{
			dd.m_GPUVendorInterface->Update();
		}
#endif

#ifdef RED_PLATFORM_DURANGO
		D3D11X_GRAPHICS_SHADER_LIMITS limits;
		ZeroMemory(&limits, sizeof(limits));

		limits.MaxWavesWithLateAllocParameterCache = 31;
		limits.VSDisableCuMask =  D3D11X_SHADER_DISABLE_CU_0;

		((ID3D11DeviceContextX *)GetDeviceContext())->SetGraphicsShaderLimits(&limits);
#endif
#ifdef RED_PLATFORM_WINPC
		dd.m_renderFence = CreateQuery( QT_CommandsFinished );
		BeginQuery( dd.m_renderFence );
#endif

#ifndef RED_FINAL_BUILD
		dd.m_NumConstantBufferUpdates = 0;
		if (!dd.m_frameQueryDisjoint.isNull())
		{
			Bool disjoint;
			Uint64 frequency;
			Uint64 frameStart;
			Uint64 frameEnd;
			eQueryResult queryResultFrequency = GetQueryResult( dd.m_frameQueryDisjoint, frequency, disjoint, false );
			eQueryResult queryResultStart = GetQueryResult( dd.m_frameStartQuery, frameStart, false );
			eQueryResult queryResultEnd = GetQueryResult( dd.m_frameEndQuery, frameEnd, false );

			if ( queryResultFrequency == QR_Success && queryResultStart == QR_Success && queryResultEnd == QR_Success )
			{
				if (!disjoint)
				{
					dd.m_DeviceUsageStats.m_GPUFrequency = Float( frequency ) / 1000.0f;
					dd.m_DeviceUsageStats.m_GPUFrameTime = Float( frameEnd - frameStart ) / dd.m_DeviceUsageStats.m_GPUFrequency;
					//GPUAPI_LOG( TXT("GPU frame time: %f [freq:%lld]"), dd.m_DeviceUsageStats.m_GPUFrameTime, frequency );
				}
				SafeRefCountAssign( dd.m_frameQueryDisjointPending, dd.m_frameQueryDisjoint );
				SafeRelease( dd.m_frameQueryDisjoint );
				BeginQuery( dd.m_frameQueryDisjointPending );
				EndQuery( dd.m_frameStartQuery );
			}
		}
		else if ( dd.m_frameQueryDisjoint.isNull() && dd.m_frameQueryDisjointPending.isNull() )
		{
			dd.m_frameQueryDisjointPending = CreateQuery( QT_TimestampDisjoint );
			BeginQuery( dd.m_frameQueryDisjointPending );
			EndQuery( dd.m_frameStartQuery );
		}
#endif
	}

	void EndRender()
	{
		GPUAPI_ASSERT( g_IsInsideRenderBlock );
		g_IsInsideRenderBlock = false;

		GpuApi::SetupBlankRenderTargets();

		SDeviceData &dd = GetDeviceData();
#ifdef RED_PLATFORM_WINPC
		EndQuery( dd.m_renderFence );
		SafeRelease( dd.m_renderFence );
#endif

#ifndef RED_FINAL_BUILD
		dd.m_DeviceUsageStats.m_NumConstantBufferUpdates = dd.m_NumConstantBufferUpdates;
#ifdef RED_PLATFORM_DURANGO
		dd.m_DeviceUsageStats.m_constantMemoryLoad = dd.m_constantBufferMem.GetDebugThroughput();
		dd.m_DeviceUsageStats.m_isConstantBufferMemoryWithinBounds = ( 2 * dd.m_constantBufferMem.GetDebugThroughput() / dd.m_constantBufferMem.GetDebugMemSize() ) == 0;
#endif
		if (!dd.m_frameQueryDisjointPending.isNull())
		{
			EndQuery( dd.m_frameEndQuery );
			EndQuery( dd.m_frameQueryDisjointPending );
			SafeRefCountAssign( dd.m_frameQueryDisjoint, dd.m_frameQueryDisjointPending );
			SafeRelease( dd.m_frameQueryDisjointPending );
		}
#endif
	}

	static Bool printHistogram = false;

	void Present( const Rect* sourceRect, const Rect* destRect, const SwapChainRef& swapChain, Bool useVsync, Uint32 vsyncThreshold )
	{
		// the rects are not used since DX10, no support
		RED_UNUSED( sourceRect );
		RED_UNUSED( destRect );

		SDeviceData &dd = GetDeviceData();

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
		if ( printHistogram )
		{
			GPUAPI_LOG( TXT( "VS histogram:" ) );
			for (Uint32 i = 0; i<256; i++)
			{
				GPUAPI_LOG( TXT( "\t%u: %u" ), i, dd.m_VSConstantsDebugHistogram[i] );
			}
			GPUAPI_LOG( TXT( "PS histogram:" ) );
			for (Uint32 i = 0; i<256; i++)
			{
				GPUAPI_LOG( TXT( "\t%u: %u" ), i, dd.m_PSConstantsDebugHistogram[i] );
			}
		}

		Red::System::MemorySet( &dd.m_VSConstantsDebugHistogram[0], 0, sizeof(Uint32) * 256 );
		Red::System::MemorySet( &dd.m_PSConstantsDebugHistogram[0], 0, sizeof(Uint32) * 256 );
#endif

		dd.m_constantBufferDirtyMask = EBDM_All;
		MapWholeBuffersRange();

#if defined(RED_PLATFORM_DURANGO)
		dd.m_constantBufferMem.FrameStart();
#endif

		Uint32 presentInterval = 0;
		if ( useVsync )
		{
			presentInterval = 1;
		}

		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );
		scd.m_swapChain->Present( presentInterval, 0 );
	}

	void PresentMultiplane( const Rect* sourceRect, const Rect* destRect, const SwapChainRef& swapChain, const SwapChainRef& swapChainOverlay, Bool useVsync, Uint32 vsyncThreshold )
	{
		// the sourcerect can be used on XboxOne
		RED_UNUSED( sourceRect );
		RED_UNUSED( destRect );

		SDeviceData &dd = GetDeviceData();

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
		if ( printHistogram )
		{
			GPUAPI_LOG( TXT( "VS histogram:" ) );
			for (Uint32 i = 0; i<256; i++)
			{
				GPUAPI_LOG( TXT( "\t%u: %u" ), i, dd.m_VSConstantsDebugHistogram[i] );
			}
			GPUAPI_LOG( TXT( "PS histogram:" ) );
			for (Uint32 i = 0; i<256; i++)
			{
				GPUAPI_LOG( TXT( "\t%u: %u" ), i, dd.m_PSConstantsDebugHistogram[i] );
			}
		}

		Red::System::MemorySet( &dd.m_VSConstantsDebugHistogram[0], 0, sizeof(Uint32) * 256 );
		Red::System::MemorySet( &dd.m_PSConstantsDebugHistogram[0], 0, sizeof(Uint32) * 256 );
#endif

		dd.m_constantBufferDirtyMask = EBDM_All;
		MapWholeBuffersRange();

#if defined(RED_PLATFORM_DURANGO)
		dd.m_constantBufferMem.FrameStart();
#endif

#if defined( RED_PLATFORM_WINPC )
		GPUAPI_HALT( "Not implemented on PC - we could emulate this by copying into the larger backbuffer and presenting that" );
		RED_UNUSED( swapChain );
		RED_UNUSED( swapChainOverlay );
#elif defined( RED_PLATFORM_DURANGO )
		static const FLOAT VIRTUAL_SCREEN_X = 1920.f;
		static const FLOAT VIRTUAL_SCREEN_Y = 1080.f;

		GPUAPI_ASSERT( swapChain != swapChainOverlay, TXT("The two swapchains cannot be equal, in this case use Present") );

		IDXGISwapChain1* ppSwapChains[ 2 ] = { 0 };
		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );
		SSwapChainData &scdOverlay = dd.m_SwapChains.Data( swapChainOverlay );
		ppSwapChains[0] = scdOverlay.m_swapChain;
		ppSwapChains[1] = scd.m_swapChain;
		DXGIX_PRESENTARRAY_PARAMETERS presentParameterSets[ 2 ] = { 0 };

		presentParameterSets[ 0 ].SourceRect.left   = 0;
		presentParameterSets[ 0 ].SourceRect.top    = 0;
		presentParameterSets[ 0 ].SourceRect.right  = 1920;
		presentParameterSets[ 0 ].SourceRect.bottom = 1080;
		presentParameterSets[ 0 ].ScaleFactorHorz   = 1.f;
		presentParameterSets[ 0 ].ScaleFactorVert   = 1.f;

		presentParameterSets[ 1 ].SourceRect.left   = sourceRect->left;
		presentParameterSets[ 1 ].SourceRect.top    = sourceRect->top;
		presentParameterSets[ 1 ].SourceRect.right  = sourceRect->right;
		presentParameterSets[ 1 ].SourceRect.bottom = sourceRect->bottom;
		presentParameterSets[ 1 ].ScaleFactorHorz   = VIRTUAL_SCREEN_X / static_cast< Float >( sourceRect->right );
		presentParameterSets[ 1 ].ScaleFactorVert   = VIRTUAL_SCREEN_Y / static_cast< Float >( sourceRect->bottom );

		#if MICROSOFT_ATG_DYNAMIC_SCALING
			// We're not using LANCZOS10 filter in case of dynamic scaling because it produces very noticeable artifacts, with different patterns 
			// for particular resolutions. Filter of choice if the best one which doesn't have these problems. From what I tested the overall quality
			// difference between lanchos10 and lanchos4 one is hardly noticeable.
			// Mentioned artifacts were noticeable on surfaces without or very slight variation (sky, deep fog etc).
			presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_LANCZOS4;
		#else
			presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_LANCZOS10;
		#endif
		
		/*
		volatile static Int32 ddd = -1;
		if ( 0 == ddd )			presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_DEFAULT;
		else if ( 1 == ddd )	presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_BILINEAR;
		else if ( 2 == ddd )	presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_SINC4;
		else if ( 3 == ddd )	presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_LANCZOS4;
		else if ( 4 == ddd )	presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_LANCZOS6;
		else if ( 5 == ddd )	presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_LANCZOS6_SOFT;
		else if ( 6 == ddd )	presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_LANCZOS8;
		else if ( 7 == ddd )	presentParameterSets[ 1 ].Flags = DXGIX_PRESENT_UPSCALE_FILTER_LANCZOS10;
		//*/
		
		DXGIXPresentArray( 2, vsyncThreshold, 0, 2, ppSwapChains, presentParameterSets );
#else
#error Unsupported platform
#endif
	}

	void PS4_SubmitDone()
	{
		// PS4 specific
	}

	Float PS4_GetAndUpdateTimeSinceSubmitDone( Float timeToAdd )
	{
		// PS4 specific
		return 0.0f;
	}

	Bool ClearColorTarget( const TextureRef &target, const Float* colorValue )
	{
		if ( !target )
		{
			return false;
		}

		// Clear target
		GetDeviceContext()->ClearRenderTargetView( GetD3DRenderTargetView(target), colorValue );
		return true;
	}

	Bool ClearDepthTarget( const TextureRef &target, Float depthValue, Int32 slice /*= -1*/ )
	{
		if ( !target )
		{
			return false;
		}

		// Clear target
		GetDeviceContext()->ClearDepthStencilView( GetD3DDepthStencilView(target, slice), D3D11_CLEAR_DEPTH, depthValue, 0 );
		return true;
	}

	Bool ClearStencilTarget( const TextureRef &target, Uint8 stencilValue, Int32 slice /*= -1*/ )
	{
		if ( !target )
		{
			return false;
		}

		// Clear target
		GetDeviceContext()->ClearDepthStencilView( GetD3DDepthStencilView(target, slice), D3D11_CLEAR_STENCIL, 0.0f, stencilValue );
		return true;
	}

	Bool ClearDepthStencilTarget( const TextureRef &target, Float depthValue, Uint8 stencilValue, Int32 slice /*= -1*/ )
	{
		if ( !target )
		{
			return false;
		}

		// Clear target
		GetDeviceContext()->ClearDepthStencilView( GetD3DDepthStencilView(target, slice), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depthValue, stencilValue );
		return true;
	}

	void CreateConstantBuffers()
	{
		SDeviceData &dd = GetDeviceData();

#ifndef RED_PLATFORM_DURANGO
		// HACK DX10 constant buffers should be created while initialization
		if (dd.m_FrequentVSConstantBuffer == NULL)
		{
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.ByteWidth = ( VSC_Frequent_Last - VSC_Frequent_First + 1 ) * 4 * sizeof(Float);
			cbDesc.MiscFlags = 0;
			GetDevice()->CreateBuffer( &cbDesc, NULL, &dd.m_FrequentVSConstantBuffer );
#ifdef GPU_API_DEBUG_PATH
			dd.m_FrequentVSConstantBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, 8, "globalvs" );
#endif

			GetDeviceContext()->VSSetConstantBuffers( dd.sc_frequentVSConstantBufferReg, 1, &dd.m_FrequentVSConstantBuffer   );
			GetDeviceContext()->GSSetConstantBuffers( dd.sc_frequentVSConstantBufferReg, 1, &dd.m_FrequentVSConstantBuffer   );
			GetDeviceContext()->HSSetConstantBuffers( dd.sc_frequentVSConstantBufferReg, 1, &dd.m_FrequentVSConstantBuffer   );
			GetDeviceContext()->DSSetConstantBuffers( dd.sc_frequentVSConstantBufferReg, 1, &dd.m_FrequentVSConstantBuffer   );
		}

		if (dd.m_CustomVSConstantBuffer == NULL)
		{
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.ByteWidth = ( VSC_Custom_Last - VSC_Custom_First + 1 ) * 4 * sizeof(Float);
			cbDesc.MiscFlags = 0;
			GetDevice()->CreateBuffer( &cbDesc, NULL, &dd.m_CustomVSConstantBuffer );
#ifdef GPU_API_DEBUG_PATH
			dd.m_CustomVSConstantBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, 8, "customvs" );
#endif

			GetDeviceContext()->VSSetConstantBuffers( dd.sc_customVSConstantBufferReg, 1, &dd.m_CustomVSConstantBuffer   );
			GetDeviceContext()->GSSetConstantBuffers( dd.sc_customVSConstantBufferReg, 1, &dd.m_CustomVSConstantBuffer   );
			GetDeviceContext()->HSSetConstantBuffers( dd.sc_customVSConstantBufferReg, 1, &dd.m_CustomVSConstantBuffer   );
			GetDeviceContext()->DSSetConstantBuffers( dd.sc_customVSConstantBufferReg, 1, &dd.m_CustomVSConstantBuffer   );
		}

		if (dd.m_FrequentPSConstantBuffer == NULL)
		{
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.ByteWidth = ( PSC_Frequent_Last - PSC_Frequent_First + 1 ) * 4 * sizeof(Float);
			cbDesc.MiscFlags = 0;
			GetDevice()->CreateBuffer( &cbDesc, NULL, &dd.m_FrequentPSConstantBuffer );
#ifdef GPU_API_DEBUG_PATH
			dd.m_FrequentPSConstantBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, 8, "globalps" );
#endif

			GetDeviceContext()->PSSetConstantBuffers( dd.sc_frequentPSConstantBufferReg, 1, &dd.m_FrequentPSConstantBuffer   );
		}

		if (dd.m_CustomPSConstantBuffer == NULL)
		{
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.ByteWidth = (PSC_Custom_Last - PSC_Custom_First + 1) * 4 * sizeof(Float);
			cbDesc.MiscFlags = 0;
			GetDevice()->CreateBuffer( &cbDesc, NULL, &dd.m_CustomPSConstantBuffer );
#ifdef GPU_API_DEBUG_PATH
			dd.m_CustomPSConstantBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, 8, "customps" );
#endif

			GetDeviceContext()->PSSetConstantBuffers( dd.sc_customPSConstantBufferReg, 1, &dd.m_CustomPSConstantBuffer   );
		}

#endif

		if ( dd.m_CustomCSConstantBuffer == NULL )
		{
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.ByteWidth = 32 * 4 * sizeof(Float);
			cbDesc.MiscFlags = 0;
			GetDevice()->CreateBuffer( &cbDesc, NULL, &dd.m_CustomCSConstantBuffer );
#ifdef GPU_API_DEBUG_PATH
			dd.m_CustomCSConstantBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, 8, "customcs" );
#endif
			GetDeviceContext()->CSSetConstantBuffers( 0, 1, &dd.m_CustomCSConstantBuffer );
		}
	}

	void BindMainConstantBuffers()
	{
		SDeviceData &dd = GetDeviceData();

		dd.m_constantBufferDirtyMask = EBDM_All;

#ifndef RED_PLATFORM_DURANGO
		

		if ( dd.m_FrequentVSConstantBuffer != NULL)
		{
			GetDeviceContext()->VSSetConstantBuffers( dd.sc_frequentVSConstantBufferReg, 1, &dd.m_FrequentVSConstantBuffer   );
			GetDeviceContext()->GSSetConstantBuffers( dd.sc_frequentVSConstantBufferReg, 1, &dd.m_FrequentVSConstantBuffer   );
			GetDeviceContext()->HSSetConstantBuffers( dd.sc_frequentVSConstantBufferReg, 1, &dd.m_FrequentVSConstantBuffer   );
			GetDeviceContext()->DSSetConstantBuffers( dd.sc_frequentVSConstantBufferReg, 1, &dd.m_FrequentVSConstantBuffer   );
		}

		if (dd.m_CustomVSConstantBuffer != NULL)
		{
			GetDeviceContext()->VSSetConstantBuffers( dd.sc_customVSConstantBufferReg, 1, &dd.m_CustomVSConstantBuffer   );
			GetDeviceContext()->GSSetConstantBuffers( dd.sc_customVSConstantBufferReg, 1, &dd.m_CustomVSConstantBuffer   );
			GetDeviceContext()->HSSetConstantBuffers( dd.sc_customVSConstantBufferReg, 1, &dd.m_CustomVSConstantBuffer   );
			GetDeviceContext()->DSSetConstantBuffers( dd.sc_customVSConstantBufferReg, 1, &dd.m_CustomVSConstantBuffer   );
		}

		if (dd.m_FrequentPSConstantBuffer != NULL)
		{
			GetDeviceContext()->PSSetConstantBuffers( dd.sc_frequentPSConstantBufferReg, 1, &dd.m_FrequentPSConstantBuffer   );
		}

		if (dd.m_CustomPSConstantBuffer != NULL)
		{
			GetDeviceContext()->PSSetConstantBuffers( dd.sc_customPSConstantBufferReg, 1, &dd.m_CustomPSConstantBuffer   );
		}

		// note! m_CustomCSConstantBuffer is handled in a special way, elsewhere!
#endif
		// Note! Durango sets them a different way.
	}

	void UpdateConstantBuffers( )
	{
		SDeviceData &dd = GetDeviceData();

#ifdef RED_PLATFORM_DURANGO
		ID3D11DeviceContextX* context = (ID3D11DeviceContextX*)GetDeviceContext();
#else
		ID3D11DeviceContext* context = GetDeviceContext();
#endif
		
		if ( dd.m_constantBufferDirtyMask & EBDM_FrequentVS )
		{			
			Int32 cbSize = g_FrequentVSRange.GetFullNumIndexes();

#ifdef RED_PLATFORM_DURANGO
			void* mem = dd.m_constantBufferMem.Allocate(cbSize*16,16);
			CopyVectors128( mem, &dd.m_VSConstants[0] , cbSize );

			context->SetFastResources(	D3D11X_SET_FAST_VS_WITH_OFFSET, dd.sc_frequentVSConstantBufferReg, dd.m_PlacementConstantBuffer, UINT64(mem),
										D3D11X_SET_FAST_GS_WITH_OFFSET, dd.sc_frequentVSConstantBufferReg, dd.m_PlacementConstantBuffer, UINT64(mem),
										D3D11X_SET_FAST_HS_WITH_OFFSET, dd.sc_frequentVSConstantBufferReg, dd.m_PlacementConstantBuffer, UINT64(mem),
										D3D11X_SET_FAST_DS_WITH_OFFSET, dd.sc_frequentVSConstantBufferReg, dd.m_PlacementConstantBuffer, UINT64(mem));
#else
			D3D11_MAPPED_SUBRESOURCE mapped;
			GetDeviceContext()->Map(dd.m_FrequentVSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
			CopyVectors128(mapped.pData, &dd.m_VSConstants[0], cbSize);
			GetDeviceContext()->Unmap(dd.m_FrequentVSConstantBuffer, 0);
#endif

#ifndef RED_FINAL_BUILD
			++dd.m_NumConstantBufferUpdates;
#endif
			//g_FrequentVSRange.Reset();
		}

		if ( dd.m_constantBufferDirtyMask & EBDM_CustomVS )
		{
			Int32 cbSize = g_CustomVSRange.GetFullNumIndexes();

#ifdef RED_PLATFORM_DURANGO
			void* mem = dd.m_constantBufferMem.Allocate(cbSize*16, 16);
			CopyVectors128( mem, &dd.m_VSConstants[VSC_Custom_First * 4], cbSize );

			context->SetFastResources(	D3D11X_SET_FAST_VS_WITH_OFFSET, dd.sc_customVSConstantBufferReg, dd.m_PlacementConstantBuffer, UINT64(mem),
										D3D11X_SET_FAST_GS_WITH_OFFSET, dd.sc_customVSConstantBufferReg, dd.m_PlacementConstantBuffer, UINT64(mem),
										D3D11X_SET_FAST_HS_WITH_OFFSET, dd.sc_customVSConstantBufferReg, dd.m_PlacementConstantBuffer, UINT64(mem),
										D3D11X_SET_FAST_DS_WITH_OFFSET, dd.sc_customVSConstantBufferReg, dd.m_PlacementConstantBuffer, UINT64(mem));
#else
			D3D11_MAPPED_SUBRESOURCE mapped;
			GetDeviceContext()->Map(dd.m_CustomVSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
			CopyVectors128(mapped.pData, &dd.m_VSConstants[VSC_Custom_First * 4], cbSize);
			GetDeviceContext()->Unmap(dd.m_CustomVSConstantBuffer, 0);
#endif

#ifndef RED_FINAL_BUILD
			++dd.m_NumConstantBufferUpdates;
#endif
			//g_CustomVSRange.Reset();
		}

		if ( dd.m_constantBufferDirtyMask & EBDM_FrequentPS )
		{
			Int32 cbSize = g_FrequentPSRange.GetFullNumIndexes();

#ifdef RED_PLATFORM_DURANGO			
			void* mem = dd.m_constantBufferMem.Allocate(cbSize*16, 16);
			CopyVectors128( mem, &dd.m_PSConstants[0], cbSize );

			context->PSSetPlacementConstantBuffer(dd.sc_frequentPSConstantBufferReg, dd.m_PlacementConstantBuffer, mem);
#else
			D3D11_MAPPED_SUBRESOURCE mapped;
			GetDeviceContext()->Map(dd.m_FrequentPSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
			CopyVectors128(mapped.pData, &dd.m_PSConstants[0], cbSize);
			GetDeviceContext()->Unmap(dd.m_FrequentPSConstantBuffer, 0);
#endif
#ifndef RED_FINAL_BUILD
			++dd.m_NumConstantBufferUpdates;
#endif
			//g_FrequentPSRange.Reset();
		}

		if ( dd.m_constantBufferDirtyMask & EBDM_CustomPS )
		{
			Int32 cbSize = g_CustomPSRange.GetFullNumIndexes();

#ifdef RED_PLATFORM_DURANGO
			void* mem = dd.m_constantBufferMem.Allocate(cbSize*16, 16);
			CopyVectors128( mem, &dd.m_PSConstants[PSC_Custom_First * 4], cbSize );
			
			context->PSSetPlacementConstantBuffer(dd.sc_customPSConstantBufferReg, dd.m_PlacementConstantBuffer, mem);
#else
			D3D11_MAPPED_SUBRESOURCE mapped;
			GetDeviceContext()->Map(dd.m_CustomPSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
			CopyVectors128(mapped.pData, &dd.m_PSConstants[PSC_Custom_First * 4], cbSize);
			GetDeviceContext()->Unmap(dd.m_CustomPSConstantBuffer, 0);
#endif

#ifndef RED_FINAL_BUILD
			++dd.m_NumConstantBufferUpdates;
#endif
			//g_CustomPSRange.Reset();
		}

		dd.m_constantBufferDirtyMask = 0;

	}

	void DrawPrimitive( ePrimitiveType primitive, Uint32 startVertex, Uint32 primitiveCount )
	{
		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		if (dd.m_HullShader.isNull())
		{
			GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );
		}
		else
		{
			GPUAPI_ASSERT(primitive == PRIMTYPE_TriangleList || primitive == PRIMTYPE_PointList);

			D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
			if ( primitive == PRIMTYPE_PointList )
			{
				topology = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
			}
			GetDeviceContext()->IASetPrimitiveTopology( topology );
		}

		GetDeviceContext()->IASetInputLayout(GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ));

		Int32 vertCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		GetDeviceContext()->Draw( vertCount, startVertex );
	}

	void DrawPrimitiveRaw( ePrimitiveType primitive, Uint32 startVertex, Uint32 primitiveCount )
	{
		SDeviceData &dd = GetDeviceData();

		if (dd.m_HullShader.isNull())
		{
			GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );
		}
		else
		{
			GPUAPI_ASSERT(primitive == PRIMTYPE_TriangleList || primitive == PRIMTYPE_PointList);

			D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
			if ( primitive == PRIMTYPE_PointList )
			{
				topology = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
			}
			GetDeviceContext()->IASetPrimitiveTopology( topology );
		}

		Int32 vertCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		GetDeviceContext()->Draw( vertCount, startVertex );
	}

	void DrawPrimitiveNoBuffers( ePrimitiveType primitive, Uint32 primitiveCount )
	{
		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		// Unbind streams
		ID3D11Buffer* nullBuffers[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = {};
		Uint32 nullStrideOffset[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = {};
		GetDeviceContext()->IASetVertexBuffers( 0, 8, nullBuffers, nullStrideOffset, nullStrideOffset );

		for ( Uint32 i = 0; i <  GPUAPI_VERTEX_LAYOUT_MAX_SLOTS; ++i )
		{
			dd.m_VertexBuffers[i] = BufferRef::Null();
		}

		// FIXME: Better solution storing it in the dd device data struct?
		ID3D11InputLayout* oldInputLayout = nullptr;
		GetDeviceContext()->IAGetInputLayout( &oldInputLayout );

		GetDeviceContext()->IASetInputLayout( nullptr );

		GetDeviceContext()->IASetPrimitiveTopology( Map( primitive ) );

		Int32 vertCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		GetDeviceContext()->Draw( vertCount, 0 );
		if ( oldInputLayout )
		{
			GetDeviceContext()->IASetInputLayout( oldInputLayout );
			oldInputLayout->Release();
		}
	}


	void DrawIndexedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount )
	{
		RED_UNUSED(numVertices);
		RED_UNUSED(minIndex);

		SDeviceData &dd = GetDeviceData();
		
		UpdateConstantBuffers();

		if ( !dd.m_HullShader.isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		GetDeviceContext()->IASetInputLayout(GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ));

		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->DrawIndexed( indexNum, startIndex, baseVertexIndex );
	}

	void DrawInstancedIndexedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount, Uint32 instancesCount )
	{
		RED_UNUSED(numVertices);
		RED_UNUSED(minIndex);

		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		//// Setup instancing
		//GPUAPI_MUST_SUCCEED( dd.m_pDevice->SetStreamSourceFreq( 0, D3DSTREAMSOURCE_INDEXEDDATA  | (UINT)instancesCount ) );
		//GPUAPI_MUST_SUCCEED( dd.m_pDevice->SetStreamSourceFreq( 1, D3DSTREAMSOURCE_INSTANCEDATA | (UINT)1 ) );

		//// Draw
		//GPUAPI_MUST_SUCCEED( dd.m_pDevice->DrawIndexedPrimitive( Map(primitive), baseVertexIndex, minIndex, numVertices, startIndex, primitiveCount ) );

		//// Disable instancing
		//GPUAPI_MUST_SUCCEED( dd.m_pDevice->SetStreamSourceFreq( 0, 1 ) );
		//GPUAPI_MUST_SUCCEED( dd.m_pDevice->SetStreamSourceFreq( 1, 1 ) );

		if ( !dd.m_HullShader.isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( chunkType );
		}
		GetDeviceContext()->IASetInputLayout(GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ));

		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->DrawIndexedInstanced ( indexNum, instancesCount, startIndex, baseVertexIndex, 0 );
	}

	void DrawInstancedPrimitiveIndirect( ePrimitiveType primitive )
	{
		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( chunkType );
		}
		GetDeviceContext()->IASetInputLayout(GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ));

		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->DrawInstancedIndirect( GetD3DBuffer(dd.m_indirectArgs), 0 );
	}

	void DrawInstancedIndexedPrimitiveIndirect( ePrimitiveType primitive )
	{
		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( chunkType );
		}
		GetDeviceContext()->IASetInputLayout(GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ));

		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->DrawIndexedInstancedIndirect( GetD3DBuffer(dd.m_indirectArgs), 0 );
	}

	void DrawInstancedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 numVertices, Uint32 primitiveCount, Uint32 instancesCount )
	{
		RED_UNUSED(numVertices);
		RED_UNUSED(primitiveCount);

		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		if ( !dd.m_HullShader.isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( chunkType );
		}
		GetDeviceContext()->IASetInputLayout(GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ));
		
		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->DrawInstanced( numVertices, instancesCount, baseVertexIndex, 0 );
	}

	void DrawInstancedPrimitiveNoBuffers( ePrimitiveType primitive, Uint32 vertexCount, Uint32 instancesCount )
	{
		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		// Unbind streams
		ID3D11Buffer* nullBuffers[] = { NULL, NULL };
		Uint32 nullStrideOffset[2] = { 0, 0 };
		GetDeviceContext()->IASetVertexBuffers( 0, 2, nullBuffers, nullStrideOffset, nullStrideOffset );
		dd.m_VertexBuffers[0] = dd.m_VertexBuffers[1] = BufferRef::Null();

		// FIXME: Better solution storing it in the dd device data struct?
		ID3D11InputLayout* oldInputLayout = nullptr;
		GetDeviceContext()->IAGetInputLayout( &oldInputLayout );

		GetDeviceContext()->IASetInputLayout( NULL );

		GetDeviceContext()->IASetPrimitiveTopology( Map( primitive ) );
		GetDeviceContext()->DrawInstanced( vertexCount, instancesCount, 0, 0 );

		if ( oldInputLayout )
		{
			GetDeviceContext()->IASetInputLayout( oldInputLayout );
			oldInputLayout->Release();
		}
	}

	void DrawIndexedPrimitiveRaw( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount )
	{
		RED_UNUSED(numVertices);
		RED_UNUSED(minIndex);

		SDeviceData &dd = GetDeviceData();
#ifdef NO_GPU_ASSERTS
		RED_UNUSED( dd );
#endif
		GPUAPI_ASSERT( ( IsControlPointPatch( primitive ) == !dd.m_HullShader.isNull() ) );

		Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->DrawIndexed( indexNum, startIndex, baseVertexIndex );
	}

	void DrawInstancedIndexedPrimitiveRaw( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount, Uint32 instancesCount )
	{
		RED_UNUSED(numVertices);
		RED_UNUSED(minIndex);

		SDeviceData &dd = GetDeviceData();
#ifdef NO_GPU_ASSERTS
		RED_UNUSED( dd );
#endif
		GPUAPI_ASSERT( ( IsControlPointPatch( primitive ) == !dd.m_HullShader.isNull() ) );

		Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->DrawIndexedInstanced ( indexNum, instancesCount, startIndex, baseVertexIndex, 0 );
	}

	void DrawSystemPrimitive( ePrimitiveType primitive, Uint32 primitiveCount, eBufferChunkType vertexType, const void *vertexBuffer )
	{	
		GPUAPI_ASSERT( vertexBuffer );

		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		if ( !dd.m_HullShader.isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Uint32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		Int32 vertexDataSize = vertexCount * GetChunkTypeStride( vertexType );

		eBufferLockFlag lockType = BLF_NoOverwrite;

		if ( dd.m_currentVertexWritePosition + vertexDataSize > g_drawPrimitiveUPBufferSize )
		{
			lockType = BLF_Discard;
			dd.m_currentVertexWritePosition = 0;
		}

		// Bind vertex buffer, map it, and copy data to it
		{
			void* lockedBufferPtr = GpuApi::LockBuffer( dd.m_drawPrimitiveUPVertexBuffer, lockType, dd.m_currentVertexWritePosition, vertexDataSize );
			Red::System::MemoryCopy( lockedBufferPtr, vertexBuffer, vertexDataSize );
			GpuApi::UnlockBuffer( dd.m_drawPrimitiveUPVertexBuffer );

			const Uint32 stride = GetChunkTypeStride( vertexType );
			Uint32 offset = dd.m_currentVertexWritePosition;
			ID3D11Buffer* vbuffer = GetDeviceData().m_Buffers.Data( dd.m_drawPrimitiveUPVertexBuffer ).m_pBufferResource;
			GetDeviceContext()->IASetVertexBuffers( 0, 1, &vbuffer, &stride, &offset );

			dd.m_VertexBuffers[0] = dd.m_drawPrimitiveUPVertexBuffer;
			dd.m_VertexLayout = GetVertexLayoutForChunkType( vertexType );
		}

		GetDeviceContext()->IASetInputLayout( GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ) );

		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->Draw( vertexCount, 0 );

		dd.m_currentVertexWritePosition += vertexDataSize;
		//dex++: bug fix
		GPUAPI_ASSERT(dd.m_currentVertexWritePosition <= g_drawPrimitiveUPBufferSize);
		//dex--
	}

	void DrawSystemIndexedPrimitive( ePrimitiveType primitive, Uint32 minVertexIndex, Uint32 numVertices, Uint32 primitiveCount, const Uint16 *indexBuffer, eBufferChunkType vertexType, const void *vertexBuffer )
	{
		RED_UNUSED(minVertexIndex);

		GPUAPI_ASSERT( indexBuffer && vertexBuffer );

		SDeviceData &dd = GetDeviceData();

		UpdateConstantBuffers();

		if ( !dd.m_HullShader.isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Int32 vertexDataSize = numVertices * GetChunkTypeStride( vertexType );

		Int32 numIndices = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		
		Int32 indexDataSize = numIndices * sizeof( Uint16 );

		eBufferLockFlag vertexLockType = BLF_NoOverwrite;
		eBufferLockFlag indexLockType = BLF_NoOverwrite;

		if ( dd.m_currentVertexWritePosition + vertexDataSize > g_drawPrimitiveUPBufferSize )
		{
			vertexLockType = BLF_Discard;
			dd.m_currentVertexWritePosition = 0;
		}

		if ( dd.m_currentIndexWritePosition + indexDataSize > g_drawPrimitiveUPBufferSize )
		{
			indexLockType = BLF_Discard;
			dd.m_currentIndexWritePosition = 0;
		}

		// Bind vertex buffer, map it, and copy data to it
		{
			void* lockedBufferPtr = GpuApi::LockBuffer( dd.m_drawPrimitiveUPVertexBuffer, vertexLockType, dd.m_currentVertexWritePosition, vertexDataSize );
			Red::System::MemoryCopy( lockedBufferPtr, vertexBuffer, vertexDataSize );
			GpuApi::UnlockBuffer( dd.m_drawPrimitiveUPVertexBuffer );

			const Uint32 stride = GetChunkTypeStride( vertexType );
			Uint32 offset = dd.m_currentVertexWritePosition;
			ID3D11Buffer* vbuffer = GetDeviceData().m_Buffers.Data( dd.m_drawPrimitiveUPVertexBuffer ).m_pBufferResource;
			GetDeviceContext()->IASetVertexBuffers( 0, 1, &vbuffer, &stride, &offset );

			dd.m_VertexBuffers[0] = dd.m_drawPrimitiveUPVertexBuffer;
			dd.m_VertexLayout = GetVertexLayoutForChunkType( vertexType );
		}
		
		// Bind index buffer, map it, and copy data to it
		{
			void* lockedBufferPtr = GpuApi::LockBuffer( dd.m_drawPrimitiveUPIndexBuffer, indexLockType, dd.m_currentIndexWritePosition, indexDataSize );
			Red::System::MemoryCopy( lockedBufferPtr, indexBuffer, indexDataSize );
			GpuApi::UnlockBuffer( dd.m_drawPrimitiveUPIndexBuffer );
			GetDeviceContext()->IASetIndexBuffer( GetDeviceData().m_Buffers.Data(dd.m_drawPrimitiveUPIndexBuffer).m_pBufferResource, DXGI_FORMAT_R16_UINT, dd.m_currentIndexWritePosition );
			dd.m_IndexBuffer = dd.m_drawPrimitiveUPIndexBuffer;
		}

		GetDeviceContext()->IASetInputLayout( GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ) );

		GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		GetDeviceContext()->DrawIndexed( numIndices, 0, 0 );

		dd.m_currentVertexWritePosition += vertexDataSize;
		dd.m_currentIndexWritePosition += indexDataSize;

		//dex++: bug fix
		GPUAPI_ASSERT(dd.m_currentVertexWritePosition <= g_drawPrimitiveUPBufferSize);
		GPUAPI_ASSERT(dd.m_currentIndexWritePosition <= g_drawPrimitiveUPBufferSize);
		//dex--
	}

	void DispatchCompute( ShaderRef& computeShader, Uint32 x, Uint32 y, Uint32 z )
	{
		GPUAPI_ASSERT( !computeShader.isNull() );

		if (!computeShader.isNull())
		{
			GPUAPI_ASSERT( GetDeviceData().m_Shaders.Data(computeShader).m_type == ComputeShader );
			GetDeviceContext()->CSSetShader( (ID3D11ComputeShader*) GetDeviceData().m_Shaders.Data(computeShader).m_pShader, NULL, 0 );
			GetDeviceContext()->Dispatch( x, y, z );
		}

		GetDeviceContext()->CSSetShader( NULL, NULL, 0 );
	}

#ifndef RED_PLATFORM_CONSOLE

	Bool ShouldDecompressBeforeSaving( const eTextureFormat format, const eTextureSaveFormat saveFormat )
	{
		Bool shouldDecompressBecauseOfSaveFormat = saveFormat == SAVE_FORMAT_BMP || saveFormat == SAVE_FORMAT_JPG || saveFormat == SAVE_FORMAT_PNG || saveFormat == SAVE_FORMAT_TGA;

		// list of formats that are savable to WIC in current DirectX Tex version
		// this can be determined by looking into _DXGIToWIC function in DirectXTex lib
		DXGI_FORMAT savableFormats[ ] = 
		{ 
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			DXGI_FORMAT_D32_FLOAT,
			DXGI_FORMAT_D16_UNORM,
			DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
			DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
			DXGI_FORMAT_R32G32B32_FLOAT,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_B8G8R8A8_UNORM,
			DXGI_FORMAT_B8G8R8X8_UNORM,
			DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
			DXGI_FORMAT_B5G5R5A1_UNORM,
			DXGI_FORMAT_B5G6R5_UNORM,
			DXGI_FORMAT_R32_FLOAT,
			DXGI_FORMAT_R16_FLOAT,
			DXGI_FORMAT_R16_UNORM,
			DXGI_FORMAT_R8_UNORM,
			DXGI_FORMAT_A8_UNORM,
			DXGI_FORMAT_R1_UNORM,
		};

		DXGI_FORMAT dxgiFormat = Map( format );

		Bool shouldDecompressBecauseOfTextureFormat = true;
		for ( Uint32 i = 0; i < ARRAY_COUNT(savableFormats); ++i )
		{
			if ( dxgiFormat == savableFormats[i] )
			{
				// texture format can be saved directly, no need to decompress
				shouldDecompressBecauseOfTextureFormat = false;
				break;
			}
		}

		return shouldDecompressBecauseOfSaveFormat && shouldDecompressBecauseOfTextureFormat;
	}

	Bool SaveBufferToFile( Uint32* targetBuffer, const Char* fileName, const Uint32 width, const Uint32 height, eTextureSaveFormat saveFormat, const Bool normalize /*=false*/, const Uint32 denominator /*=1*/, eTextureFormat srcDataFormat /*=TEXFMT_R8G8B8X8*/ )
	{
		if ( normalize )
		{
			GPUAPI_ASSERT( denominator != 0 );
		}

		GpuApi::TextureDesc desc;
		desc.width = width;
		desc.height = height;
		desc.format = srcDataFormat;
		desc.usage = TEXUSAGE_Dynamic | TEXUSAGE_Samplable;
		desc.initLevels = 1;
		TextureRef texture = CreateTexture( desc, GpuApi::TEXG_System );

		D3D11_MAPPED_SUBRESOURCE mappedTexture;
		GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( GetDeviceData().m_Textures.Data(texture).m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTexture ) );

		// Fill texture
		for ( Uint32 iy = 0; iy < height; ++iy )
		{
			Uint8* dst = (Uint8*)mappedTexture.pData + iy * mappedTexture.RowPitch;
			const Uint32 *src = &targetBuffer[ iy * width * 3 ];

			for ( Uint32 ix = 0; ix < width; ++ix )
			{
				if ( normalize )
				{
					*(dst++) = static_cast<Uint8>( ( *(src++) ) / denominator );
					*(dst++) = static_cast<Uint8>( ( *(src++) ) / denominator );
					*(dst++) = static_cast<Uint8>( ( *(src++) ) / denominator );
					*(dst++) = 255;
				}
				else
				{
					*(dst++) = static_cast<Uint8>( *(src++) );
					*(dst++) = static_cast<Uint8>( *(src++) );
					*(dst++) = static_cast<Uint8>( *(src++) );
					*(dst++) = 255;
				}
			}
		}

		GetDeviceContext()->Unmap( GetDeviceData().m_Textures.Data(texture).m_pTexture, 0 );
		
		Bool res = SaveTextureToFile( texture, fileName, saveFormat );

		Release( texture );

		return res;
	}

	Bool SaveTextureToFile( const TextureRef& textureRef, const Char* fileName, const eTextureSaveFormat format )
	{
		// save data
		DirectX::ScratchImage* scratchImage = new DirectX::ScratchImage();
		HRESULT captureRes = DirectX::CaptureTexture(GetDevice(), GetDeviceContext(), GetDeviceData().m_Textures.Data(textureRef).m_pTexture, *scratchImage);
		HRESULT saveRes = S_FALSE;
		if ( captureRes == S_OK )
		{
			//! HACK - because COM component did not initialize before function SaveToFile
			static Bool isCoInitialized = false;
			if ( !isCoInitialized )
			{
				switch( format )
				{
				case SAVE_FORMAT_BMP:
				case SAVE_FORMAT_JPG:
				case SAVE_FORMAT_PNG:
					HRESULT hr = S_FALSE;
					hr = CoInitialize( NULL );
					GPUAPI_ASSERT( hr == S_OK );
					isCoInitialized = true;
					break;
				}
			}

			switch( format )
			{
			case SAVE_FORMAT_DDS:
				saveRes = DirectX::SaveToDDSFile(scratchImage->GetImages(), scratchImage->GetImageCount(), scratchImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, fileName);
				break;
			case SAVE_FORMAT_BMP:
				saveRes = DirectX::SaveToWICFile(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), fileName );
				break;
			case SAVE_FORMAT_JPG:
				saveRes = DirectX::SaveToWICFile(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), fileName );
				break;
			case SAVE_FORMAT_PNG:
				saveRes = DirectX::SaveToWICFile(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), fileName );
				break;
			}
		}

		if ( captureRes != S_OK || saveRes != S_OK )
		{
			scratchImage->Release();
			GPUAPI_HALT( "Texture saving failed" );
			return false;
		}
		scratchImage->Release();
		return true;
	}

	Bool SaveTextureToMemory( const TextureRef& textureRef, const eTextureSaveFormat format, void* buffer, size_t bufferSize, size_t* outBufferSizeWritten )
	{
		// save data
		DirectX::Blob blob; 
		DirectX::ScratchImage* scratchImage = new DirectX::ScratchImage();
		HRESULT captureRes = DirectX::CaptureTexture(GetDevice(), GetDeviceContext(), GetDeviceData().m_Textures.Data(textureRef).m_pTexture, *scratchImage);
		HRESULT saveRes = S_FALSE;
		if ( captureRes == S_OK )
		{
			//! HACK - because COM component did not initialize before function
			static Bool isCoInitialized = false;
			if ( !isCoInitialized )
			{
				switch( format )
				{
				case SAVE_FORMAT_BMP:
				case SAVE_FORMAT_JPG:
				case SAVE_FORMAT_PNG:
					HRESULT hr = S_FALSE;
					hr = CoInitialize( NULL );
					GPUAPI_ASSERT( hr == S_OK );
					isCoInitialized = true;
					break;
				}
			}

			switch( format )
			{
			case SAVE_FORMAT_DDS:
				saveRes = DirectX::SaveToDDSMemory(scratchImage->GetImages(), scratchImage->GetImageCount(), scratchImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
				break;
			case SAVE_FORMAT_BMP:
				saveRes = DirectX::SaveToWICMemory(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), blob );
				break;
			case SAVE_FORMAT_JPG:
				saveRes = DirectX::SaveToWICMemory(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), blob );
				break;
			case SAVE_FORMAT_PNG:
				saveRes = DirectX::SaveToWICMemory(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), blob );
				break;
			}
		}

		if ( captureRes != S_OK || saveRes != S_OK )
		{
			GPUAPI_HALT( "Texture saving failed" );
			return false;
		}
		scratchImage->Release();

		// TODO: is it really required to copy this memory back and forth so many times??? it's ridiculous.
		if ( blob.GetBufferSize() <= bufferSize )
		{
			Red::MemoryCopy( buffer, blob.GetBufferPointer(), bufferSize );
			if (outBufferSizeWritten)
			{
				*outBufferSizeWritten = blob.GetBufferSize();
			}
		}
		else
		{
			GPUAPI_HALT( "Texture saving failed" );
			return false;
		}

		blob.Release();
		return true;
	}

	Bool SaveTextureToFile( const TextureDataDesc& image, const Char* fileName, const eTextureSaveFormat saveFormat )
	{
		Bool shouldDecompressBeforeSaving = ShouldDecompressBeforeSaving( image.format, saveFormat );
		Uint8* decompressedData = NULL;

		DirectX::Image img;
		img.width = image.width;
		img.height = image.height;

		if ( shouldDecompressBeforeSaving )
		{
			TextureDataDesc decompressedImage;
			decompressedImage.data = &decompressedData;
			decompressedImage.format = TEXFMT_R8G8B8A8;
			bool decompOK = DecompressImage( image, decompressedImage );
			GPUAPI_ASSERT( decompOK, TXT( "Error decompressing texture before saving." ) );
			img.pixels = decompressedData;
			img.rowPitch = decompressedImage.rowPitch;
			img.slicePitch = decompressedImage.slicePitch;
			img.format = Map( decompressedImage.format );
		}
		else
		{
			// use given data
			img.rowPitch = image.rowPitch;
			img.slicePitch = image.slicePitch;
			img.format = Map( image.format );
			img.pixels = *image.data;
		}

		HRESULT hr = S_FALSE;
		switch ( saveFormat )
		{
		case SAVE_FORMAT_DDS:
			hr = DirectX::SaveToDDSFile( img, DirectX::DDS_FLAGS_NONE, fileName );
			break;
		case SAVE_FORMAT_BMP:
			hr = DirectX::SaveToWICFile( img, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), fileName );
			break;
		case SAVE_FORMAT_JPG:
			hr = DirectX::SaveToWICFile( img, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), fileName );
			break;
		case SAVE_FORMAT_PNG:
			hr = DirectX::SaveToWICFile( img, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), fileName );
			break;
		case SAVE_FORMAT_TGA:
			hr = DirectX::SaveToTGAFile( img, fileName );
			break;
		}

		// regardless of the saving, free data buffer for decompressed texture
		if ( shouldDecompressBeforeSaving )
		{
			GPUAPI_ASSERT( decompressedData );
			GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, decompressedData );
		}

		if ( FAILED( hr ) )
		{
			if ( hr == ERROR_NOT_SUPPORTED )
			{
				GPUAPI_HALT( "Error exporting texture: format not supported." );
			}
			else
			{
				GPUAPI_HALT( "Error exporting texture." );
			}
			return false;
		}
		return true;
	}

	void TakeScreenshotPCImplementation( Uint32 screenshotWidth, Uint32 screenshotHeight, eTextureSaveFormat format, const Char* fileName, void* buffer, size_t bufferSize, size_t* outBufferSizeWritten )
	{
		Uint32 viewportWidth = GetViewport().width;
		Uint32 viewportHeight = GetViewport().height;

		// get backbuffer params
		TextureDesc desc;
		GetTextureDesc( GetBackBufferTexture(), desc );

		// create copy of backbuffer, we can read from it on the CPU
		desc.usage = TEXUSAGE_Staging;
		TextureRef backBufferCopy = CreateTexture( desc, GpuApi::TEXG_System );

		// get appropriate d3d pointers
		const STextureData &dstData = GetDeviceData().m_Textures.Data( backBufferCopy );
		const STextureData &srcData = GetDeviceData().m_Textures.Data( GetBackBufferTexture() );

		// copy data from GPU resource to resource available to CPU
		GetDeviceContext()->CopyResource( dstData.m_pTexture, srcData.m_pTexture );

		// create final texture, we are able to write to in on the CPU side
		desc.width = screenshotWidth;
		desc.height = screenshotHeight;
		desc.usage = TEXUSAGE_Dynamic | TEXUSAGE_Samplable;
		TextureRef finalTexture = CreateTexture( desc, GpuApi::TEXG_System );

		// map textures
		D3D11_MAPPED_SUBRESOURCE mappedFinalTexture;
		GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( GetDeviceData().m_Textures.Data(finalTexture).m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedFinalTexture ) );
		D3D11_MAPPED_SUBRESOURCE mappedBackBuffer;
		GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( dstData.m_pTexture, 0, D3D11_MAP_READ, 0, &mappedBackBuffer ) );

		Uint32 supersampledPixelsY = viewportHeight / screenshotHeight;
		if ( supersampledPixelsY == 0 )
		{
			supersampledPixelsY = 1;
		}
		Uint32 supersampledPixelsX = viewportWidth / screenshotWidth;
		if ( supersampledPixelsX == 0 )
		{
			supersampledPixelsX = 1;
		}

		Float trueSupersampledPixelsY = ((Float)screenshotHeight / (Float)viewportHeight);
		Float trueSupersampledPixelsX = ((Float)screenshotWidth / (Float)viewportWidth );

		// Copy data to pixel buffer
		for ( Uint32 iy=0; iy<screenshotHeight; iy++ )
		{
			Uint8 *dst = (Uint8*)mappedFinalTexture.pData + iy * mappedFinalTexture.RowPitch;

			for ( Uint32 ix=0; ix<screenshotWidth; ix++ )
			{
				Uint32 accumRed = 0, accumBlue = 0, accumGreen = 0;

				Float trueTiltX = 0.0f, trueTiltY = 0.0f;

				for ( Uint32 j = 0; j < supersampledPixelsY; ++j, trueTiltY += trueSupersampledPixelsY )
				{
					Uint8 *src = (Uint8*)mappedBackBuffer.pData + ((iy * viewportHeight) / screenshotHeight + (Uint32)trueTiltY) * mappedBackBuffer.RowPitch;

					for ( Uint32 i = 0; i < supersampledPixelsX; ++i, trueTiltX += trueSupersampledPixelsX )
					{
						Uint8 *srcX = src + 4 * ((ix * viewportWidth) / screenshotWidth + (Uint32)trueTiltX);

						accumRed	+= *(srcX++);
						accumGreen	+= *(srcX++);
						accumBlue	+= *(srcX++);
						srcX++;
					}
				}

				*(dst++) = (Uint8)( accumRed / (supersampledPixelsX * supersampledPixelsY ) );
				*(dst++) = (Uint8)( accumGreen / (supersampledPixelsX * supersampledPixelsY ) );
				*(dst++) = (Uint8)( accumBlue / (supersampledPixelsX * supersampledPixelsY ) );
				*(dst++) = 255;
			}
		}

		GetDeviceContext()->Unmap( dstData.m_pTexture, 0 );
		GetDeviceContext()->Unmap( GetDeviceData().m_Textures.Data(finalTexture).m_pTexture, 0 );

		if ( fileName )
		{
			SaveTextureToFile( finalTexture, fileName, format );
		}

		if ( buffer && bufferSize > 0 )
		{
			SaveTextureToMemory( finalTexture, format, buffer, bufferSize, outBufferSizeWritten );
		}

		// destroy temporary textures
		Release( backBufferCopy );
		Release( finalTexture );
	}

	void TakeScreenshot( Uint32 screenshotWidth, Uint32 screenshotHeight, eTextureSaveFormat format, const Char* fileName, void* buffer, size_t bufferSize, size_t* outBufferSizeWritten )
	{
		TakeScreenshotPCImplementation( screenshotWidth, screenshotHeight, format, fileName, buffer, bufferSize, outBufferSizeWritten );
	}

#endif

	void GrabBackBuffer( Uint32* targetBuffer, const Rect& r, const Uint32 fullHDChunks, const Uint32 chunkNumX, const Uint32 chunkNumY, const Uint32 fullHDResX )
	{
		// Get backbuffer desc
		GpuApi::TextureDesc desc;
		GpuApi::GetTextureDesc( GetBackBufferTexture(), desc );

		// create a staging copy
		desc.usage = GpuApi::TEXUSAGE_Staging;
		TextureRef backBufferCopy = CreateTexture( desc, GpuApi::TEXG_System );

		// get appropriate d3d pointers
		const STextureData &dstData = GetDeviceData().m_Textures.Data( backBufferCopy );
		const STextureData &srcData = GetDeviceData().m_Textures.Data( GetBackBufferTexture() );

		// copy data from GPU resource to resource available to CPU
		GetDeviceContext()->CopyResource( dstData.m_pTexture, srcData.m_pTexture );

		D3D11_MAPPED_SUBRESOURCE mappedBackBuffer;
		GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( dstData.m_pTexture, 0, D3D11_MAP_READ, 0, &mappedBackBuffer ) );

		const Uint32 height = r.bottom - r.top;
		const Uint32 width = r.right - r.left;

		// Copy data to pixel buffer
		for ( Uint32 iy = 0; iy < height; ++iy )
		{
			Uint8* src = (Uint8*)mappedBackBuffer.pData + ( iy + r.top ) * mappedBackBuffer.RowPitch + r.left * 4;
			Uint32 pixelYPosMultiplied = ( iy * fullHDChunks + chunkNumY ) * fullHDResX;
			Uint32 *dst = &targetBuffer[ ( pixelYPosMultiplied + chunkNumX ) * 3 ];
			
			for ( Uint32 ix = 0; ix < width; ++ix, dst += fullHDChunks * 3 )
			{
				*(dst)		+= *(src++);
				*(dst + 1)	+= *(src++);
				*(dst + 2)	+= *(src++);
				src++;
			}
		}
		
		GetDeviceContext()->Unmap( GetDeviceData().m_Textures.Data( backBufferCopy ).m_pTexture, 0 );
		Release( backBufferCopy );
	}
		
	void Flush()
	{
#ifdef RED_PLATFORM_DURANGO
		ID3D11DeviceContextX* context = (ID3D11DeviceContextX*)GetDeviceContext();
		return context->InsertWaitUntilIdle( 0 );
#else
		GetDeviceContext()->Flush();
		return;
#endif
	}

	Uint64 InsertFence()
	{
#ifdef RED_PLATFORM_DURANGO
		ID3D11DeviceContextX* context = (ID3D11DeviceContextX*)GetDeviceContext();
		return context->InsertFence( D3D11_INSERT_FENCE_NO_KICKOFF );
#else
		SDeviceData &dd = GetDeviceData();
		if( !dd.m_renderFence.isNull() )
		{
			AddRef( dd.m_renderFence );
			return dd.m_renderFence.id;
		}
		else
		{
			return 0;
		}
#endif
	}

	void InsertWaitOnFence( Uint64 fence )
	{
		// On consoles, this just inserts a wait into the command list, not block the CPU. Since PC doesn't have that
		// sort of synchronization (it's all done for us), there's nothing to do here.

#ifdef RED_PLATFORM_DURANGO
		ID3D11DeviceContextX* context = (ID3D11DeviceContextX*)GetDeviceContext();
		return context->InsertWaitOnFence( D3D11_INSERT_FENCE_NO_KICKOFF, fence );
#endif
	}

	Bool IsFencePending( Uint64 fence )
	{
#ifdef RED_PLATFORM_DURANGO
		ID3D11DeviceX* device = (ID3D11DeviceX*)GetDevice();
		return device->IsFencePending( fence ) != 0;
#else
		if (fence == 0)
		{
			return false;
		}

		GPUAPI_ASSERT( fence <= (Uint64)UINT32_MAX );
		Uint32 queryId = (Uint32)fence;
		SQueryData& queryData = GetDeviceData().m_Queries.Data( queryId );
		// this should not flush the pipeline
		HRESULT hr = GetDeviceContext()->GetData( queryData.m_pQuery, nullptr, 0, D3D11_ASYNC_GETDATA_DONOTFLUSH );
		return hr != S_OK;
#endif
	}

	void ReleaseFence( Uint64 fence )
	{
#ifndef RED_PLATFORM_DURANGO

		if (fence == 0)
		{
			return;
		}

		GPUAPI_ASSERT( fence <= (Uint64)UINT32_MAX );
		Uint32 queryId = (Uint32)fence;
		Release( QueryRef( queryId ) );
#endif
	}

	void InsertWriteToMemory( volatile Uint64* dstAddress, Uint64 value )
	{
#ifdef RED_PLATFORM_DURANGO
		SDeviceData &dd = GetDeviceData();

		// make sure that all the work is done before signaling the CPU
		dd.m_pImmediateContext->InsertWaitUntilIdle( 0 );

		// D3D doesn't let us write uint64 in one... so...
		volatile Uint32* dstAddress32 = reinterpret_cast< volatile Uint32* >( dstAddress );
		dd.m_pImmediateContext->FillMemoryWithValue( (void*)dstAddress32, sizeof( Uint32 ), (Uint32)( value >> 32 ) );
		dd.m_pImmediateContext->FillMemoryWithValue( (void*)(dstAddress32 + 1), sizeof( Uint32 ), (Uint32)( value & 0x00000000ffffffff ) );

		// Insert a fence after the copy and kickoff the DMA engine
		// DO NOT REMOVE THIS IF YOU DON'T KNOW WHY IT'S HERE!!!
		Uint64 afterFence = dd.m_pImmediateContext->InsertFence( 0 );
#else
		GPUAPI_HALT("DMA is not available on this platform");
#endif
	}

	void AddSyncComputeTaskToQueue( const ComputeTaskDesc& computeTaskDesc )
	{
		SDeviceData &dd = GetDeviceData();

#ifdef RED_PLATFORM_DURANGO
		ID3D11DeviceContextX* dc = dd.m_pImmediateContext;
#else
		ID3D11DeviceContext* dc = dd.m_pImmediateContext;
#endif

		if ( computeTaskDesc.m_inputTextureCount > 0 )
		{
			ID3DShaderResourceView* srvs[16] = {};
			for ( Uint32 i = 0; i < computeTaskDesc.m_inputTextureCount; ++i )
			{
				srvs[i] = GetD3DShaderResourceView(computeTaskDesc.m_inputTextures[i]);
			}
			dc->CSSetShaderResources( 0, computeTaskDesc.m_inputTextureCount, &srvs[0] );
		}

		if ( computeTaskDesc.m_constantBufferCount > 0 )
		{
			ID3D11Buffer* constantBuffers[16] = {};
			for ( Uint32 i = 0; i < computeTaskDesc.m_constantBufferCount; ++i )
			{
				constantBuffers[i] = GetD3DBuffer(computeTaskDesc.m_constantBuffers[i]);
			}
			dc->CSSetConstantBuffers( 0, computeTaskDesc.m_constantBufferCount, &constantBuffers[0] );
		}

		ID3D11UnorderedAccessView* uav = GetD3DUnorderedAccessView( computeTaskDesc.m_uav );
		dc->CSSetUnorderedAccessViews( computeTaskDesc.m_uavIndex, 1, &uav, 0 );

		dc->CSSetShader( (ID3D11ComputeShader*) GetDeviceData().m_Shaders.Data(computeTaskDesc.m_shader).m_pShader, NULL, 0 );

#ifdef RED_PLATFORM_DURANGO
		// No proper dependency tracking but compute tasks can depend on each other so we need this
		dc->GpuSendPipelinedEvent( D3D11X_GPU_PIPELINED_EVENT_CS_PARTIAL_FLUSH );
#endif

		dc->Dispatch( computeTaskDesc.m_threadGroupX, computeTaskDesc.m_threadGroupY, computeTaskDesc.m_threadGroupZ );

		// unset everything to avoid any issues
		{
			dc->CSSetShader( nullptr, nullptr, 0 );

			uav = nullptr;
			dc->CSSetUnorderedAccessViews( computeTaskDesc.m_uavIndex, 1, &uav, 0 );

			if ( computeTaskDesc.m_inputTextureCount > 0 )
			{
				ID3DShaderResourceView* nullsrvs[16] = {};
				dc->CSSetShaderResources( 0, computeTaskDesc.m_inputTextureCount, &nullsrvs[0] );
			}
		}
	}

	void AddAsyncComputeTaskToQueue( const ComputeTaskDesc& computeTaskDesc )
	{
#ifdef RED_PLATFORM_DURANGO
		SDeviceData &dd = GetDeviceData();

		ID3D11DeviceContextX* dc = dd.m_pImmediateContext;
		ID3D11ComputeContextX* cc = dd.m_computeContext;


		// Track if we need to sync with the graphics context. This could happen if we need to decompress some texture on
		// the graphics context, so that the compute context will wait for that to finish.
		Bool needSync = computeTaskDesc.m_insertSync;

		if ( computeTaskDesc.m_inputTextureCount > 0 )
		{
			Bool didBindForDecompress = false;

			Bool anyCompressed = false;
			ID3DShaderResourceView* srvs[16] = {};
			for ( Uint32 i = 0; i < computeTaskDesc.m_inputTextureCount; ++i )
			{
				srvs[i] = GetD3DShaderResourceView(computeTaskDesc.m_inputTextures[i]);

				Uint32 texCompressionFlags = dc->GetResourceCompression( GetD3DTextureBase( computeTaskDesc.m_inputTextures[i] ) );
				if ( texCompressionFlags != 0 )
				{
					// Bind so that it gets decompressed. Bind to the very last slot, so we aren't likely going to interfere
					// with anything the renderer may have bound and wants to keep there.
					dc->CSSetShaderResources( D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1, 1, &srvs[ i ] );
					didBindForDecompress = true;
				}
				
			}
			cc->CSSetShaderResources( 0, computeTaskDesc.m_inputTextureCount, &srvs[0] );

			// If we bound something to the graphics context for decompression, unbind here so we don't cause
			// it to be kept alive with this extra reference.
			if ( didBindForDecompress )
			{
				ID3D11ShaderResourceView* nullSRV = nullptr;
				dc->CSSetShaderResources( D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1, 1, &nullSRV );

				// If we had to decompress a texture, we need the compute context to wait for that to finish.
				needSync = true;
			}
		}

		if ( computeTaskDesc.m_constantBufferCount > 0 )
		{
			ID3D11Buffer* constantBuffers[16] = {};
			for ( Uint32 i = 0; i < computeTaskDesc.m_constantBufferCount; ++i )
			{
				constantBuffers[i] = GetD3DBuffer(computeTaskDesc.m_constantBuffers[i]);
			}
			cc->CSSetConstantBuffers( 0, computeTaskDesc.m_constantBufferCount, &constantBuffers[0] );
		}

		ID3D11UnorderedAccessView* uav = GetD3DUnorderedAccessView( computeTaskDesc.m_uav );
		cc->CSSetUnorderedAccessViews( computeTaskDesc.m_uavIndex, 1, &uav, 0 );

		Uint32 uavCompressionFlags = dc->GetResourceCompression( GetD3DTextureBase( computeTaskDesc.m_uav ) );
		if ( uavCompressionFlags != 0 )
		{
			// Bind so that it gets decompressed. Bind to the very last slot, so we aren't likely going to interfere
			// with anything the renderer may have bound and wants to keep there. Immediately unbind so we don't cause
			// it to be kept alive with this extra reference.
			dc->CSSetUnorderedAccessViews( D3D11_1_UAV_SLOT_COUNT - 1, 1, &uav, 0 );

			ID3D11UnorderedAccessView* nullUAV = nullptr;
			dc->CSSetUnorderedAccessViews( D3D11_1_UAV_SLOT_COUNT - 1, 1, &nullUAV, 0 );
			needSync = true;
		}

		cc->CSSetShader( (ID3D11ComputeShader*) GetDeviceData().m_Shaders.Data(computeTaskDesc.m_shader).m_pShader, NULL, 0 );


		if ( needSync )
		{
			// Flush CB, L1, K$, so that any rendering or buffer updates will be visible to compute.
			dc->FlushGpuCacheRange( D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_KCACHE_INVALIDATE, nullptr, 0 );

			Uint64 fence = dc->InsertFence( D3D11_INSERT_FENCE_NO_KICKOFF );
			cc->InsertWaitOnFence( D3D11_INSERT_FENCE_NO_KICKOFF, fence );
		}


		// No proper dependency tracking but compute tasks can depend on each other so we need this
		cc->GpuSendPipelinedEvent( D3D11X_GPU_PIPELINED_EVENT_CS_PARTIAL_FLUSH );

		cc->Dispatch( computeTaskDesc.m_threadGroupX, computeTaskDesc.m_threadGroupY, computeTaskDesc.m_threadGroupZ );


		// Flush L1 after dispatch, in case next compute runs on a different CU or something.
		cc->FlushGpuCachesBottomOfPipe( D3D11_FLUSH_TEXTURE_L1_INVALIDATE );


		// unset everything to avoid any issues
		{
			cc->CSSetShader( nullptr, nullptr, 0 );

			uav = nullptr;
			cc->CSSetUnorderedAccessViews( computeTaskDesc.m_uavIndex, 1, &uav, 0 );

			if ( computeTaskDesc.m_inputTextureCount > 0 )
			{
				ID3DShaderResourceView* nullsrvs[16] = {};
				cc->CSSetShaderResources( 0, computeTaskDesc.m_inputTextureCount, &nullsrvs[0] );
			}
		}
#else
		GPUAPI_HALT("Not available on this platform");
#endif
	}

	Uint64 KickoffAsyncComputeTasks()
	{
#ifdef RED_PLATFORM_DURANGO
		SDeviceData &dd = GetDeviceData();
		ID3D11ComputeContextX* cc = dd.m_computeContext;

		// Flush L1/L2, since we've probably written to UAV. Don't need it between individual dispatches, because the
		// caches should generally stay consistent (if dispatchA is writing to a UAV, and then dispatchB reads from that
		// as a texture, it stays within L1/L2).
		cc->FlushGpuCachesBottomOfPipe( D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE );

		return cc->InsertFence( 0 ); //not using the NO_KICKOFF flag means we are kicking off the work on the compute context
#else
		GPUAPI_HALT("Not available on this platform");
		return 0;
#endif
	}

	volatile Uint64* AllocateValue()
	{
#ifdef RED_PLATFORM_DURANGO
		// This can be any coherant garlic-visible pool. ConstantBuffers fits for now
		return (volatile Uint64*)GPU_API_ALLOCATE( GpuMemoryPool_ConstantBuffers, MC_Label, sizeof( Uint64 ), sizeof( Uint64 ) );
#else
		GPUAPI_HALT("Not available on this platform");
		return nullptr;
#endif
	}

	void DeallocateValue( volatile Uint64* value )
	{
#ifdef RED_PLATFORM_DURANGO
		// This can be any coherant garlic-visible pool. ConstantBuffers fits for now
		GPU_API_FREE( GpuMemoryPool_ConstantBuffers, MC_Label, (void*)value );
#else
		GPUAPI_HALT("Not available on this platform");
#endif
	}

#ifndef RED_FINAL_BUILD
	void DumpResourceStats()
	{
#ifdef GPU_API_DEBUG_PATH
		char buffer[512];

		// Dump list of loaded textures
		Uint32 numTextures = 0;
		{
			for ( Uint32 i=0; i<(Uint32)GetDeviceData().m_Textures._MaxResCount; ++i )
			{
				if ( GetDeviceData().m_Textures.IsInUse(i) )
				{
					const STextureData& data = GetDeviceData().m_Textures.Data(i);
					Uint32 numRef = GetDeviceData().m_Textures.GetRefCount(i);

					const char* groupType = "Unknown";
					switch ( data.m_Group )
					{
					case TEXG_System: groupType = "System"; break;
					case TEXG_Generic: groupType = "Generic"; break;
					case TEXG_Streamable: groupType = "Streamable"; break;
					case TEXG_UI: groupType = "UI"; break;
					}

					sprintf_s( buffer, "Texture[%i]: ID %i, %s, %ix%i, format %i, num ref %i, source '%ls'\n", numTextures, i, groupType, data.m_Desc.width, data.m_Desc.height, data.m_Desc.format, numRef, data.m_debugPath );
					OutputDebugStringA( buffer );

					++numTextures;
				}
			}
		}

		/// Dump list of loaded buffers
		Uint32 numBuffers = 0;
		{
			for ( Uint32 i=0; i<(Uint32)GetDeviceData().m_Buffers._MaxResCount; ++i )
			{
				if ( GetDeviceData().m_Buffers.IsInUse(i) )
				{
					const SBufferData& data = GetDeviceData().m_Buffers.Data(i);
					Uint32 numRef = GetDeviceData().m_Buffers.GetRefCount(i);

					const char* bufferType = "Unknown";
					switch ( data.m_Desc.category )
					{
					case BCC_Vertex: bufferType = "Vertex"; break;
					case BCC_Index16Bit: bufferType = "Index16"; break;
					case BCC_Index32Bit: bufferType = "Index32"; break;
					}

					sprintf_s( buffer, "Buffer[%i]: ID %i, %s, %i bytes, num ref %i, source '%ls'\n", numBuffers, i, bufferType, data.m_Desc.size, numRef, data.m_debugPath );
					OutputDebugStringA( buffer );

					++numBuffers;
				}
			}
		}

		/// Count queries
		Uint32 numQueries = 0;
		{
			for ( Uint32 i=0; i<(Uint32)GetDeviceData().m_Queries._MaxResCount; ++i )
			{
				if ( GetDeviceData().m_Queries.IsInUse(i) )
				{
					//const SQueryData& data = GetDeviceData().m_Queries.Data(i);
					++numQueries;
				}
			}
		}

		/// Total stats
		sprintf_s( buffer, "GPU api %i textures in use\n", numTextures );
		OutputDebugStringA( buffer );
		sprintf_s( buffer, "GPU api %i buffers in use\n", numBuffers );
		OutputDebugStringA( buffer );
		sprintf_s( buffer, "GPU api %i queries in use\n", numQueries );
		OutputDebugStringA( buffer );
#endif
	}
#endif

	void SetComputeShaderConstsRaw( Uint32 dataSize, const void *dataBuffer )
	{
		SDeviceData &dd = GetDeviceData();
		
		if ( !dataSize || NULL == dataBuffer )
		{
			return;
		}

		if ( NULL == dd.m_CustomCSConstantBuffer )
		{
			return;
		}

		// Update buffer
		{
		#ifdef _DEBUG
			{
				D3D11_BUFFER_DESC desc;
				dd.m_CustomCSConstantBuffer->GetDesc( &desc );
				GPUAPI_ASSERT( dataSize <= desc.ByteWidth );
			}
		#endif

			GPUAPI_ASSERT( 0 == dataSize % sizeof(Float) );

			D3D11_MAPPED_SUBRESOURCE mapped;
			GetDeviceContext()->Map(dd.m_CustomCSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
			Red::System::MemoryCopy(mapped.pData, dataBuffer, dataSize);
			GetDeviceContext()->Unmap(dd.m_CustomCSConstantBuffer, 0);
		}

		// Bind buffer
		GetDeviceContext()->CSSetConstantBuffers( 0, 1, &dd.m_CustomCSConstantBuffer );
	}

	void SetShaderDebugPath( const ShaderRef& shader, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_Shaders.IsInUse(shader) );
		SShaderData &data = GetDeviceData().m_Shaders.Data(shader);
		Red::System::StringCopy( data.m_debugPath, debugPath, ARRAYSIZE(data.m_debugPath) );

		Uint32 pathLen = ( Uint32 )Red::System::StringLength( data.m_debugPath );

		// Stored as IUknown, avoiding QueryInterface
		ID3D11DeviceChild* pShader = static_cast< ID3D11DeviceChild* >( data.m_pShader );

		if ( pShader )
		{
			// Destroy previous data
			pShader->SetPrivateData( WKPDID_D3DDebugObjectName, 0, NULL );

			if (pathLen > 0)
			{
				pShader->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, data.m_debugPath );
			}
		}
#endif
	}

	//----------------------------------------------------------------------------

	void SetVsWaveLimits(Uint32 waveLimitBy16, Uint32 lateAllocWavesMinus1)
	{
	}

	void ResetVsWaveLimits()
	{
	}

	namespace Hacks
	{
		void UpdateConstantBuffers()
		{
			GpuApi::UpdateConstantBuffers();
		}
	}
}
