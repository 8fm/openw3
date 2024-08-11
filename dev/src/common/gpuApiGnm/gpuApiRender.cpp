/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "..\redSystem\crt.h"
#include "../redMath/mathfunctions_fpu.h"

#include <pm4_dump.h>
#include <perf.h>

#include "../gpuApiUtils/gpuApiRenderCommon.h"

namespace GpuApi
{
	// ----------------------------------------------------------------------

	Bool				g_IsAlreadyRendering = false;
	static Uint32		g_batchIndex = 0;
	Bool				g_captureNextFrame = false;
	Bool				g_validatePreviousFrame = false;

	// from gpuApiDevice.cpp
	extern const char* GetProfilerMarkerString();
	extern Int32 ValidateGfxc(sce::Gnmx::GfxContext&);
	extern Int32 ValidatePreviousFrame(sce::Gnmx::GfxContext&);

	Uint32 BatchIndex()
	{
		return g_batchIndex;
	}

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
		GPUAPI_FATAL_ASSERT( !g_IsInsideRenderBlock, "Begin render already inside render block!" );
		g_IsInsideRenderBlock = true;

		static Uint32 frameIndex = 0;

		g_batchIndex = 0;	// reset our batch counter

		SDeviceData &dd = GetDeviceData();

		// Perform GNM Display Buffer setup
		if (!g_IsAlreadyRendering)
		{
			SSwapChainData& swapChainData = GetSwapChainData();

			sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;
			sce::Gnmx::ComputeContext &cmpc = swapChainData.backBuffer->computeContext;

			// Wait for the EOP event, discarding the first #DisplayBuffers frames.
			// This is important as it ensures that the command buffers are not
			// overridden by the CPU while the GPU is still reading them.
			if( frameIndex >= swapChainData.displayBufferCount )
			{
				Int32 ret;
				Uint32 waitCount = 0;

				while( swapChainData.backBuffer->state[0] != (Uint32)SSwapChainData::EDisplayBufferIdle )
				{
					waitCount++;

					if (g_validatePreviousFrame)
					{
						g_validatePreviousFrame = false;
						ValidatePreviousFrame(gfxc);
					}
#if 0
#ifndef RED_FINAL_BUILD
					if (SCE_KERNEL_ERROR_ETIMEDOUT == ret)
					{
						if ( waitCount++ & 0xFF )
						{					
							debugBits ^= 0xFF;
							::sceKernelSetGPO( debugBits );
						}

						if (waitCount==300)
						{
							GPUAPI_LOG(TXT("Probable GPU hang detected, running Validate on the submitted drawcalls..."));

							// if we've hung for 30 seconds call Validate on the DCBs that were submitted on the frame that crashed
							ValidatePreviousFrame(gfxc);
						}
					}
					else
					{
						::sceKernelSetGPO( 0 );
					}
#endif

					if (SCE_KERNEL_ERROR_ETIMEDOUT == ret)
					{
						++waitCount;
						if (waitCount==300)
						{
							sce::Gnm::debugHardwareStatus(sce::Gnm::kHardwareStatusDump);
							__debugbreak();

							sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;

#define ENABLE_DUMP_PACK_STREAM (1)
#if defined ENABLE_DUMP_PACK_STREAM
							const uint32_t dcbSizeInBytes = static_cast<uint32_t>(gfxc.m_dcb.m_cmdptr - gfxc.m_dcb.m_beginptr);
							sce::Gnm::Pm4Dump::dumpPm4PacketStream(stdout, gfxc.m_dcb.m_beginptr, dcbSizeInBytes);
#ifdef ENABLE_COMPUTE_SHADERS
							{
								const uint32_t dcbSizeInBytes = static_cast<uint32_t>(gComputeContexts[GCurBackBuffer].m_dcb.m_cmdptr - gComputeContexts[GCurBackBuffer].m_dcb.m_beginptr);
								sce::Gnm::Pm4Dump::dumpPm4PacketStream(stdout, gComputeContexts[GCurBackBuffer].m_dcb.m_beginptr, dcbSizeInBytes);
							}
#endif
#endif
						}
					}
#endif
				};

				// Safety check: ensure that the GPU passed the prepareFlip
				// command for the DCB that will be used in this frame.
				SceVideoOutFlipStatus flipStatus;
				for(;;)
				{
					ret = sceVideoOutGetFlipStatus( dd.m_videoOutHandle, &flipStatus );
					if( ret != SCE_OK )
					{
						GPUAPI_ERROR( TXT("sceVideoOutGetFlipStatus failed: 0x%08X"), ret );
					}

					// gcQueueNum contains the number of queued flips for which the
					// GPU still needs to execute the relative prepareFlip command.
					if( flipStatus.gcQueueNum < swapChainData.displayBufferCount )
						break;
				}

#if !defined(RED_FINAL_BUILD) || defined(RED_PROFILE_BUILD)
				sceRazorCpuSync();
#endif

				if (g_captureNextFrame)
				{
					g_captureNextFrame = false;
					sce::Gnm::triggerCapture("/hostapp/capturedFrame.rzrx");
				}

			}

			g_frameIndex++;

			// reset sampler state cache
			GpuApi::InvalidateSamplerStates();

			// Flag the display buffer as "in use"
			swapChainData.backBuffer->state[0] = SSwapChainData::EDisplayBufferInUse;

			// Reset the graphical context and initialize the hardware state.
			ResetGfxcContext(gfxc, cmpc);

			// just do this once per frame because setGlobalDescriptor invalidates the cached global table and needs to be synced before the next draw
			if ( SShaderData::s_graphicsScratchBufferSize > 0 )
			{
				gfxc.setGlobalDescriptor( sce::Gnm::kShaderGlobalResourceScratchRingForGraphic, &SShaderData::s_graphicsScratchBuffer ); 
				gfxc.setGraphicsScratchSize( SShaderData::s_graphicsMaxNumWaves, SShaderData::s_graphicsNum1KbyteChunksPerWave );
			}

			GpuApi::SetCustomDrawContext(GpuApi::DSSM_Max, GpuApi::RASTERIZERMODE_Max, GpuApi::BLENDMODE_Max);

			gfxc.setActiveShaderStages( sce::Gnm::kActiveShaderStagesVsPs );
			dd.m_lastActiveShaderStages = sce::Gnm::kActiveShaderStagesVsPs;
			dd.m_lastIndexBCC = BCC_Invalid;
			dd.m_lastPrimitiveType =  PRIMTYPE_Invalid;
			dd.m_shadersChangedMask = 0xFF;
			dd.m_lastNumInstances = 0;

			++frameIndex;
			g_IsAlreadyRendering = true;
		}

#ifndef RED_FINAL_BUILD
		// Reset usage stats
		dd.m_NumConstantBufferUpdates = 0;

		if ( dd.m_queryPending )
		{
			Uint64 frameStart;
			Uint64 frameEnd;
			eQueryResult queryResultStart = GetQueryResult( dd.m_frameStartQuery, frameStart, false );
			eQueryResult queryResultEnd = GetQueryResult( dd.m_frameEndQuery, frameEnd, false );

			if ( queryResultStart == QR_Success && queryResultEnd == QR_Success )
			{
				dd.m_DeviceUsageStats.m_GPUFrameTime = Float( frameEnd - frameStart ) / Float( SCE_GNM_GPU_CORE_CLOCK_FREQUENCY ) * 1000.0f;
				dd.m_queryPending = false;
			}
		}

		if ( !dd.m_queryPending )
		{
			EndQuery( dd.m_frameStartQuery );
		}
#endif
	}

	void EndRender()
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		g_IsInsideRenderBlock = false;

		GpuApi::SetupBlankRenderTargets();

#ifndef RED_FINAL_BUILD
		SDeviceData &dd = GetDeviceData();

		// NOTE: GNM Display Buffer flipping and submission goes to the Present function below
		// Update device usage stats
		dd.m_DeviceUsageStats.m_NumConstantBufferUpdates = dd.m_NumConstantBufferUpdates;
		dd.m_DeviceUsageStats.m_constantMemoryLoad = dd.m_constantBufferMem.GetDebugThroughput();
		dd.m_DeviceUsageStats.m_isConstantBufferMemoryWithinBounds = ( 2 * dd.m_constantBufferMem.GetDebugThroughput() / dd.m_constantBufferMem.GetDebugMemSize() ) == 0;

		if ( !dd.m_queryPending )
		{
			EndQuery( dd.m_frameEndQuery );
			dd.m_queryPending = true;
		}
#endif
	}

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
	static Bool printHistogram = false;
#endif
	
	void WaitUntilBackBufferReady()
	{
		SDeviceData& dd = GetDeviceData();
		SSwapChainData& swapChainData = GetSwapChainData();
		sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;

		// The waitUntilSafeForRendering stalls the GPU until the scan-out
		// operations on the current display buffer have been completed.
		// This command is non-blocking for the CPU.
		gfxc.waitUntilSafeForRendering( dd.m_videoOutHandle, swapChainData.backBufferIndex );

		if ( dd.m_lastFrameWaitedForBackBuffer == FrameIndex() )
		{
			GPUAPI_LOG_WARNING( TXT("Waiting for a back buffer more than once a frame! This is likely not harmful if it happens rarely in some nearly empty frames, but only then.") );
		}
		dd.m_lastFrameWaitedForBackBuffer = FrameIndex();
	}

	Bool enableCMASK = true;
	void AssertAnyTextureThatHasCMASKButIsntCleared()
	{
		SDeviceData& dd = GetDeviceData();
		for ( Uint32 i=1; i<dd.m_Textures._MaxResCount; ++i )
		{
			if ( dd.m_Textures.IsInUse( i ) )
			{
				STextureData& texData = dd.m_Textures.Data( i );
				if ( texData.m_aliasedAsRenderTargetsSize > 0 )
				{
					if ( texData.m_aliasedAsRenderTargets[0].getCmaskFastClearEnable() )
					{
						Int32 aaa = 0;
						(void)aaa;
					}
					if ( texData.m_aliasedAsRenderTargets[0].getCmaskFastClearEnable() && ( g_frameIndex - texData.m_lastClearingFrame > 1 ) )
					{
						GPUAPI_ASSERT( "AAA" );
					}
					if ( texData.m_doCmask && texData.m_aliasedAsRenderTargets[0].getCmaskFastClearEnable() != enableCMASK )
					{
						texData.m_aliasedAsRenderTargets[0].setCmaskFastClearEnable( enableCMASK );
					}
				}
			}
		}
	}

	void Present( const Rect* sourceRect, const Rect* destRect, const SwapChainRef& swapChain, Bool useVsync, Uint32 vsyncThreshold )
	{
		SDeviceData &dd = GetDeviceData();

		//AssertAnyTextureThatHasCMASKButIsntCleared();

		g_IsAlreadyRendering = false;

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

		dd.m_CustomCSConstantBuffer.setBaseAddress(nullptr);
		dd.m_constantBufferMem.FrameStart();

		// Perform GNM Display Buffer flip and submit. Will wait for the end of pipe event (can stall CPU if necessary)
		{
			SSwapChainData& swapChainData = GetSwapChainData();

			sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;

			// Write the label that indicates that the GPU finished working on this frame
			// and trigger a software interrupt to signal the EOP event queue
			gfxc.writeImmediateDwordAtEndOfPipe(
				sce::Gnm::kEopFlushCbDbCaches,
				(void*) swapChainData.backBuffer->state,
				SSwapChainData::EDisplayBufferIdle,
				sce::Gnm::kCacheActionNone
				);



			// Submit compute context.
			auto computeRet = dd.m_computeQueue.submit( &swapChainData.backBuffer->computeContext );
			if ( computeRet != sce::Gnmx::ComputeQueue::kSubmitOK )
			{
				GPUAPI_ERROR( TXT("ComputeQueue::submit failed.") );
			}


			// Submit the command buffers and request a flip of the display buffer
			Int32 ret = SubmitAllAndFlip(gfxc, dd.m_videoOutHandle, swapChainData.backBufferIndex, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);
			if( ret != sce::Gnm::kSubmissionSuccess )
			{
				GPUAPI_ERROR( TXT("GfxContext::submitAndFlip failed: 0x%08X."), ret );
				swapChainData.backBuffer->state[0] = SSwapChainData::EDisplayBufferIdle;

				switch ( ret )
				{
				case sce::Gnm::kSubmissionFailedInvalidArgument:
					GPUAPI_ERROR( TXT("The submission failed because invalid arguments were passed to the submit function.") );
					break;
				case sce::Gnm::kSubmissionFailedNotEnoughResources:
					GPUAPI_ERROR( TXT("The submission failed because there were not enough resources to submit the command buffers; try to reduce the number of DCB/CCB being submitted at once.") );
					break;
				case sce::Gnm::kSubmissionAndFlipFailedInvalidCommandBuffer:
					GPUAPI_ERROR( TXT("The submission failed because the DrawCommandBuffer::prepareFlip() hasn't been called.") );
					break;
				case sce::Gnm::kSubmissionAndFlipFailedInvalidQueueFull:
					GPUAPI_ERROR( TXT("The submission failed because the flip queue is full.") );
					break;
				case sce::Gnm::kSubmissionAndFlipFailedRequestFailed: 
					GPUAPI_ERROR( TXT("The submission failed because the flip request failed.") );
					break;
				case sce::Gnm::kSubmissionFailedInternalError:
					GPUAPI_ERROR( TXT("The submission failed because of an internal error.") );
					break;
				default:
					break;
				}
			}

			// Signal the system that every draw for this frame has been submitted.
			// This function gives permission to the OS to hibernate when all the
			// currently running GPU tasks (graphics and compute) are done.
			PS4_SubmitDone();

			// Update the display chain pointers
			swapChainData.backBufferIndex = (swapChainData.backBufferIndex + 1) % swapChainData.displayBufferCount;
			swapChainData.backBuffer = swapChainData.displayBuffers + swapChainData.backBufferIndex;

			// ensure dynamic vertex/index buffers Discard the first time they are used each frame!
			dd.m_currentVertexWritePosition = 0;
			dd.m_currentIndexWritePosition = 0;
		}
	}

	void PS4_SubmitDone()
	{
		SDeviceData &dd = GetDeviceData();
		Int32 ret = sce::Gnm::submitDone();
		if( ret == SCE_OK )
		{
			dd.m_timeSinceSubmitDone = 0.0f;
		}
		else
		{
			GPUAPI_ERROR( TXT("Gnm::submitDone failed: 0x%08X."), ret );
		}
	}

	Float PS4_GetAndUpdateTimeSinceSubmitDone( Float timeToAdd )
	{
		SDeviceData &dd = GetDeviceData();

		const Float returnValue = dd.m_timeSinceSubmitDone;
		dd.m_timeSinceSubmitDone += timeToAdd;

		return returnValue;
	}

	size_t MapAlignment(sce::Gnm::EmbeddedDataAlignment align)
	{
		switch (align)
		{
		case sce::Gnm::kEmbeddedDataAlignment4	:		return 4;
		case sce::Gnm::kEmbeddedDataAlignment8  :		return 8;
		case sce::Gnm::kEmbeddedDataAlignment16 :		return 16;
		case sce::Gnm::kEmbeddedDataAlignment64 :		return 64;
		case sce::Gnm::kEmbeddedDataAlignment128:		return 128;
		case sce::Gnm::kEmbeddedDataAlignment256:		return 256;
		default:
			GPUAPI_HALT ("Unhandled EmbeddedDataAlignment %d", align);
			return 4;
		}
	}

	void CreateConstantBuffers()
	{
		// Nothing to do here for Gnm
	}


	ShaderRef GetShader(SDeviceData &dd, eShaderType shaderType)
	{
		GPUAPI_ASSERT( shaderType < ShaderTypeMax );
		return dd.m_shadersSet[ shaderType ];
	}


	void SetConstantBufferOnAllVertexShaders(SDeviceData &dd, sce::Gnmx::GfxContext& gfxc, Uint32 slot, const sce::Gnm::Buffer* buffer)
	{
		if ( dd.m_shadersSet[VertexShader] )
		{
			if ( dd.m_shadersSet[ HullShader ] ) 
			{
				gfxc.setConstantBuffers(sce::Gnm::kShaderStageLs, slot, 1, buffer);
				gfxc.setConstantBuffers(sce::Gnm::kShaderStageHs, slot, 1, buffer);
			} 
			else if (dd.m_shadersSet[ GeometryShader ] ) 
			{
				gfxc.setConstantBuffers(sce::Gnm::kShaderStageEs, slot, 1, buffer);
				gfxc.setConstantBuffers(sce::Gnm::kShaderStageGs, slot, 1, buffer);
			}
			gfxc.setConstantBuffers(sce::Gnm::kShaderStageVs, slot, 1, buffer);
		}
	}


	// forward define
	void SetBufferSRVsInternal(SDeviceData &dd, sce::Gnmx::GfxContext &gfxc);

	RED_INLINE Bool WasShaderChanged( Uint8 shaderTypes ) { return ( g_DeviceData->m_shadersChangedMask & shaderTypes ) != 0; }

	void UpdateConstantBuffers(bool setGlobalConstants = true, bool compute = false)
	{
		SDeviceData &dd = GetDeviceData();
		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		if (setGlobalConstants )
		{
			{
				Bool doSet = WasShaderChanged( VertexShaderFlag | HullShaderFlag | DomainShaderFlag | GeometryShaderFlag );
				if ( dd.m_constantBufferDirtyMask & EBDM_FrequentVS )
				{
					Int32 cbSize = g_FrequentVSRange.GetFullNumIndexes();
					void* mem = dd.m_constantBufferMem.Allocate(cbSize*16, 16 );
					CopyVectors128( mem, dd.m_VSConstants, cbSize );
					dd.m_frequentVSConstantBuffer.initAsConstantBuffer( mem, cbSize*16 );
					dd.m_frequentVSConstantBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

					dd.m_constantBufferDirtyMask ^= EBDM_FrequentVS;
#ifndef RED_FINAL_BUILD
					++dd.m_NumConstantBufferUpdates;
#endif
					g_FrequentVSRange.Reset();
					doSet = true;
				}

				if ( doSet )
				{
					SetConstantBufferOnAllVertexShaders(dd, gfxc, dd.sc_frequentVSConstantBufferReg, &dd.m_frequentVSConstantBuffer);
				}
			}

			{
				Bool doSet = WasShaderChanged( VertexShaderFlag | HullShaderFlag | DomainShaderFlag | GeometryShaderFlag );
				if (dd.m_constantBufferDirtyMask & EBDM_CustomVS )
				{
					Int32 cbSize = g_CustomVSRange.GetFullNumIndexes();
					void* mem = dd.m_constantBufferMem.Allocate(cbSize*16, 16 );
					CopyVectors128( mem, &dd.m_VSConstants[VSC_Custom_First * 4], cbSize );
					dd.m_customVSConstantBuffer.initAsConstantBuffer( mem, cbSize*16 );
					dd.m_customVSConstantBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

					dd.m_constantBufferDirtyMask ^= EBDM_CustomVS;
#ifndef RED_FINAL_BUILD
					++dd.m_NumConstantBufferUpdates;
#endif
					g_CustomVSRange.Reset();
					doSet = true;
				}

				if ( doSet )
				{
					SetConstantBufferOnAllVertexShaders(dd, gfxc, dd.sc_customVSConstantBufferReg, &dd.m_customVSConstantBuffer);
				}
			}

			{
				Bool doSet = WasShaderChanged( PixelShaderFlag );
				if (dd.m_constantBufferDirtyMask & EBDM_FrequentPS )
				{
					Int32 cbSize = g_FrequentPSRange.GetFullNumIndexes();
					void* mem = dd.m_constantBufferMem.Allocate(cbSize*16, 16 );
					CopyVectors128( mem, dd.m_PSConstants, cbSize );
					dd.m_frequentPSConstantBuffer.initAsConstantBuffer( mem, cbSize*16 );
					dd.m_frequentPSConstantBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

					dd.m_constantBufferDirtyMask ^= EBDM_FrequentPS;
#ifndef RED_FINAL_BUILD
					++dd.m_NumConstantBufferUpdates;
#endif
					g_FrequentPSRange.Reset();
					doSet = true;
				}

				if ( doSet && dd.m_shadersSet[ PixelShader ] )
				{
					gfxc.setConstantBuffers( sce::Gnm::kShaderStagePs, dd.sc_frequentPSConstantBufferReg, 1, &dd.m_frequentPSConstantBuffer );
				}
			}

			{
				Bool doSet = WasShaderChanged( PixelShaderFlag );
				if (dd.m_constantBufferDirtyMask & EBDM_CustomPS )
				{
					Int32 cbSize = g_CustomPSRange.GetFullNumIndexes();
					void* mem = dd.m_constantBufferMem.Allocate(cbSize*16, 16 );
					CopyVectors128( mem, &dd.m_PSConstants[PSC_Custom_First * 4], cbSize );
					dd.m_customPSConstantBuffer.initAsConstantBuffer( mem, cbSize*16 );
					dd.m_customPSConstantBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

					dd.m_constantBufferDirtyMask ^= EBDM_CustomPS;
#ifndef RED_FINAL_BUILD
					++dd.m_NumConstantBufferUpdates;
#endif
					g_CustomPSRange.Reset();
					doSet = true;
				}

				if ( doSet && dd.m_shadersSet[ PixelShader ] )
				{
					gfxc.setConstantBuffers( sce::Gnm::kShaderStagePs, dd.sc_customPSConstantBufferReg, 1, &dd.m_customPSConstantBuffer );
				}
			}

			if ( compute && dd.m_shadersSet[ ComputeShader ] )
			{
				if (dd.m_CustomCSConstantBuffer.getBaseAddress())
				{
					gfxc.setConstantBuffers(sce::Gnm::kShaderStageCs, 0, 1, &dd.m_CustomCSConstantBuffer);
				}
			}
		}

		Uint32 shader_stage_i = compute ? ComputeShader : 0;
		Uint32 endShaderStage = compute ? (ComputeShader+1) : ComputeShader;

		// bind all the cached constant buffers the shader requires
		for (; shader_stage_i < endShaderStage; ++shader_stage_i)
		{
			sce::Gnm::ShaderStage gnm_shader_stage = MapToShaderStage((eShaderType)shader_stage_i);
			ShaderRef shader_ref = GetShader(dd, (eShaderType)shader_stage_i);
			if (shader_ref)
			{
				SShaderData& shader = dd.m_Shaders.Data(shader_ref);
				Uint32 mask = shader.m_constantBufferMask;

				// CB2 and 3 are the frequent and custom constant buffers so always skip those here - unless we're CS which works differently!
				if (setGlobalConstants)
				{
					if (shader_stage_i == ComputeShader)
						mask &= ~1;
					else
						mask &= ~12; // ~ 0 0 0 0 1 1 0 0
				}

				while (mask)
				{
					int slot_i = __builtin_ctz(mask);	// find the lowest set bit
					mask ^= (1 << slot_i);				// clear the bit

					BufferRef bufferRef = dd.m_ConstantBuffersSet[shader_stage_i][slot_i];
					if (bufferRef != BufferRef::Null() && dd.m_Buffers.IsInUse(bufferRef))
					{
						SBufferData& bufferData = dd.m_Buffers.Data(bufferRef);
						sce::Gnm::Buffer constantBuffer;
						if ( bufferData.m_Desc.usage == BUT_Dynamic )
						{
							if ( bufferData.cbuf.m_discarded || bufferData.m_frameDiscarded < FrameIndex() )
							{
								bufferData.cbuf.m_submitBuffer = static_cast< Int8* >( dd.m_constantBufferMem.Allocate(bufferData.m_Desc.size, 16 ) );
								GPUAPI_ASSERT( bufferData.cbuf.m_submitBuffer != nullptr );
								Red::MemoryCopy(bufferData.cbuf.m_submitBuffer, bufferData.GetMemoryPtr(), bufferData.m_Desc.size);
								bufferData.cbuf.m_discarded = false;
								bufferData.m_frameDiscarded = FrameIndex();
							}
							constantBuffer.initAsConstantBuffer( bufferData.cbuf.m_submitBuffer, bufferData.m_Desc.size );
						}
						else
						{
							constantBuffer.initAsConstantBuffer( bufferData.GetMemoryPtr(), bufferData.m_Desc.size );
						}
						gfxc.setConstantBuffers(gnm_shader_stage, slot_i, 1, &constantBuffer);
						
						if (gnm_shader_stage == sce::Gnm::kShaderStageVs)
						{
							if ( dd.m_shadersSet[ HullShader ] )
								gfxc.setConstantBuffers(sce::Gnm::kShaderStageLs, slot_i, 1, &constantBuffer);
							if ( dd.m_shadersSet[ GeometryShader ] )
								gfxc.setConstantBuffers(sce::Gnm::kShaderStageEs, slot_i, 1, &constantBuffer);
						}
					}
					else
					{
						GPUAPI_HALT ("Shader expects CB in slot %d but it is not set!", slot_i);
						// provide fallback 
					}

				}
			}
		}

		// set SRVs here
		SetBufferSRVsInternal(dd, gfxc);
	}

	void BindMainConstantBuffers()
	{
		SDeviceData &dd = GetDeviceData();
		dd.m_constantBufferDirtyMask = EBDM_All;
		MapWholeBuffersRange();
		dd.m_shadersChangedMask = 0xFF;
		dd.m_lastPrimitiveType = PRIMTYPE_Invalid;
		dd.m_lastIndexBCC = BCC_Invalid;
	}


	void SetStreamOutBuffersInternal(SDeviceData &dd, sce::Gnmx::GfxContext &gfxc)
	{
		if ( dd.m_shadersSet[ GeometryShader ] )
		{
			// do not progress if StreamOut is not supported by this shader
			const SShaderData& shaderData = dd.m_Shaders.Data (dd.m_shadersSet[ GeometryShader ]);
			if (!shaderData.m_streamOutDesc)
				return;

			const VertexLayoutDesc& layout = *shaderData.m_streamOutDesc;

			bool setupSO = false;

			sce::Gnm::StreamoutBufferMapping bufferBinding;
			bufferBinding.init();

			for (Uint32 i = 0; i < GPUAPI_STREAMOUT_MAX_SLOTS; ++i)
			{
				// figure out the stride of this buffer
				Uint32 stride = layout.GetSlotStride(i);
				if (!dd.m_StreamOutBuffers[i].isNull() && stride>0)
				{
					if (!setupSO)
					{
						// SO requires Partial vs wave, the rest are defaults
						gfxc.setVgtControl(256-1, sce::Gnm::kVgtPartialVsWaveEnable);

						// this is apparently a very expensive operation so only do it if necessary
						gfxc.flushStreamout();
					}
					setupSO = true;

					bufferBinding.bindStream((sce::Gnm::StreamoutBufferId)i, sce::Gnm::StreamoutBufferMapping::kGsStreamBuffer0);


					const SBufferData& bufData = dd.m_Buffers.Data(dd.m_StreamOutBuffers[i]);

					Uint32 bufferSizeDW = bufData.m_Desc.size / 4;
					Uint32 bufferStrideDW = stride / 4;
					Uint32 bufferOffsetDW = dd.m_StreamOutOffsets[i] / 4;

					sce::Gnm::Buffer streamOutBuffer;
					streamOutBuffer.initAsGsStreamoutDescriptor(bufData.GetMemoryPtr(), stride);
					streamOutBuffer.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC); // we write to it, so it's GPU coherent

					gfxc.setStreamoutBuffers(i, 1, &streamOutBuffer);

					gfxc.setStreamoutBufferDimensions((sce::Gnm::StreamoutBufferId)i, bufferSizeDW, bufferStrideDW);
					gfxc.writeStreamoutBufferOffset((sce::Gnm::StreamoutBufferId)i, bufferOffsetDW);

					char marker_str[128];
					sprintf(marker_str, "SO: 0x%p offset: %d size: %d finalptr: 0x%p", bufData.GetMemoryPtr(), dd.m_StreamOutOffsets[i], bufData.m_Desc.size, bufData.GetMemoryPtr() + (bufferOffsetDW*4));
					gfxc.setMarker(marker_str);

					//GPUAPI_LOG (TXT("STREAMOUT: [%i] 0x%016llx offset:%d size:%d stride:%d finalptr: 0x%016llx"), i, bufData.GetMemoryPtr(), dd.m_StreamOutOffsets[i], bufData.m_Desc.size, stride, bufData.GetMemoryPtr() + (bufferOffsetDW*4));
				}
			}

			if (setupSO)
			{
				// now we've processed all streams this bufferBinding information should be complete
				gfxc.setStreamoutMapping(&bufferBinding);
			}
		}
	}



	void SetVertexBuffersInternal( SDeviceData &dd, sce::Gnmx::GfxContext &gfxc )
	{
		SVertexLayoutData& layoutData = dd.m_VertexLayouts.Data( dd.m_VertexLayout );
		SVertexLayoutEntryDesc* elements = layoutData.m_elements;

		static VertexLayoutRef lastLayout = VertexLayoutRef::Null();
		if ( !dd.m_vbChanged && !WasShaderChanged( VertexShaderFlag | HullShaderFlag | DomainShaderFlag | GeometryShaderFlag ) && lastLayout == dd.m_VertexLayout )
		{
			return;
		}
		lastLayout = dd.m_VertexLayout;

		// Figure out at which hardware stage we will be reading the streams
		sce::Gnm::ShaderStage fetchStage = sce::Gnm::kShaderStageVs;
		if ( dd.m_shadersSet[ HullShader ] )				fetchStage = sce::Gnm::kShaderStageLs;
		else if ( dd.m_shadersSet[ GeometryShader ] )		fetchStage = sce::Gnm::kShaderStageEs;

		for ( Uint32 i=0; i < layoutData.m_numElements; ++i )
		{
			if ( !(dd.m_VertexBuffers[ elements[i].m_slot ].isNull()) )
			{
				const SBufferData& bufferData = dd.m_Buffers.Data( dd.m_VertexBuffers[ elements[i].m_slot ] );

				// Compute address of the first wanted occurence of the element (eg. color or uv)
				Int8* garlicBufferWithOffset = bufferData.GetMemoryPtr()	// First byte of the first vertex
					+ dd.m_VertexBufferOffsets[ elements[i].m_slot ]			// offset requested by the caller
				+ elements[i].m_offset;										// offset to the element in the vertex structure 

				Uint32 elementCount = (bufferData.m_Desc.size - dd.m_VertexBufferOffsets[ elements[i].m_slot ]) / elements[i].m_stride;
				Uint32 stride = dd.m_VertexBufferStrides[ elements[i].m_slot ];

				dd.m_vertexBufferObjects[ i ].initAsVertexBuffer( garlicBufferWithOffset, elements[i].m_format, stride, elementCount );

				if (bufferData.m_Desc.usage == BUT_StreamOut)
					dd.m_vertexBufferObjects[ i ].setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);

				gfxc.setVertexBuffers( fetchStage, i, 1, &dd.m_vertexBufferObjects[ i ] );
			}
		}

		SetStreamOutBuffersInternal (dd, gfxc);

		dd.m_vbChanged = false;
	}

	// DOCUMENTATION: Overview of Mapping Shader Types and Shader Stages

	//                            TessOFF+GeomOFF          |          TessON+GeomOFF          |         TessON+GeomON            |         TessOff+GeomON
	// Vertex Shader   -            Vertex Stage           |             LDS Stage            |            LDS Stage             |          Export Stage
	// Hull Shader     -                 NA                |             Hull Stage           |           Hull Stage             |              NA
	// Domain Shader   -                 NA                |            Vertex Stage          |          Export Stage            |              NA
	// Geometry Shader -                 NA                |                NA                |  Geometry Stage + Vertex Stage   |  Geometry Stage + Vertex Stage
	// Pixel Shader    -            Pixel Stage            |           Pixel Stage            |           Pixel Stage            |          Pixel Stage
	// Compute Shader  -           Compute Stage           |          Compute Stage           |          Compute Stage           |         Compute Stage

	RED_FORCE_INLINE void SetActiveShaderStages( SDeviceData& dd, sce::Gnmx::GfxContext& gfxc, sce::Gnm::ActiveShaderStages activeStages )
	{
		if ( dd.m_lastActiveShaderStages != activeStages )
		{
			gfxc.setActiveShaderStages( activeStages );
			dd.m_lastActiveShaderStages = activeStages;
		}
	}

	void SetCBControlInternal( SDeviceData& dd, sce::Gnmx::GfxContext& gfxc, sce::Gnm::CbMode cbMode )
	{
		if ( dd.m_lastCBMode != cbMode )
		{
			gfxc.setCbControl( cbMode, sce::Gnm::kRasterOpCopy );	
			dd.m_lastCBMode = cbMode;
		}
	}

	void internalSetPsShader(sce::Gnmx::GfxContext& gfxc, const SShaderData* shaderData)
	{
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		const sce::Gnmx::InputResourceOffsets* resourceTable = shaderData ? &shaderData->m_resourceOffsets : nullptr;
		gfxc.setBoundShaderResourceOffsets(sce::Gnm::kShaderStagePs, resourceTable);
#endif

		gfxc.setPsShader(shaderData ? shaderData->m_psShader : nullptr);
	}


	void internalSetVsShader(sce::Gnmx::GfxContext& gfxc, const SShaderData* shaderData, const SFetchShader* fetchShader)
	{
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		const sce::Gnmx::InputResourceOffsets* resourceTable = (fetchShader && fetchShader->m_resourceTable) ? fetchShader->m_resourceTable : &shaderData->m_resourceOffsets;
		gfxc.setBoundShaderResourceOffsets(sce::Gnm::kShaderStageVs, resourceTable);
#endif

		if (fetchShader)
			gfxc.setVsShader( shaderData->m_vsShader, fetchShader->m_shaderModifier, fetchShader->m_fetchShaderMemory );
		else
			gfxc.setVsShader( shaderData->m_vsShader, 0, nullptr );
	}

	void internalSetEsShader(sce::Gnmx::GfxContext& gfxc, const SShaderData* shaderData, const SFetchShader* fetchShader)
	{
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		const sce::Gnmx::InputResourceOffsets* resourceTable = (fetchShader && fetchShader->m_resourceTable) ? fetchShader->m_resourceTable : &shaderData->m_resourceOffsets;
		gfxc.setBoundShaderResourceOffsets(sce::Gnm::kShaderStageEs, resourceTable);
#endif

		gfxc.setEsShader( shaderData->m_esShader, fetchShader->m_shaderModifier, fetchShader->m_fetchShaderMemory );
	}

	void internalSetGsVsShaders(sce::Gnmx::GfxContext& gfxc, const SShaderData* shaderData)
	{
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		const sce::Gnmx::InputResourceOffsets* resourceTable = &shaderData->m_resourceOffsets;
		gfxc.setBoundShaderResourceOffsets(sce::Gnm::kShaderStageGs, resourceTable);
#endif

		gfxc.setGsVsShaders(shaderData->m_gsShader);
	}

	void internalSetCsShader(sce::Gnmx::GfxContext& gfxc, const SShaderData* shaderData)
	{
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		const sce::Gnmx::InputResourceOffsets* resourceTable = shaderData ? &shaderData->m_resourceOffsets : nullptr;
		gfxc.setBoundShaderResourceOffsets(sce::Gnm::kShaderStageCs, resourceTable);
#endif

		gfxc.setCsShader( shaderData ? shaderData->m_csShader : nullptr );
	}

	RED_FORCE_INLINE void SetPrimitiveInternal( SDeviceData& dd, sce::Gnmx::GfxContext& gfxc, ePrimitiveType primitive )
	{
		if ( primitive != dd.m_lastPrimitiveType )
		{
			gfxc.setPrimitiveType( Map( primitive ) );
			dd.m_lastPrimitiveType = primitive;
		}
	}

	void internalSetLsHsShader(sce::Gnmx::GfxContext& gfxc, const SShaderData* shaderData, const SFetchShader* fetchShader, SShaderData& hullShaderData)
	{
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		const sce::Gnmx::InputResourceOffsets* resourceTable = (fetchShader && fetchShader->m_resourceTable) ? fetchShader->m_resourceTable : &shaderData->m_resourceOffsets;
		gfxc.setBoundShaderResourceOffsets(sce::Gnm::kShaderStageLs, resourceTable);
		gfxc.setBoundShaderResourceOffsets(sce::Gnm::kShaderStageHs, &hullShaderData.m_resourceOffsets);
#endif

		gfxc.setLsHsShaders(shaderData->m_lsShader, fetchShader->m_shaderModifier, fetchShader->m_fetchShaderMemory, hullShaderData.m_hsShader, hullShaderData.m_patchCount);
	}

	RED_FORCE_INLINE void SetIndexSizeInternal( SDeviceData& dd, sce::Gnmx::GfxContext& gfxc, eBufferChunkCategory bcc )
	{
		GPUAPI_ASSERT( bcc == BCC_Index16Bit || bcc == BCC_Index16BitUAV || bcc == BCC_Index32Bit, TXT("Wrong buffer type!") );
		if ( bcc != dd.m_lastIndexBCC )
		{
			if( bcc == BCC_Index32Bit )
			{
				gfxc.setIndexSize( sce::Gnm::IndexSize::kIndexSize32 );
			}
			else
			{
				gfxc.setIndexSize( sce::Gnm::IndexSize::kIndexSize16 );
			}

			dd.m_lastIndexBCC = bcc;
		}
	}

	void SynchronizeComputeToGraphics( sce::Gnmx::GfxContext& gfxc )
	{
		SDeviceData& dd = GetDeviceData();

		// Synchronize Compute to Graphics
		volatile Uint64* label = (volatile Uint64*)dd.m_constantBufferMem.Allocate( sizeof(Uint64), 8 );
		*label = 0x0;
		gfxc.m_dcb.writeAtEndOfShader( sce::Gnm::kEosCsDone, const_cast< Uint64* >( label ), 0x1 );
		gfxc.m_dcb.waitOnAddress( const_cast< Uint64* >( label ), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 );
		gfxc.m_dcb.flushShaderCachesAndWait( sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0, sce::Gnm::kStallCommandBufferParserDisable );
	}

	void SynchronizeGraphicsToCompute( sce::Gnmx::GfxContext& gfxc )
	{
		SDeviceData& dd = GetDeviceData();

		volatile Uint64* label = (volatile Uint64*)dd.m_constantBufferMem.Allocate( sizeof(Uint64), 8 ); // allocate memory from the command buffer
		*label = 0x0; // set the memory to have the val 0
		// This EOP event does the following:
		// - wait for all rendering to complete
		// - flush and invalidate the GPU's CB, DB, L1, and L2 caches
		// - write 0x1 to the specified label address.
		gfxc.m_dcb.writeAtEndOfPipe(
			sce::Gnm::kEopFlushCbDbCaches, 
			sce::Gnm::kEventWriteDestMemory, const_cast<Uint64*>(label),
			sce::Gnm::kEventWriteSource64BitsImmediate, 0x1,
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kCachePolicyLru
			);
		gfxc.m_dcb.waitOnAddress( const_cast<Uint64*>(label), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
	}

	void FillUintsWithCompute( sce::Gnmx::GfxContext& gfxc, void *dstUints, Uint32 numDstUints, Uint32* srcValues, Uint32 numSrcUints )
	{
		SDeviceData& dd = GetDeviceData();
		dd.m_shadersChangedMask = 0xFF;

		// Copy source values to the CB memory so the GPU can safely read it when it gets to it
		Uint32 *source = static_cast<Uint32*>( dd.m_constantBufferMem.Allocate( numSrcUints * sizeof( Uint32 ), sce::Gnm::kEmbeddedDataAlignment4 ) );
		Red::System::MemoryCopy( source, srcValues, numSrcUints * sizeof( Uint32 ) );
		
		gfxc.setShaderType( sce::Gnm::kShaderTypeCompute );

		// If the amount of Uints is a power of two, we can use a slightly faster version of the compute shader
		const Bool srcUintsIsPowerOfTwo = (numSrcUints & (numSrcUints-1)) == 0;
		SShaderData& computeShaderData = dd.m_Shaders.Data( srcUintsIsPowerOfTwo ? dd.m_embeddedShaders.cs_set_uint_fast_c() : dd.m_embeddedShaders.cs_set_uint_c() );
		internalSetCsShader( gfxc, &computeShaderData );

		sce::Gnm::Buffer destinationBuffer;
		destinationBuffer.initAsDataBuffer( dstUints, sce::Gnm::kDataFormatR32Uint, numDstUints );
		destinationBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
		gfxc.setRwBuffers( sce::Gnm::kShaderStageCs, 0, 1, &destinationBuffer );

		sce::Gnm::Buffer sourceBuffer;
		sourceBuffer.initAsDataBuffer( source, sce::Gnm::kDataFormatR32Uint, numSrcUints );
		sourceBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );
		gfxc.setBuffers( sce::Gnm::kShaderStageCs, 0, 1, &sourceBuffer );

		struct Constants
		{
			Uint32 m_destUints;
			Uint32 m_srcUints;
		};
		Constants *constants = (Constants*)dd.m_constantBufferMem.Allocate( sizeof(Constants), sce::Gnm::kEmbeddedDataAlignment4 );
		constants->m_destUints = numDstUints;
		constants->m_srcUints = numSrcUints - (srcUintsIsPowerOfTwo ? 1 : 0);
		sce::Gnm::Buffer constantBuffer;
		constantBuffer.initAsConstantBuffer( constants, sizeof( *constants ) );
		gfxc.setConstantBuffers( sce::Gnm::kShaderStageCs, 0, 1, &constantBuffer );

		gfxc.dispatch( (numDstUints + sce::Gnm::kThreadsPerWavefront - 1) / sce::Gnm::kThreadsPerWavefront, 1, 1 );
	}

	void ClearHTile( sce::Gnmx::GfxContext& gfxc, sce::Gnm::DepthRenderTarget* depthRT, Float depth )
	{
		GPUAPI_ASSERT( depth >= 0.f && depth <= 1.f, TXT("depth value of %f is not between 0 and 1"), depth );

		sce::Gnm::Htile htile = {};
		htile.m_hiZ.m_zMask = 0;
		htile.m_hiZ.m_minZ = static_cast<Uint32>( Red::Math::MFloor( depth * sce::Gnm::Htile::kMaximumZValue) );
		htile.m_hiZ.m_maxZ = static_cast<Uint32>( Red::Math::MCeil( depth * sce::Gnm::Htile::kMaximumZValue) );

		GPUAPI_ASSERT( depthRT->getHtileAddress() != NULL, TXT("depthTarget (0x%p) has no HTILE surface."), depthRT );

		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateDbMeta );

		// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
		Uint32 numSlices = depthRT->getLastArraySliceIndex() - depthRT->getBaseArraySliceIndex() + 1;

		Uint8* htilePtr = ( Uint8* )depthRT->getHtileAddress();
		htilePtr += depthRT->getHtileSliceSizeInBytes() * depthRT->getBaseArraySliceIndex();

		FillUintsWithCompute( gfxc, htilePtr, depthRT->getHtileSliceSizeInBytes() * numSlices / sizeof( Uint32 ), &htile.m_asInt, 1 );
		SynchronizeComputeToGraphics( gfxc );
	}

	void ClearCMask( sce::Gnmx::GfxContext &gfxc, const sce::Gnm::RenderTarget *target )
	{
		GPUAPI_ASSERT( target->getCmaskAddress() != NULL, TXT("target (0x%p) has no CMASK surface."), target);
		
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbMeta );

		// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
		uint32_t numSlices = target->getLastArraySliceIndex() - target->getBaseArraySliceIndex() + 1;

		Uint32 valueToClearWith = 0;
		FillUintsWithCompute( gfxc, target->getCmaskAddress(), target->getCmaskSliceSizeInBytes()*numSlices/sizeof(Uint32), &valueToClearWith, 1 );
		SynchronizeComputeToGraphics( gfxc );
	}

	void EliminateFastClear( sce::Gnmx::GfxContext &gfxc, const sce::Gnm::RenderTarget *target )
	{
		GPUAPI_ASSERT( target, TXT("target must not be NULL.") );
		GPUAPI_ASSERT( target->getCmaskFastClearEnable() );
		GPUAPI_ASSERT( target->getCmaskAddress() != NULL, TXT( "Can not eliminate fast clear from a target with no CMASK." ) );

		gfxc.setShaderType( sce::Gnm::kShaderTypeGraphics );

		SDeviceData& dd = GetDeviceData();
		dd.m_shadersChangedMask = 0xFF;

		// Needs to flush the CMask data
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbMeta );
		gfxc.setRenderTarget( 0, target );

		sce::Gnm::BlendControl blendControl;
		blendControl.init();
		blendControl.setBlendEnable(false);
		gfxc.setBlendControl( 0, blendControl );
		gfxc.setRenderTargetMask( 0x0000000F ); // enable MRT0 output only
		SetCBControlInternal( dd, gfxc, sce::Gnm::kCbModeEliminateFastClear );
		sce::Gnm::DepthStencilControl dsc;
		dsc.init();
		dsc.setDepthControl( sce::Gnm::kDepthControlZWriteDisable, sce::Gnm::kCompareFuncAlways );
		dsc.setDepthEnable( true );
		gfxc.setDepthStencilControl( dsc );
		gfxc.setupScreenViewport( 0, 0, target->getWidth(), target->getHeight(), 0.5f, 0.5f );

		// draw full screen quad using the provided color-clear shader.
		internalSetPsShader( gfxc, nullptr );
		
		SShaderData& vertexShaderData = dd.m_Shaders.Data( dd.m_embeddedShaders.vex_clear_vv() );
		internalSetVsShader( gfxc, &vertexShaderData, nullptr );

		SetPrimitiveInternal( dd, gfxc, PRIMTYPE_QuadList );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );
		gfxc.drawIndexAuto( 3 );

		// Wait for the draw to be finished, and flush the Cb/Db caches:
		{
			volatile Uint32 *cbLabel = static_cast< Uint32* >( dd.m_constantBufferMem.Allocate( sizeof(Uint32), 16 ) );
			*cbLabel = 0;
			gfxc.writeImmediateDwordAtEndOfPipe( sce::Gnm::kEopFlushCbDbCaches, (void*)cbLabel, 1, sce::Gnm::kCacheActionNone );
			gfxc.waitOnAddress( (void*)cbLabel, 0xFFFFFFFF, sce::Gnm::kWaitCompareFuncEqual, 1 );
		}
	}

	void DecompressDepth( sce::Gnmx::GfxContext& gfxc, sce::Gnm::DepthRenderTarget* depthRT )
	{
		GPUAPI_ASSERT( depthRT );

		// This is a quickly pasted'&'rearranged(TM) code from Gnmx Toolkit. Hence I'm resetting some state ghost values like this:
		SDeviceData& dd = GetDeviceData();
		dd.m_shadersChangedMask = 0xFF;

		gfxc.setShaderType( sce::Gnm::kShaderTypeGraphics );
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateDbMeta ); 
		gfxc.setDepthStencilDisable();

		SetCBControlInternal( dd, gfxc, sce::Gnm::kCbModeDisable );
		{	
			gfxc.setDepthRenderTarget( depthRT );
			gfxc.setupScreenViewport(0, 0, depthRT->getWidth(), depthRT->getHeight(), 0.5f, 0.5f);
		}
		{
			sce::Gnm::DbRenderControl dbRenderControl;
			dbRenderControl.init();
			const sce::Gnm::DbTileWriteBackPolicy dbTileWriteBackPolicy = sce::Gnm::kDbTileWriteBackPolicyCompressionForbidden;
			dbRenderControl.setStencilTileWriteBackPolicy(dbTileWriteBackPolicy);
			dbRenderControl.setDepthTileWriteBackPolicy(dbTileWriteBackPolicy);
			gfxc.setDbRenderControl(dbRenderControl);
		}

		gfxc.setPsShader( nullptr );

		SShaderData& vertexShaderData = dd.m_Shaders.Data( dd.m_embeddedShaders.vex_clear_vv() );
		internalSetVsShader( gfxc, &vertexShaderData, nullptr );

		gfxc.setPrimitiveType( sce::Gnm::kPrimitiveTypeRectList );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );
		gfxc.drawIndexAuto( 3 );

		{
			volatile Uint32* label = static_cast<Uint32*>( dd.m_constantBufferMem.Allocate( sizeof(Uint32), 16 ) );
			*label = 0;
			gfxc.writeImmediateDwordAtEndOfPipe( sce::Gnm::kEopFlushCbDbCaches, (void*)label, 1, sce::Gnm::kCacheActionNone);
			gfxc.waitOnAddress( (void*)label, 0xFFFFFFFF, sce::Gnm::kWaitCompareFuncEqual, 1 );
		}
	}

	Bool ClearDepthTarget( const TextureRef &target, Float depthValue, Int32 slice /*= -1*/ )
	{
		// not quite sure what slice==-1 means, lets assume it means the first slice
		if (slice < 0)
			slice = 0;

		sce::Gnmx::GfxContext& context = GetSwapChainData().backBuffer->context;
		STextureData &textureData = GetDeviceData().m_Textures.Data( target );
		const TextureDesc& desc = GetTextureDesc(target);

		Uint16 mipIndex = slice < 0 ? 0 : CalculateMipFromSliceIndex(desc, (Uint16)slice);
		if (mipIndex >= textureData.m_aliasedAsDepthStencilsSize)
		{
			GPUAPI_HALT ("CalculateCubemapSliceMipIndex OUT OF RANGE");
			return false;
		}

		sce::Gnm::DepthRenderTarget& depthStencil = textureData.m_aliasedAsDepthStencils[mipIndex];
		if (depthStencil.getHtileAccelerationEnable())
		{
			Int32 arraySlice = CalculateArrayIndexFromSlice(desc, slice);

			textureData.m_clearDepth = depthValue;
			textureData.m_needsDecompress = true;

			sce::Gnm::DepthRenderTarget depthStencilCopy = depthStencil;
			depthStencilCopy.setArrayView(arraySlice, arraySlice);

			ClearHTile( context, &depthStencilCopy, depthValue );
			GetDeviceData().m_shadersChangedMask = 0xFF;
			GetDeviceData().m_lastPrimitiveType = PRIMTYPE_Invalid;
			GetDeviceData().m_lastIndexBCC = BCC_Invalid;

			if ( textureData.m_needsInitialClear )
			{
				// This is initial usage of this depth target. We return false, so the renderer will clear it.
				textureData.m_needsInitialClear = false;
				return false;
			}

			// Htile clear successful, no need for the renderer to clear with the fullscreen quad!
			return true;
		}

		// No Htile acceleration, have the renderer clear with the fullscreen quad.
		return false;
	}

	Bool ClearStencilTarget( const TextureRef &target, Uint8 stencilValue, Int32 slice /*= -1*/ )
	{
		GPUAPI_ASSERT( slice == -1, TXT("DEPTH TARGETS WITH MULTIPLE SLICES NOT IMPLEMENTED") );
		GPUAPI_HALT("STENCIL CLEAR NOT IMPLEMENTED");
		return false;
	}

	Bool ClearColorTarget( const TextureRef &target, const Float* colorValue )
	{
		if ( !target )
		{
			return true;
		}

		STextureData &textureData = GetDeviceData().m_Textures.Data( target );
		sce::Gnm::RenderTarget& rt = textureData.m_aliasedAsRenderTargets[0];

		textureData.m_lastClearingFrame = FrameIndex();

		if (textureData.m_aliasedAsRenderTargets && rt.getCmaskFastClearEnable())
		{
			ClearCMask( GetSwapChainData().backBuffer->context, &rt );
			
			textureData.m_clearColor = *((Uint64*)colorValue);
			textureData.m_needsDecompress = true;

			return true;
		}
		else
		{
			// TODO: have a gpuapi embedded clearing here instead of returning false and leaving this for the renderer
			return false;
		}
	}

	Bool ClearDepthStencilTarget( const TextureRef &target, Float depthValue, Uint8 stencilValue, Int32 slice /*= -1*/ )
	{
		// not quite sure what slice==-1 means, lets assume it means the first slice
		if (slice < 0)
			slice = 0;

		sce::Gnmx::GfxContext& context = GetSwapChainData().backBuffer->context;
		STextureData &textureData = GetDeviceData().m_Textures.Data( target );
		const TextureDesc& desc = GetTextureDesc(target);

		Uint16 mipIndex = CalculateMipFromSliceIndex(desc, slice);
		if (mipIndex >= textureData.m_aliasedAsDepthStencilsSize)
		{
			GPUAPI_HALT ("CalculateMipFromSliceIndex OUT OF RANGE");
			return false;
		}

		sce::Gnm::DepthRenderTarget& depthStencil = textureData.m_aliasedAsDepthStencils[mipIndex];

		// if this is true we currently know that there is no stencil on this surface
		if (depthStencil.getHtileAccelerationEnable())
		{
			Int32 arraySlice = CalculateArrayIndexFromSlice(desc, slice);

			textureData.m_clearDepth = depthValue;
			textureData.m_needsDecompress = true;

			sce::Gnm::DepthRenderTarget depthStencilCopy = depthStencil;
			depthStencilCopy.setArrayView(arraySlice, arraySlice);

			ClearHTile( context, &depthStencilCopy, depthValue );
			GetDeviceData().m_shadersChangedMask = 0xFF;
			GetDeviceData().m_lastPrimitiveType = PRIMTYPE_Invalid;
			GetDeviceData().m_lastIndexBCC = BCC_Invalid;

			if ( textureData.m_needsInitialClear )
			{
				// This is initial usage of this depth target. We return false, so the renderer will clear it.
				textureData.m_needsInitialClear = false;
				return false;
			}

			// Htile clear successful, no need for the renderer to clear with the fullscreen quad!
			return true;
		}

		// No Htile acceleration, have the renderer clear with the fullscreen quad.
		return false;
	}

	Uint16 PopFetchShaderId(SDeviceData& dd)
	{
		Int32 i = dd.m_fetchShaderFreeIndex.Decrement() + 1;
		if (i <= 0)
		{
			GPUAPI_HALT("FATAL! Out of fetch shaders!");
			return 0;
		}
		Uint16 id = dd.m_fetchShaderFreeIndices[i];
		return id;
	}

	void PushFetchShaderId(SDeviceData& dd, Uint16 id)
	{
		Uint32 i = dd.m_fetchShaderFreeIndex.Increment();
		dd.m_fetchShaderFreeIndices[i] = id;
	}

	RED_FORCE_INLINE void SetNumInstancesInternal( SDeviceData &dd, sce::Gnmx::GfxContext &gfxc, Uint32 numInstances )
	{
		if ( numInstances != dd.m_lastNumInstances )
		{
			gfxc.setNumInstances( numInstances );
			dd.m_lastNumInstances = numInstances;
		}
	}

	RED_FORCE_INLINE void SetIndirectArgsInternal( SDeviceData &dd, sce::Gnmx::GfxContext &gfxc )
	{
		const SBufferData& bufferData = dd.m_Buffers.Data( dd.m_indirectArgs );
		gfxc.setBaseIndirectArgs( bufferData.GetMemoryPtr() );
	}

	Bool SetShadersInternal( SDeviceData &dd, sce::Gnmx::GfxContext &gfxc, Uint32 texturesBitfield[ShaderTypeMax] )
	{
		if ( WasShaderChanged( VertexShaderFlag | HullShaderFlag | DomainShaderFlag | GeometryShaderFlag ) || dd.m_vbChanged )
		{
		if ( dd.m_shadersSet[ VertexShader ] )
		{
			// Grab current vertex shader and layout
			SShaderData& vertexShaderData = dd.m_Shaders.Data( dd.m_shadersSet[ VertexShader ] );
			SVertexLayoutData& layoutData = dd.m_VertexLayouts.Data( dd.m_VertexLayout );

			texturesBitfield[VertexShader] = vertexShaderData.m_textureMask;

			Uint32 vertexLayoutIndex = dd.m_VertexLayout.id - 1;

			if ( dd.m_shadersSet[ HullShader ] )
			{
				GPUAPI_ASSERT( dd.m_shadersSet[ DomainShader ] );

				Uint16 lsFetchShaderId = vertexShaderData.m_lsFetchShaderId[vertexLayoutIndex];
				if ( lsFetchShaderId == 0 )
				{
					lsFetchShaderId = PopFetchShaderId(dd);
					dd.m_fetchShaders[ lsFetchShaderId ] = CreateFetchShader( layoutData, vertexShaderData, sce::Gnm::kShaderStageLs );
					vertexShaderData.m_lsFetchShaderId[vertexLayoutIndex] = lsFetchShaderId;
				}
				GPUAPI_ASSERT( dd.m_fetchShaders[ lsFetchShaderId ] );
				if ( !dd.m_fetchShaders[ lsFetchShaderId ] )
				{
					return false;
				}

				// Grab shader data
				SShaderData& hullShaderData = dd.m_Shaders.Data( dd.m_shadersSet[ HullShader ] );
				SShaderData& domainShaderData = dd.m_Shaders.Data( dd.m_shadersSet[ DomainShader ] );

				if ( hullShaderData.m_patchCount == 0 && hullShaderData.m_vgtPrimCount == 0 )
				{
					// Initialize patch count and vgt primitives count
					{
						// Calculate the minimum amount of LDS memory and VGPRs required by each patch
						Uint32 minLdsSize = sce::Gnm::computeLdsUsagePerPatchInBytesPerThreadGroup(	&hullShaderData.m_hsShader->m_hullStateConstants, vertexShaderData.m_lsShader->m_lsStride );
						Uint32 minVgprCount = hullShaderData.m_hsShader->getNumVgprs();

						// Provide a minimum of 4KiB of LDS and 16 VGPRs.
						Uint32 ldsSize = Red::Math::NumericalUtils::Max( minLdsSize, 4096u );
						Uint32 vgprCount = Red::Math::NumericalUtils::Max( minVgprCount, 16u );

						sce::Gnmx::computeVgtPrimitiveAndPatchCounts(
							&hullShaderData.m_vgtPrimCount,		// OUTPUT1: VGT primitive count
							&hullShaderData.m_patchCount,  		// OUTPUT2: Number of patches
							vgprCount,				   			// Max VGPRs for the HULL shader
							ldsSize,					   		// Max local data share size
							vertexShaderData.m_lsShader,   		// LS shader (vertex shader) binary
							hullShaderData.m_hsShader );		// HS shader (hull shader) binary
					}
					
					// Initialize the tessellation constants memory
					{
						sce::Gnm::TessellationDataConstantBuffer tessConstants;
						tessConstants.init( &hullShaderData.m_hsShader->m_hullStateConstants,
							vertexShaderData.m_lsShader->m_lsStride, hullShaderData.m_patchCount );

						// Allocate a GPU-mapped memory block for the tessellation constants.
						hullShaderData.m_tessConstantsMem = GPU_API_ALLOCATE( GpuMemoryPool_GPUInternal, MC_GPURingBuffers, sizeof( tessConstants ), sce::Gnm::kAlignmentOfBufferInBytes );

						// Copy the tessellation constants in GPU-mapped memory.
						memcpy( hullShaderData.m_tessConstantsMem, &tessConstants, sizeof(tessConstants) );
					}
				}

				// Activate the LS -> HS -> VS -> PS stages.
				SetActiveShaderStages( dd, gfxc, sce::Gnm::kActiveShaderStagesLsHsVsPs );
				internalSetLsHsShader(gfxc, &vertexShaderData, dd.m_fetchShaders[lsFetchShaderId], hullShaderData);
				texturesBitfield[HullShader] = hullShaderData.m_textureMask;

				internalSetVsShader(gfxc, &domainShaderData, nullptr);
				texturesBitfield[DomainShader] = domainShaderData.m_textureMask;

				gfxc.setVgtControl(	hullShaderData.m_vgtPrimCount - 1, sce::Gnm::kVgtPartialVsWaveDisable);
				
				// Bind the tessellation constants buffer.
				// The second parameter indicates the stage on which the domain shader runs.
				gfxc.setTessellationDataConstantBuffer(	hullShaderData.m_tessConstantsMem, sce::Gnm::kShaderStageVs );
			}
			else if ( dd.m_shadersSet[ GeometryShader ] )
			{
				// Geometry shader is on. Generate fetch shader for the ES stage.
				Uint16 esFetchShaderId = vertexShaderData.m_esFetchShaderId[vertexLayoutIndex];
				if ( esFetchShaderId == 0 )
				{
					esFetchShaderId = PopFetchShaderId(dd);
					dd.m_fetchShaders[ esFetchShaderId ] = CreateFetchShader( layoutData, vertexShaderData, sce::Gnm::kShaderStageEs );
					vertexShaderData.m_esFetchShaderId[vertexLayoutIndex] = esFetchShaderId;
				}
				GPUAPI_ASSERT( dd.m_fetchShaders[ esFetchShaderId ] );
				if ( !dd.m_fetchShaders[ esFetchShaderId ] )
				{
					return false;
				}

				// Grab geometry data
				SShaderData& geomShaderData = dd.m_Shaders.Data( dd.m_shadersSet[ GeometryShader ] );

				// Activate the ES -> GS -> VS -> PS stages.
				SetActiveShaderStages( dd, gfxc, sce::Gnm::kActiveShaderStagesEsGsVsPs );

				// Bind the vertex shader and its fetch shader to the ES stage.
				internalSetEsShader( gfxc, &vertexShaderData, dd.m_fetchShaders[ esFetchShaderId ] );

				// Bind the geometry shader binary to the GS stage and the internal copy 
				// shader to the VS stage. The shader compiler automatically splits the bulk
				// of the geometry shader functionalities from the copy shader so that Gnm
				// knows exactly how to bind the two parts to the relative stages.
				internalSetGsVsShaders( gfxc, &geomShaderData );
				texturesBitfield[GeometryShader] = geomShaderData.m_textureMask;

				// allocate new globalResourceTable so we can change parameters without stalling GPU
				void* globalResourceTable = g_DeviceData->m_constantBufferMem.Allocate(SCE_GNM_SHADER_GLOBAL_TABLE_SIZE, 16);
				gfxc.setGlobalResourceTableAddr(globalResourceTable);

				// when we change globalResourceTable under LCUE we must set all global state again
				gfxc.setGlobalDescriptor( sce::Gnm::kShaderGlobalResourceTessFactorBuffer, &dd.m_tessFactorsBuffer );

				// Initialize the ES->GS and the GS->VS ring buffers.
				gfxc.setEsGsRingBuffer(
					dd.m_EsGsRingBufferMem, sce::Gnm::kGsRingSizeSetup4Mb,
					vertexShaderData.m_esShader->m_memExportVertexSizeInDWord);
				gfxc.setGsVsRingBuffers(
					dd.m_GsVsRingBufferMem, sce::Gnm::kGsRingSizeSetup4Mb,
					geomShaderData.m_gsShader->m_memExportVertexSizeInDWord,
					geomShaderData.m_gsShader->m_maxOutputVertexCount);
			}
			else
			{
				Uint16 vsFetchShaderId = vertexShaderData.m_vsFetchShaderId[vertexLayoutIndex];
				if ( vsFetchShaderId == 0 )
				{
					vsFetchShaderId = PopFetchShaderId(dd);
					dd.m_fetchShaders[ vsFetchShaderId ] = CreateFetchShader( layoutData, vertexShaderData, sce::Gnm::kShaderStageVs );
					vertexShaderData.m_vsFetchShaderId[vertexLayoutIndex] = vsFetchShaderId;
				}
				GPUAPI_ASSERT( dd.m_fetchShaders[ vsFetchShaderId ] );
				if ( !dd.m_fetchShaders[ vsFetchShaderId ] )
				{
					return false;
				}

				// Activate the VS -> PS stages.
				SetActiveShaderStages( dd, gfxc, sce::Gnm::kActiveShaderStagesVsPs );

				internalSetVsShader(gfxc, &vertexShaderData, dd.m_fetchShaders[ vsFetchShaderId ]);
			}
		}
		}

		//if ( WasShaderChanged( PixelShaderFlag ) )
		{
			if ( dd.m_shadersSet[ PixelShader ] )
			{
				const SShaderData& psData = dd.m_Shaders.Data( dd.m_shadersSet[ PixelShader ] );
				internalSetPsShader(gfxc, &psData);
				texturesBitfield[PixelShader] = psData.m_textureMask;
			}
			else
			{
				internalSetPsShader(gfxc, nullptr);
			}
		}

		if ( WasShaderChanged( ComputeShaderFlag ) )
		{
			if ( dd.m_shadersSet[ ComputeShader ] )
			{
				const SShaderData& csData = dd.m_Shaders.Data( dd.m_shadersSet[ ComputeShader ] );
				internalSetCsShader(gfxc, &csData);
				texturesBitfield[ComputeShader] = csData.m_textureMask;
				gfxc.setShaderType( sce::Gnm::kShaderTypeCompute );
			}
			else
			{
				gfxc.setShaderType( sce::Gnm::kShaderTypeGraphics );
			}
		}


		return true;
	}

	void IncrementBatchIndex(sce::Gnmx::GfxContext &gfxc)
	{
		g_batchIndex++;
	}

	void IncrementBatchIndexCompute(sce::Gnmx::GfxContext &gfxc)
	{
		g_batchIndex++;
	}


	void InternalPostDraw( SDeviceData &dd, sce::Gnmx::GfxContext &gfxc )
	{
		IncrementBatchIndex(gfxc);

		if ( dd.m_shadersSet[ HullShader ] )
		{
			// Restore vertex geometry tessellators (VGT) to their default state (toggling between VGT every 256 vertices) 
			gfxc.setVgtControl( 256-1, sce::Gnm::kVgtPartialVsWaveDisable);
			SetActiveShaderStages( dd, gfxc, sce::Gnm::kActiveShaderStagesVsPs );
		}
		if ( dd.m_shadersSet[ GeometryShader ] )
		{
			gfxc.setGsModeOff();
			SetActiveShaderStages( dd, gfxc, sce::Gnm::kActiveShaderStagesVsPs );
		}

		if ( !dd.m_depthWriteNotified && dd.m_StateRenderStateCache.IsDepthWriteEnabled() && dd.m_StateRenderTargetSetup.depthTarget )
		{
			// This is the first drawcall that writes depth (or may have written depth) to this depth target since it was last decompressed.
			// Because of that, we need to make sure this depth target will be decompressed again if someone will try to bind it as a texture.
			STextureData& texData = dd.m_Textures.Data( dd.m_StateRenderTargetSetup.depthTarget );
			texData.m_needsDecompress = true;
			dd.m_depthWriteNotified = true;
		}

		dd.m_shadersChangedMask = 0;
	}


#ifdef SCE_GNMX_ENABLE_GFX_LCUE
	void SetTexturesWithMask(SDeviceData &dd, sce::Gnmx::GfxContext &gfxc, Uint32	texturesBitfield[ShaderTypeMax])
	{
		if ( dd.m_shadersSet[ HullShader ].isNull() || dd.m_shadersSet[ DomainShader ].isNull() )
		{
			if ( dd.m_shadersSet[ GeometryShader ].isNull() )
			{
				// vertex stage
				if ( dd.m_shadersSet[ VertexShader ] )
				{
					gfxc.setTexturesWithMask( sce::Gnm::kShaderStageVs, dd.m_texturesSet[ VertexShader ], MAX_PS_SAMPLERS, texturesBitfield[VertexShader] );
				}
			}
			else
			{
				gfxc.setTexturesWithMask( sce::Gnm::kShaderStageEs, dd.m_texturesSet[ VertexShader ], MAX_PS_SAMPLERS, texturesBitfield[VertexShader] );
				gfxc.setTexturesWithMask( sce::Gnm::kShaderStageGs, dd.m_texturesSet[ GeometryShader ], MAX_PS_SAMPLERS, texturesBitfield[GeometryShader] );
			}
		}
		else if (dd.m_shadersSet[ VertexShader ] )
		{
			// LDS stage regardless of geometry shader
			gfxc.setTexturesWithMask( sce::Gnm::kShaderStageLs, dd.m_texturesSet[ VertexShader ], MAX_PS_SAMPLERS, texturesBitfield[VertexShader] );

			if ( dd.m_shadersSet[ GeometryShader ].isNull() )
			{
				gfxc.setTexturesWithMask( sce::Gnm::kShaderStageHs, dd.m_texturesSet[ HullShader ], MAX_PS_SAMPLERS, texturesBitfield[HullShader] );
				gfxc.setTexturesWithMask( sce::Gnm::kShaderStageVs, dd.m_texturesSet[ DomainShader ], MAX_PS_SAMPLERS, texturesBitfield[DomainShader] );
			}
			else
			{
				gfxc.setTexturesWithMask( sce::Gnm::kShaderStageHs, dd.m_texturesSet[ DomainShader ], MAX_PS_SAMPLERS, texturesBitfield[DomainShader] );
				gfxc.setTexturesWithMask( sce::Gnm::kShaderStageEs, dd.m_texturesSet[ DomainShader ], MAX_PS_SAMPLERS, texturesBitfield[DomainShader] );
				gfxc.setTexturesWithMask( sce::Gnm::kShaderStageGs, dd.m_texturesSet[ GeometryShader ], MAX_PS_SAMPLERS, texturesBitfield[GeometryShader] );
			}
		}

		if ( !dd.m_shadersSet[ PixelShader ].isNull() )
		{
			gfxc.setTexturesWithMask(sce::Gnm::kShaderStagePs, dd.m_texturesSet[PixelShader], MAX_PS_SAMPLERS, texturesBitfield[PixelShader]);
		}

		if ( dd.m_shadersSet[ ComputeShader ] )
		{
			gfxc.setTexturesWithMask(sce::Gnm::kShaderStageCs, dd.m_texturesSet[ComputeShader], MAX_PS_SAMPLERS, texturesBitfield[ComputeShader]);
		}
	}
#endif

	void SetTexturesInternal( SDeviceData &dd, sce::Gnmx::GfxContext &gfxc, Uint32 texturesBitfield[ShaderTypeMax] )
	{
#ifdef SCE_GNMX_ENABLE_GFX_LCUE
		{
			SetTexturesWithMask(dd, gfxc, texturesBitfield);
			return;
		}
#endif

		// based on the current shader bindings we have to figure out where exactly the textures have to be bound


		// DOCUMENTATION: Overview of Mapping Shader Types and Shader Stages

		//                            TessOFF+GeomOFF          |          TessON+GeomOFF          |         TessON+GeomON            |         TessOff+GeomON
		// Vertex Shader   -            Vertex Stage           |             LDS Stage            |            LDS Stage             |          Export Stage
		// Hull Shader     -                 NA                |             Hull Stage           |           Hull Stage             |              NA
		// Domain Shader   -                 NA                |            Vertex Stage          |          Export Stage            |              NA
		// Geometry Shader -                 NA                |                NA                |  Geometry Stage + Vertex Stage   |  Geometry Stage + Vertex Stage
		// Pixel Shader    -            Pixel Stage            |           Pixel Stage            |           Pixel Stage            |          Pixel Stage
		// Compute Shader  -           Compute Stage           |          Compute Stage           |          Compute Stage           |         Compute Stage


		for ( Uint32 i=0; i<MAX_PS_SAMPLERS; ++i )
		{
			if ( dd.m_shadersSet[ HullShader ].isNull() || dd.m_shadersSet[ DomainShader ].isNull() )
			{
				if (dd.m_shadersSet[ GeometryShader ].isNull())
				{
					// vertex stage
					if ( dd.m_shadersSet[ VertexShader ] && dd.m_texturesSet[ VertexShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStageVs, i, 1, &(dd.m_texturesSet[ VertexShader ][i]) );
					}
				}
				else
				{
					// export stage
					if ( dd.m_texturesSet[ VertexShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStageEs, i, 1, &(dd.m_texturesSet[ VertexShader ][i]) );
					}
					if ( dd.m_texturesSet[ GeometryShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStageGs, i, 1, &(dd.m_texturesSet[ GeometryShader ][i]) );
					}
				}
			}
			else if ( dd.m_shadersSet[ VertexShader ] )
			{
				// LDS stage regardless of geometry shader
				if ( dd.m_texturesSet[ VertexShader ][i].isTexture() )
				{
					gfxc.setTextures( sce::Gnm::kShaderStageLs, i, 1, &(dd.m_texturesSet[ VertexShader ][i]) );
				}

				if ( dd.m_shadersSet[ GeometryShader ].isNull() )
				{
					if ( dd.m_texturesSet[ HullShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStageHs, i, 1, &(dd.m_texturesSet[ HullShader ][i]) );
					}
					if ( dd.m_texturesSet[ DomainShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStageVs, i, 1, &(dd.m_texturesSet[ DomainShader ][i]) );
					}
				}
				else
				{
					if ( dd.m_texturesSet[ DomainShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStageHs, i, 1, &(dd.m_texturesSet[ DomainShader ][i]) );
					}
					if ( dd.m_texturesSet[ DomainShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStageEs, i, 1, &(dd.m_texturesSet[ DomainShader ][i]) );
					}
					if ( dd.m_texturesSet[ GeometryShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStageGs, i, 1, &(dd.m_texturesSet[ GeometryShader ][i]) );
					}
				}
			}

			if ( dd.m_shadersSet[ PixelShader ] )
			{
				// HACK
				if ( i!=20 && i!=21 )
				{
					if ( dd.m_texturesSet[ PixelShader ][i].isTexture() )
					{
						gfxc.setTextures( sce::Gnm::kShaderStagePs, i, 1, &(dd.m_texturesSet[ PixelShader ][i]) );
					}
				}

				//gfxc.setTextures( sce::Gnm::kShaderStagePs, 22, MAX_PS_SAMPLERS - 22, &(dd.m_texturesSet[ PixelShader ][22]) );
			}

			if ( dd.m_shadersSet[ ComputeShader ] )
			{
				if ( dd.m_texturesSet[ ComputeShader ][i].isTexture() )
				{
					gfxc.setTextures( sce::Gnm::kShaderStageCs, i, 1, &(dd.m_texturesSet[ ComputeShader ][i]) );
				}
			}
		}
	}


	void SetSamplersInternal(SDeviceData &dd, sce::Gnmx::GfxContext &gfxc)
	{
		for (Uint32 shader_stage_i = 0; shader_stage_i < ShaderTypeMax; ++shader_stage_i)
		{
			sce::Gnm::ShaderStage gnm_shader_stage = MapToShaderStage((eShaderType)shader_stage_i);
			ShaderRef shader_ref = GetShader(dd, (eShaderType)shader_stage_i);
			if (shader_ref)
			{
				SShaderData& shader = dd.m_Shaders.Data(shader_ref);
				Uint32 samplerMask = shader.m_samplerMask;

 				while (samplerMask)
 				{
 					int slot_i = __builtin_ctz(samplerMask);	// find the lowest set bit
  					samplerMask ^= (1 << slot_i);				// clear the bit

					SamplerStateRef samplerRef = dd.m_SamplerStateCache[shader_stage_i][slot_i];
					//GPUAPI_ASSERT (samplerRef != SamplerStateRef::Null() && dd.m_SamplerStates.IsInUse(samplerRef), TXT("Shader expects sampler in slot %d but it is not set!"), slot_i);
					if (samplerRef != SamplerStateRef::Null() && dd.m_SamplerStates.IsInUse(samplerRef))
					{
						sce::Gnm::Sampler* gnm_sampler = &dd.m_SamplerStates.Data(samplerRef).m_sampler;
						gfxc.setSamplers(gnm_shader_stage, slot_i, 1, gnm_sampler);

						if (shader_stage_i == VertexShader)
						{
							if ( dd.m_shadersSet[ HullShader ] )
								gfxc.setSamplers(sce::Gnm::kShaderStageLs, slot_i, 1, gnm_sampler);
							if ( dd.m_shadersSet[ GeometryShader ] )
								gfxc.setSamplers(sce::Gnm::kShaderStageEs, slot_i, 1, gnm_sampler);
						}
					}
					else
					{
						GPUAPI_HALT ("Shader expects sampler in slot %d but it is not set!", slot_i);

						// provide fallback 
						sce::Gnm::Sampler gnm_sampler;
						gnm_sampler.init();
						gfxc.setSamplers(gnm_shader_stage, slot_i, 1, &gnm_sampler);
					}
 				}
			}


		}
	}


	void SetTextureUAVsInternal(SDeviceData &dd, sce::Gnmx::GfxContext &gfxc)
	{
		if ( dd.m_shadersSet[ ComputeShader ] )
		{
			Uint32 mask = 0x0000FFFF;	// D3TTODO parse this from the shader (see m_samplerMask)
			while (mask)
			{
				int slot_i = __builtin_ctz(mask);	// find the lowest set bit
				mask ^= (1 << slot_i);				// clear the bit

				const sce::Gnm::Texture* gnm_texture = &dd.m_textureUAVsSet[slot_i];
				if (gnm_texture->getBaseAddress() != 0)
				{
					gfxc.setRwTextures(sce::Gnm::kShaderStageCs, slot_i, 1, gnm_texture);
				}
			}			
		}
	}

	void SetBufferUAVsInternal(SDeviceData &dd, sce::Gnmx::GfxContext &gfxc)
	{
		if ( dd.m_shadersSet[ ComputeShader ] )
		{
			Uint32 mask = 0x0000FFFF; // D3TTODO parse this from the shader (see m_samplerMask)
			while (mask)
			{
				int slot_i = __builtin_ctz(mask);	// find the lowest set bit
				mask ^= (1 << slot_i);				// clear the bit

				const sce::Gnm::Buffer* gnm_buffer = &dd.m_bufferUAVsSet[slot_i];
				if (gnm_buffer->getBaseAddress() != 0)
				{
					gfxc.setRwBuffers( sce::Gnm::kShaderStageCs, slot_i, 1, gnm_buffer );
				}
			}			
		}
	}

	void SetBufferSRVsInternal(SDeviceData &dd, sce::Gnmx::GfxContext &gfxc)
	{
		for (Uint32 shader_stage_i = 0; shader_stage_i < ShaderTypeMax; ++shader_stage_i)
		{
			sce::Gnm::ShaderStage gnm_shader_stage = MapToShaderStage((eShaderType)shader_stage_i);

			// May need to adjust the gnm stage, depending on what stages are active.
			if ( shader_stage_i == VertexShader )
			{
				if ( dd.m_shadersSet[ HullShader ] )			gnm_shader_stage = sce::Gnm::kShaderStageLs;
				else if ( dd.m_shadersSet[ GeometryShader ] )	gnm_shader_stage = sce::Gnm::kShaderStageEs;
			}

			ShaderRef shader_ref = GetShader(dd, (eShaderType)shader_stage_i);
			if (shader_ref)
			{
				SShaderData& shader = dd.m_Shaders.Data(shader_ref);
				Uint32 mask = shader.m_resourceMask;
				while (mask)
				{
					int slot_i = __builtin_ctz(mask);	// find the lowest set bit
					mask ^= (1 << slot_i);				// clear the bit

					const BufferRef& buffer_ref = dd.m_bufferSRVsSet[shader_stage_i][slot_i];
					if (buffer_ref)
					{
						SBufferData& bufData = dd.m_Buffers.Data( buffer_ref );

						sce::Gnm::Buffer tempBuffer;
						if(bufData.m_Desc.category == BCC_Structured)
						{
							GpuApi::Uint32 elemCount = bufData.vsharp.m_elementCount;
							GpuApi::Uint32 stride = bufData.m_Desc.size / elemCount;
							tempBuffer.initAsRegularBuffer( (void *)bufData.GetMemoryPtr(), stride, elemCount );
						}
						else if(bufData.m_Desc.category == BCC_VertexSRV)
						{
							Uint32 elemCount = bufData.m_Desc.size / 8;
							tempBuffer.initAsDataBuffer((void *)bufData.GetMemoryPtr(), sce::Gnm::kDataFormatR16G16B16A16Unorm, elemCount);
						}
						else if(bufData.m_Desc.category == BCC_Index16BitUAV)
						{
							Uint32 elemCount = bufData.m_Desc.size / 2;
							tempBuffer.initAsDataBuffer((void *)bufData.GetMemoryPtr(), sce::Gnm::kDataFormatR16Uint, elemCount);
						}
						else
						{
							tempBuffer.initAsByteBuffer( (void *)bufData.GetMemoryPtr(), bufData.m_Desc.size );
						}
						tempBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

// 						if (shader_stage_i == PixelShader) {
// 							GPUAPI_LOG(TXT("BIND SRV: [%d][%d] 0x%016llx - 0x%016llx (%d bytes)"), shader_stage_i, slot_i, bufData.GetMemoryPtr(), bufData.GetMemoryPtr() + bufData.m_Desc.size, bufData.m_Desc.size);
// 						}

						gfxc.setBuffers( gnm_shader_stage, slot_i, 1, &tempBuffer );
					}
				}			
			}
		}
	}


	void DrawPrimitive( ePrimitiveType primitive, Uint32 startVertex, Uint32 primitiveCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();
		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		if ( startVertex != 0 )
		{
			GPUAPI_HALT( "NOT IMPLEMENTED!" );
			return;
		}

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		if ( !dd.m_shadersSet[ HullShader ].isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		SetVertexBuffersInternal(dd, gfxc);
		SetTexturesInternal(dd, gfxc, texturesBitfield);
		SetSamplersInternal(dd, gfxc);
		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, 1 );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );

		Int32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		gfxc.drawIndexAuto(vertexCount, startVertex, 0);

		ValidateContext("DrawPrimitive");

		InternalPostDraw( dd, gfxc );
	}

	void DrawPrimitiveRaw( ePrimitiveType primitive, Uint32 startVertex, Uint32 primitiveCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();
		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers (false);
		SetVertexBuffersInternal(dd, gfxc);
		SetTexturesInternal(dd, gfxc, texturesBitfield);
		SetSamplersInternal(dd, gfxc);
		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, 1 );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );

		Int32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		gfxc.drawIndexAuto( vertexCount );

		InternalPostDraw( dd, gfxc );

		ValidateContext("DrawPrimitiveRaw");
	}

	void DrawPrimitiveNoBuffers( ePrimitiveType primitive, Uint32 primitiveCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();
		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		// D3T - need to set vertex layout to something here to assign a fetch shader - even though no vertices are used!
		SetVertexFormatRaw(BCT_VertexSystemPos);
		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		SetTexturesInternal( dd, gfxc, texturesBitfield );
		SetSamplersInternal(dd, gfxc);
		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, 1 );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );

		Int32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		gfxc.drawIndexAuto( vertexCount );

		ValidateContext("DrawPrimitiveNoBuffers");

		InternalPostDraw( dd, gfxc );
	}

	void DrawIndexedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();
		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		if ( baseVertexIndex != 0 )
		{
			GPUAPI_HALT( "NOT IMPLEMENTED!" );
			return;
		}

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		if ( !dd.m_shadersSet[ HullShader ].isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		SBufferData& indexBufferData = dd.m_Buffers.Data( dd.m_IndexBuffer );
		Uint32 indexOffset = 0;

		switch ( indexBufferData.m_Desc.category )
		{
		case BCC_Index16Bit:
			indexOffset = startIndex * 2;
			break;
		case BCC_Index32Bit:
			indexOffset = startIndex * 4;
			break;
		default:
			GPUAPI_HALT( "" );
			return;
		}

		SetVertexBuffersInternal(dd, gfxc);
		SetTexturesInternal(dd, gfxc, texturesBitfield);
		SetSamplersInternal(dd, gfxc);
		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, 1 );
		SetIndexSizeInternal( dd, gfxc, indexBufferData.m_Desc.category );

		Int8* offsetedIndexBuffer = indexBufferData.GetMemoryPtr() + dd.m_IndexBufferOffset + indexOffset;

		gfxc.drawIndex( indexNum, offsetedIndexBuffer );

		ValidateContext("DrawIndexedPrimitive");

		InternalPostDraw( dd, gfxc );
	}

	void DrawInstancedIndexedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount, Uint32 instancesCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( chunkType );
		}

		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		SetNumInstancesInternal( dd, gfxc, instancesCount );

		SetVertexBuffersInternal( dd, gfxc );
		SetTexturesInternal( dd, gfxc, texturesBitfield );
		SetSamplersInternal(dd, gfxc);

		if ( baseVertexIndex != 0 )
		{
			GPUAPI_HALT( "NOT IMPLEMENTED!" );
			return;
		}

		if ( !dd.m_shadersSet[ HullShader ].isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		SBufferData& indexBufferData = dd.m_Buffers.Data( dd.m_IndexBuffer );
		Uint32 indexOffset = 0;

		switch ( indexBufferData.m_Desc.category )
		{
		case BCC_Index16Bit:
		case BCC_Index16BitUAV:
			indexOffset = startIndex * 2;
			break;
		case BCC_Index32Bit:
			indexOffset = startIndex * 4;
			break;
		default:
			GPUAPI_HALT( "" );
			return;
		}

		SetPrimitiveInternal( dd, gfxc, primitive );
		SetIndexSizeInternal( dd, gfxc, indexBufferData.m_Desc.category );

		Int8* offsetedIndexBuffer = indexBufferData.GetMemoryPtr() + dd.m_IndexBufferOffset + indexOffset;
		gfxc.drawIndex( indexNum, offsetedIndexBuffer );

		ValidateContext("DrawInstancedIndexedPrimitive");

		InternalPostDraw( dd, gfxc );
	}

	void DrawInstancedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 numVertices, Uint32 primitiveCount, Uint32 instancesCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( chunkType );
		}

		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		SetNumInstancesInternal( dd, gfxc, instancesCount );

		SetVertexBuffersInternal( dd, gfxc );
		SetTexturesInternal( dd, gfxc, texturesBitfield );
		SetSamplersInternal(dd, gfxc);

		if ( baseVertexIndex != 0 )
		{
			GPUAPI_HALT( "NOT IMPLEMENTED!" );
			return;
		}

		if ( !dd.m_shadersSet[ HullShader ].isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		SetPrimitiveInternal( dd, gfxc, primitive );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );
		gfxc.drawIndexAuto( numVertices );

		ValidateContext("DrawInstancedPrimitive");

		InternalPostDraw( dd, gfxc );
	}

	void DrawInstancedIndexedPrimitiveIndirect( ePrimitiveType primitive )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( chunkType );
		}

		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		SetPrimitiveInternal( dd, gfxc, primitive );

		SetNumInstancesInternal( dd, gfxc, 0 );

		SetVertexBuffersInternal( dd, gfxc );

		SetBufferSRVsInternal( dd, gfxc );

		SetIndirectArgsInternal( dd, gfxc );

		SBufferData& indexBufferData = dd.m_Buffers.Data( dd.m_IndexBuffer );
		Int8* indexBuffer = indexBufferData.GetMemoryPtr();
		if ( indexBufferData.m_Desc.category != dd.m_lastIndexBCC )
		{
			gfxc.setIndexSize( sce::Gnm::kIndexSize16 );
			dd.m_lastIndexBCC = indexBufferData.m_Desc.category;
		}
		gfxc.setIndexBuffer( indexBuffer );
		gfxc.setIndexCount( indexBufferData.m_Desc.size / 2 ); // todo: divide by 4 if kIndexSize32

		gfxc.stallCommandBufferParser();
		gfxc.drawIndexIndirect( 0 );

		ValidateContext("DrawInstancedPrimitiveIndirect");

		InternalPostDraw( dd, gfxc );
	}

	void DrawInstancedPrimitiveIndirect( ePrimitiveType primitive )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( chunkType );
		}

		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, 0 );

		SetVertexBuffersInternal( dd, gfxc );
		SetBufferSRVsInternal( dd, gfxc );
		SetIndirectArgsInternal( dd, gfxc );
		
		gfxc.stallCommandBufferParser();
		gfxc.drawIndirect( 0 );

		ValidateContext("DrawInstancedPrimitiveIndirect");

		InternalPostDraw( dd, gfxc );
	}

	void DrawInstancedPrimitiveNoBuffers( ePrimitiveType primitive, Uint32 vertexCount, Uint32 instancesCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();
		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		// D3T - need to set vertex layout to something here to assign a fetch shader - even though no vertices are used!
		SetVertexFormatRaw(BCT_VertexSystemPos);

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		SetTexturesInternal( dd, gfxc, texturesBitfield );
		SetSamplersInternal(dd, gfxc);
		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, instancesCount );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );

		gfxc.drawIndexAuto( vertexCount );

		ValidateContext("DrawInstancedPrimitiveNoBuffers");

		InternalPostDraw( dd, gfxc );
	}

	void DrawIndexedPrimitiveRaw( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();
		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}

		if ( baseVertexIndex != 0 )
		{
			GPUAPI_HALT( "NOT IMPLEMENTED!" );
			return;
		}

		if ( !dd.m_shadersSet[ HullShader ].isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		SBufferData& indexBufferData = dd.m_Buffers.Data( dd.m_IndexBuffer );
		Uint32 indexOffset = 0;

		switch ( indexBufferData.m_Desc.category )
		{
		case BCC_Index16Bit:
			indexOffset = startIndex * 2;
			break;
		case BCC_Index32Bit:
			indexOffset = startIndex * 4;
			break;
		default:
			GPUAPI_HALT( "" );
			return;
		}

		UpdateConstantBuffers (false);
		SetVertexBuffersInternal(dd, gfxc);
		SetTexturesInternal(dd, gfxc, texturesBitfield);
		SetSamplersInternal(dd, gfxc);
		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, 1 );
		SetIndexSizeInternal( dd, gfxc, indexBufferData.m_Desc.category );

		Int8* offsetedIndexBuffer = indexBufferData.GetMemoryPtr() + dd.m_IndexBufferOffset + indexOffset;

		gfxc.drawIndex( indexNum, offsetedIndexBuffer );

		ValidateContext("DrawIndexedPrimitiveRaw");
	}

	void DrawInstancedIndexedPrimitiveRaw( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount, Uint32 instancesCount )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		SDeviceData &dd = GetDeviceData();

		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers(false);

		SetNumInstancesInternal( dd, gfxc, instancesCount );

		SetVertexBuffersInternal( dd, gfxc );
		SetTexturesInternal( dd, gfxc, texturesBitfield );
		SetSamplersInternal(dd, gfxc);

		if ( baseVertexIndex != 0 )
		{
			GPUAPI_HALT( "NOT IMPLEMENTED!" );
			return;
		}

		if ( !dd.m_shadersSet[ HullShader ].isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		SBufferData& indexBufferData = dd.m_Buffers.Data( dd.m_IndexBuffer );
		Uint32 indexOffset = 0;

		switch ( indexBufferData.m_Desc.category )
		{
		case BCC_Index16Bit:
			indexOffset = startIndex * 2;
			break;
		case BCC_Index32Bit:
			indexOffset = startIndex * 4;
			break;
		default:
			GPUAPI_HALT( "" );
			return;
		}

		SetPrimitiveInternal( dd, gfxc, primitive );
		SetIndexSizeInternal( dd, gfxc, indexBufferData.m_Desc.category );

		Int8* offsetedIndexBuffer = indexBufferData.GetMemoryPtr() + dd.m_IndexBufferOffset + indexOffset;

		gfxc.drawIndex( indexNum, offsetedIndexBuffer );

		ValidateContext("DrawInstancedIndexedPrimitiveRaw");

		InternalPostDraw( dd, gfxc );
	}

	void DrawIndexedPrimitiveUP( ePrimitiveType primitive, Uint32 minIndex, Uint32 numVertices, Uint32 primitiveCount, void* indices, eIndexFormat indicesType, void* vertices, Uint32 vertexSize )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");
	}

	void DrawSystemPrimitive( ePrimitiveType primitive, Uint32 primitiveCount, eBufferChunkType vertexType, const void *vertexBuffer )
	{	
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		GPUAPI_ASSERT( vertexBuffer );

		SDeviceData &dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		if ( !dd.m_shadersSet[ HullShader ].isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Uint32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		Int32 vertexDataSize = vertexCount * GetChunkTypeStride( vertexType );

		eBufferLockFlag lockType = BLF_NoOverwrite;

		if ( dd.m_currentVertexWritePosition == 0 || (dd.m_currentVertexWritePosition + vertexDataSize) > g_drawPrimitiveUPBufferSize )
		{
			lockType = BLF_Discard;
			dd.m_currentVertexWritePosition = 0;
		}

		// Bind vertex buffer, map it, and copy data to it
		{
			void* lockedBufferPtr = GpuApi::LockBuffer( dd.m_drawPrimitiveUPVertexBuffer, lockType, dd.m_currentVertexWritePosition, vertexDataSize );
			Red::System::MemoryCopy( lockedBufferPtr, vertexBuffer, vertexDataSize );
			GpuApi::UnlockBuffer( dd.m_drawPrimitiveUPVertexBuffer );

			Uint32 offset = dd.m_currentVertexWritePosition;

			dd.m_VertexBuffers[0] = dd.m_drawPrimitiveUPVertexBuffer;
			dd.m_VertexBufferOffsets[0] = offset;
			dd.m_VertexBufferStrides[0] = GetChunkTypeStride( vertexType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( vertexType );
			dd.m_vbChanged = true;
		}


		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		SetVertexBuffersInternal(dd, gfxc);
		SetTexturesInternal(dd, gfxc, texturesBitfield);
		SetSamplersInternal(dd, gfxc);
		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, 1 );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );

		gfxc.drawIndexAuto( vertexCount );

		ValidateContext("DrawSystemPrimitive");

		InternalPostDraw( dd, gfxc );

		dd.m_currentVertexWritePosition += vertexDataSize;
		GPUAPI_ASSERT(dd.m_currentVertexWritePosition <= g_drawPrimitiveUPBufferSize);
	}

	void DrawSystemIndexedPrimitive( ePrimitiveType primitive, Uint32 minVertexIndex, Uint32 numVertices, Uint32 primitiveCount, const Uint16 *indexBuffer, eBufferChunkType vertexType, const void *vertexBuffer )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "Not inside render block!" );
		GPUAPI_ASSERT( indexBuffer && vertexBuffer );

		SDeviceData &dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		if ( !dd.m_shadersSet[ HullShader ].isNull() && !IsControlPointPatch( primitive ) )
		{
			// "Injecting" tessellation
			primitive = MapPrimitiveToControlPointPatch( primitive );
		}

		Uint32 indexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		Int32 vertexDataSize = numVertices * GetChunkTypeStride( vertexType );
		Int32 indexDataSize = indexCount * sizeof( Uint16 );

		eBufferLockFlag vertexLockType = BLF_NoOverwrite;
		eBufferLockFlag indexLockType = BLF_NoOverwrite;

		if ( dd.m_currentVertexWritePosition == 0 || (dd.m_currentVertexWritePosition + vertexDataSize) > g_drawPrimitiveUPBufferSize )
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

			Uint32 offset = dd.m_currentVertexWritePosition;

			dd.m_VertexBuffers[0] = dd.m_drawPrimitiveUPVertexBuffer;
			dd.m_VertexBufferOffsets[0] = offset;
			dd.m_VertexBufferStrides[0] = GetChunkTypeStride( vertexType );
			dd.m_VertexLayout = GetVertexLayoutForChunkType( vertexType );
			dd.m_vbChanged = true;
		}

		// Copy the index data to a ring buffer on garlic
		void* lockedIndexBuffer = GpuApi::LockBuffer( dd.m_drawPrimitiveUPIndexBuffer, indexLockType, dd.m_currentIndexWritePosition, indexDataSize );
		Red::System::MemoryCopy( lockedIndexBuffer, indexBuffer, indexDataSize );
		GpuApi::UnlockBuffer( dd.m_drawPrimitiveUPIndexBuffer );

		Uint32 texturesBitfield[ShaderTypeMax];
		if ( !SetShadersInternal( dd, gfxc, texturesBitfield ) )
		{
			return;
		}
		UpdateConstantBuffers();

		SetVertexBuffersInternal(dd, gfxc);
		SetTexturesInternal(dd, gfxc, texturesBitfield);
		SetSamplersInternal(dd, gfxc);
		SetPrimitiveInternal( dd, gfxc, primitive );
		SetNumInstancesInternal( dd, gfxc, 1 );
		SetIndexSizeInternal( dd, gfxc, BCC_Index16Bit );

		gfxc.drawIndex( indexCount, lockedIndexBuffer );

		ValidateContext("DrawSystemIndexedPrimitive");

		InternalPostDraw( dd, gfxc );

		dd.m_currentVertexWritePosition += vertexDataSize;
		dd.m_currentIndexWritePosition += indexDataSize;
		GPUAPI_ASSERT(dd.m_currentVertexWritePosition <= g_drawPrimitiveUPBufferSize);
		GPUAPI_ASSERT(dd.m_currentIndexWritePosition <= g_drawPrimitiveUPBufferSize);
	}

	void DispatchCompute( ShaderRef& computeShader, Uint32 x, Uint32 y, Uint32 z )
	{
		//GPUAPI_ASSERT( g_IsInsideRenderBlock, TXT("Not inside render block!") );
		GPUAPI_ASSERT( !computeShader.isNull() );

		SDeviceData &dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		if (!computeShader.isNull())
		{
			GPUAPI_ASSERT( dd.m_Shaders.Data(computeShader).m_type == ComputeShader );

			Uint32 texturesBitfield[ShaderTypeMax];
			SetShadersInternal( dd, gfxc, texturesBitfield );

			UpdateConstantBuffers(true, true);
			SetTexturesInternal(dd, gfxc, texturesBitfield);
			SetSamplersInternal(dd, gfxc);

			// bind TextureUAVs
			SetTextureUAVsInternal(dd, gfxc);
			SetBufferSRVsInternal(dd, gfxc);

			// bind BufferUAVs
			SetBufferUAVsInternal(dd, gfxc);

			// Dispatch the set of indicated thread groups along each dimension.
			gfxc.dispatch( x, y, z );

			// commands the GPU to write g_batchIndex to g_batchIndexGPU memory address once all CS shaders are finished and increments g_batchIndex
			IncrementBatchIndexCompute(gfxc);

			// Wait for compute jobs to finish and ensure data is updated for graphics pipeline.
			volatile uint64_t* label = (volatile uint64_t*)dd.m_constantBufferMem.Allocate( sizeof(uint64_t), 16 );
			*label = 0x0; // set the memory to have the val 0

			gfxc.m_dcb.writeAtEndOfPipe( sce::Gnm::kEopCsDone, sce::Gnm::kEventWriteDestMemory, const_cast<uint64_t*>(label), sce::Gnm::kEventWriteSource64BitsImmediate, 0x1, sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kCachePolicyLru );
			gfxc.m_dcb.waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1

			ValidateContext("DispatchCompute");
			dd.m_lastIndexBCC = BCC_Invalid;
			dd.m_lastPrimitiveType = PRIMTYPE_Invalid;
		}
	}

	void GrabBackBuffer( Uint32* targetBuffer, const Rect& r, const Uint32 fullHDChunks, const Uint32 chunkNumX, const Uint32 chunkNumY, const Uint32 fullHDResX )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		// Get backbuffer desc
		/*GpuApi::TextureDesc desc;
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
			Uint8* src = (Uint8*)mappedBackBuffer.pData + iy * mappedBackBuffer.RowPitch;
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
		Release( backBufferCopy );*/
	}


	void Flush()
	{
		//SDeviceData &dd = GetDeviceData();
		//SSwapChainData& scd = GetSwapChainData();
		//sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		// Submit what has been queued up to this point, and wait for the render target to be written to.
		//SubmitAll(gfxc);

		// Reset the context, and reinitialize.
		//ResetGfxcContext(gfxc);
	}

	Uint64 InsertFence()
	{
		SDeviceData &dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		Uint32* address = nullptr;
		Uint32 value = 0;
		Uint64 fence = dd.m_fenceManager.GetFence( FENCE_Graphics, address, value );
		GPUAPI_FATAL_ASSERT( address != nullptr, "CFenceManager::GetFence gave a null address!" );
		GPUAPI_FATAL_ASSERT( ( (size_t)address & 3 ) == 0, "Fence address must be 4-byte aligned" );

		gfxc.writeAtEndOfPipe(
			sce::Gnm::kEopFlushCbDbCaches,
			sce::Gnm::kEventWriteDestMemory,			address,
			sce::Gnm::kEventWriteSource32BitsImmediate,	value,
			sce::Gnm::kCacheActionInvalidateL1,
			sce::Gnm::kCachePolicyBypass );

		return fence;
	}

	Bool IsFencePending( Uint64 fence )
	{
		SDeviceData &dd = GetDeviceData();
		return dd.m_fenceManager.IsFencePending( fence );
	}

	void InsertWaitOnFence( Uint64 fence )
	{
		SDeviceData &dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		Uint32* addr = nullptr;
		Uint32 value = 0;
		dd.m_fenceManager.GetFenceInfo( fence, addr, value );
		GPUAPI_FATAL_ASSERT( addr != nullptr, "CFenceManager::GetFenceInfo gave a null address!" );
		GPUAPI_FATAL_ASSERT( ( (size_t)addr & 3 ) == 0, "Fence address must be 4-byte aligned" );

		// No reason to add the wait if we're already past the fence.
		if ( *addr < value )
		{
			gfxc.waitOnAddress( addr, 0xffffffff, sce::Gnm::kWaitCompareFuncGreaterEqual, value );
		}
	}

	void ReleaseFence( Uint64 fence )
	{
		// fences are just numbers in gnm so no need to release
	}

	void InsertWriteToMemory( volatile Uint64* dstAddress, Uint64 value )
	{
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		gfxc.writeImmediateAtEndOfPipe(
			sce::Gnm::kEopFlushCbDbCaches,
			(void*)dstAddress,
			value,
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2
			);
	}



	Bool ComputeTask_PrepareTextures( const ComputeTaskDesc& computeTaskDesc )
	{
		SDeviceData& dd = GetDeviceData();

		Bool needSync = false;

		// Decompress any textures, if needed. This is done by graphics context, so needs to be before the sync below.
		if ( computeTaskDesc.m_uav )
		{
			GPUAPI_ASSERT( dd.m_Textures.IsInUse( computeTaskDesc.m_uav ), TXT("Invalid TextureRef for UAV") );
			needSync |= DecompressIfRequired( dd.m_Textures.Data( computeTaskDesc.m_uav ) );
		}
		for ( Uint32 i = 0; i < computeTaskDesc.m_inputTextureCount; ++i )
		{
			if ( computeTaskDesc.m_inputTextures[ i ] )
			{
				GPUAPI_ASSERT( dd.m_Textures.IsInUse( computeTaskDesc.m_inputTextures[ i ] ), TXT("Invalid TextureRef for input texture %u"), i );
				needSync |= DecompressIfRequired( dd.m_Textures.Data( computeTaskDesc.m_inputTextures[ i ] ) );
			}
		}

		return needSync;
	}

	void ComputeTask_SetupConstantBuffers( const ComputeTaskDesc& computeTaskDesc, sce::Gnm::Buffer (&cbs)[16] )
	{
		SDeviceData& dd = GetDeviceData();

		GPUAPI_ASSERT( computeTaskDesc.m_constantBufferCount < ARRAY_COUNT( computeTaskDesc.m_constantBuffers ), TXT("Too many constant buffers: %u > %u"), computeTaskDesc.m_constantBufferCount, ARRAY_COUNT( computeTaskDesc.m_constantBuffers ) );

		for ( Uint32 i = 0; i < computeTaskDesc.m_constantBufferCount; ++i )
		{
			if ( computeTaskDesc.m_constantBuffers[ i ] )
			{
				GPUAPI_ASSERT( dd.m_Buffers.IsInUse( computeTaskDesc.m_constantBuffers[ i ] ), TXT("Invalid BufferRef for constant buffer %u"), i );

				const SBufferData& bufferData = dd.m_Buffers.Data( computeTaskDesc.m_constantBuffers[ i ] );

				GPUAPI_ASSERT( bufferData.m_Desc.category == BCC_Constant, TXT("Constant buffer %u is not BCC_Constant. %d"), i, bufferData.m_Desc.category );

				cbs[ i ].initAsConstantBuffer( bufferData.GetMemoryPtr(), bufferData.m_Desc.size );
			}
		}
	}

	void ComputeTask_SetupInputTextures( const ComputeTaskDesc& computeTaskDesc, sce::Gnm::Texture (&texs)[16] )
	{
		SDeviceData& dd = GetDeviceData();

		GPUAPI_ASSERT( computeTaskDesc.m_inputTextureCount < ARRAY_COUNT( computeTaskDesc.m_inputTextures ), TXT("Too many input textures: %u > %u"), computeTaskDesc.m_inputTextureCount, ARRAY_COUNT( computeTaskDesc.m_inputTextures ) );

		for ( Uint32 i = 0; i < computeTaskDesc.m_inputTextureCount; ++i )
		{
			if ( computeTaskDesc.m_inputTextures[ i ] )
			{
				const STextureData& textureData = dd.m_Textures.Data( computeTaskDesc.m_inputTextures[ i ] );

				GPUAPI_ASSERT( ( textureData.m_Desc.usage & TEXUSAGE_Samplable ) != 0, TXT("Input texture %u is not Samplable. %d"), i, textureData.m_Desc.usage );

				texs[ i ] = textureData.m_texture;
				texs[ i ].setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
			}
		}
	}

	void ComputeTask_SetupUAV( const ComputeTaskDesc& computeTaskDesc, sce::Gnm::Texture& tex )
	{
		SDeviceData& dd = GetDeviceData();

		const STextureData& textureData = dd.m_Textures.Data( computeTaskDesc.m_uav );
		GPUAPI_ASSERT( ( textureData.m_Desc.usage & TEXUSAGE_Samplable ) != 0, TXT("UAV texture is not Samplable. %d"), textureData.m_Desc.usage );

		tex = textureData.m_texture;
		tex.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
	}


	void AddSyncComputeTaskToQueue( const ComputeTaskDesc& computeTaskDesc )
	{
		SDeviceData& dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();

		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		ComputeTask_PrepareTextures( computeTaskDesc );

		// Bind shader
		GPUAPI_ASSERT( !computeTaskDesc.m_shader.isNull(), TXT("Cannot dispatch null compute shader") );
		GPUAPI_ASSERT( dd.m_Shaders.IsInUse( computeTaskDesc.m_shader ), TXT("Invalid ShaderRef") );
		const SShaderData& shaderData = dd.m_Shaders.Data( computeTaskDesc.m_shader );
		GPUAPI_ASSERT( shaderData.m_type == ComputeShader, TXT("Shader is not a compute shader. %d"), shaderData.m_type );
		gfxc.setBoundShaderResourceOffsets( sce::Gnm::kShaderStageCs, &shaderData.m_resourceOffsets );
		gfxc.setCsShader( shaderData.m_csShader );

		// Bind constant buffers
		if ( computeTaskDesc.m_constantBufferCount > 0 )
		{
			sce::Gnm::Buffer cbs[ ARRAY_COUNT( computeTaskDesc.m_constantBuffers ) ];
			ComputeTask_SetupConstantBuffers( computeTaskDesc, cbs );
			gfxc.setConstantBuffers( sce::Gnm::kShaderStageCs, 0, computeTaskDesc.m_constantBufferCount, cbs );
		}

		// Bind input textures
		if ( computeTaskDesc.m_inputTextureCount > 0 )
		{
			sce::Gnm::Texture texs[ ARRAY_COUNT( computeTaskDesc.m_inputTextures ) ];
			ComputeTask_SetupInputTextures( computeTaskDesc, texs );
			gfxc.setTextures( sce::Gnm::kShaderStageCs, 0, computeTaskDesc.m_inputTextureCount, texs );
		}


		// Bind input buffers
		// TODO


		// Bind UAV
		if ( computeTaskDesc.m_uav )
		{
			sce::Gnm::Texture rwtex;
			ComputeTask_SetupUAV( computeTaskDesc, rwtex );
			gfxc.setRwTextures( sce::Gnm::kShaderStageCs, computeTaskDesc.m_uavIndex, 1, &rwtex );
		}

		gfxc.dispatch( computeTaskDesc.m_threadGroupX, computeTaskDesc.m_threadGroupY, computeTaskDesc.m_threadGroupZ );


		// commands the GPU to write g_batchIndex to g_batchIndexGPU memory address once all CS shaders are finished and increments g_batchIndex
		IncrementBatchIndexCompute(gfxc);

		// Wait for compute jobs to finish and ensure data is updated for graphics pipeline.
		// TODO : Check if flushShaderCachesAndWait() is sufficient, instead of this? Looks like it should wait for shaders to finish.
		volatile Uint32* label = (Uint32*)gfxc.allocateFromCommandBuffer( sizeof(Uint32), sce::Gnm::kEmbeddedDataAlignment8 );
		*label = 0;

		gfxc.m_dcb.writeAtEndOfShader( sce::Gnm::kEosCsDone, const_cast<Uint32*>(label), 1 );
		gfxc.m_dcb.waitOnAddress( const_cast<Uint32*>(label), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 1 );

		gfxc.m_dcb.flushShaderCachesAndWait( sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0, sce::Gnm::kStallCommandBufferParserDisable ); // tell the CP to flush the L1$ and L2$

		ValidateContext("DispatchCompute");
		dd.m_lastIndexBCC = BCC_Invalid;
		dd.m_lastPrimitiveType = PRIMTYPE_Invalid;
	}

	void AddAsyncComputeTaskToQueue( const ComputeTaskDesc& computeTaskDesc )
	{
		SDeviceData& dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();

		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;
		sce::Gnmx::ComputeContext& cmpc = scd.backBuffer->computeContext;


		const Bool needGfxSync = ComputeTask_PrepareTextures( computeTaskDesc );

		// If we had to decompress some textures or something, or if the caller explicitly requested it,
		// synchronize with the graphics context. If it wasn't requested, and we didn't need to use it here
		// to decompress, then we shouldn't need to sync.
		if ( needGfxSync || computeTaskDesc.m_insertSync )
		{
			// Sync with graphics context.
			volatile Uint32* gfxSync = (Uint32*)cmpc.allocateFromCommandBuffer( sizeof(Uint32), sce::Gnm::kEmbeddedDataAlignment8 );
			*gfxSync = 0;

			gfxc.writeAtEndOfPipe(
				sce::Gnm::kEopFlushCbDbCaches,
				sce::Gnm::kEventWriteDestMemory,			(void*)gfxSync,
				sce::Gnm::kEventWriteSource32BitsImmediate,	1,
				sce::Gnm::kCacheActionInvalidateL1,
				sce::Gnm::kCachePolicyBypass );

			cmpc.waitOnAddress( (void*)gfxSync, 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 1 );
		}

		// Bind shader
		GPUAPI_ASSERT( !computeTaskDesc.m_shader.isNull(), TXT("Cannot dispatch null compute shader") );
		GPUAPI_ASSERT( dd.m_Shaders.IsInUse( computeTaskDesc.m_shader ), TXT("Invalid ShaderRef") );
		const SShaderData& shaderData = dd.m_Shaders.Data( computeTaskDesc.m_shader );
		GPUAPI_ASSERT( shaderData.m_type == ComputeShader, TXT("Shader is not a compute shader. %d"), shaderData.m_type );
		cmpc.setCsShader( shaderData.m_csShader, &shaderData.m_resourceOffsets );

		// Bind constant buffers
		if ( computeTaskDesc.m_constantBufferCount > 0 )
		{
			sce::Gnm::Buffer cbs[ ARRAY_COUNT( computeTaskDesc.m_constantBuffers ) ];
			ComputeTask_SetupConstantBuffers( computeTaskDesc, cbs );
			cmpc.setConstantBuffers( 0, computeTaskDesc.m_constantBufferCount, cbs );
		}

		// Bind input textures
		if ( computeTaskDesc.m_inputTextureCount > 0 )
		{
			sce::Gnm::Texture texs[ ARRAY_COUNT( computeTaskDesc.m_inputTextures ) ];
			ComputeTask_SetupInputTextures( computeTaskDesc, texs );
			cmpc.setTextures( 0, computeTaskDesc.m_inputTextureCount, texs );
		}


		// Bind input buffers
		// TODO


		// Bind UAV
		if ( computeTaskDesc.m_uav )
		{
			sce::Gnm::Texture rwtex;
			ComputeTask_SetupUAV( computeTaskDesc, rwtex );
			cmpc.setRwTextures( computeTaskDesc.m_uavIndex, 1, &rwtex );
		}

		cmpc.dispatch( computeTaskDesc.m_threadGroupX, computeTaskDesc.m_threadGroupY, computeTaskDesc.m_threadGroupZ );


		// Sync compute context with itself, so it won't start the next CS task immediately after.

		// TODO : Check if flushShaderCachesAndWait() is sufficient, instead of this? Looks like it should wait for shaders to finish.

		Uint32* address = nullptr;
		Uint32 value = 0;
		dd.m_fenceManager.GetFence( FENCE_AsyncCompute, address, value );
		GPUAPI_FATAL_ASSERT( address != nullptr, "CFenceManager::GetFence gave a null address!" );
		GPUAPI_FATAL_ASSERT( ( (size_t)address & 3 ) == 0, "Fence address must be 4-byte aligned" );

		// Write to marker when all done.
		cmpc.writeReleaseMemEvent(
			sce::Gnm::kReleaseMemEventCsDone,
			sce::Gnm::kEventWriteDestMemory,			address,
			sce::Gnm::kEventWriteSource32BitsImmediate,	value,
			sce::Gnm::kCacheActionInvalidateL1,
			sce::Gnm::kCachePolicyBypass);

		cmpc.waitOnAddress( address, 0xffffffff, sce::Gnm::kWaitCompareFuncGreaterEqual, value );
	}

	Uint64 KickoffAsyncComputeTasks()
	{
		SDeviceData& dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();

		//sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;
		sce::Gnmx::ComputeContext& cmpc = scd.backBuffer->computeContext;

		Uint32* address = nullptr;
		Uint32 value = 0;
		Uint64 fence = dd.m_fenceManager.GetFence( FENCE_AsyncCompute, address, value );
		GPUAPI_FATAL_ASSERT( address != nullptr, "CFenceManager::GetFence gave a null address!" );
		GPUAPI_FATAL_ASSERT( ( (size_t)address & 3 ) == 0, "Fence address must be 4-byte aligned" );

		// Write to marker when all done.
		cmpc.writeReleaseMemEvent(
			sce::Gnm::kReleaseMemEventCsDone,
			sce::Gnm::kEventWriteDestMemory,			address,
			sce::Gnm::kEventWriteSource32BitsImmediate,	value,
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2,
			sce::Gnm::kCachePolicyBypass);

		return fence;
	}


	volatile Uint64* AllocateValue()
	{
		return (volatile Uint64*) GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_Label, 8, 8 );
	}

	void DeallocateValue( volatile Uint64* value )
	{
		GPU_API_FREE( GpuMemoryPool_DisplayBuffer_Onion, MC_Label, (void*)value );
	}

#ifndef RED_FINAL_BUILD
	void DumpResourceStats()
	{
#ifdef GPU_API_DEBUG_PATH

		GPUAPI_HALT("NOT IMPLEMENTED");

		/*char buffer[512];

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
					const SQueryData& data = GetDeviceData().m_Queries.Data(i);
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
		OutputDebugStringA( buffer );*/
#endif
	}
#endif

	void SetComputeShaderConstsRaw( Uint32 dataSize, const void *dataBuffer )
	{
		if ( !dataSize || NULL == dataBuffer )
		{
			return;
		}

		SDeviceData &dd = GetDeviceData();

		void* mem = dd.m_constantBufferMem.Allocate(dataSize, sce::Gnm::kEmbeddedDataAlignment16);
		GPUAPI_ASSERT(mem, TXT("FAILED TO ALLOCATE ComputeShaderConstsRaw(%d)"), dataSize);
		Red::System::MemoryCopy( mem, dataBuffer, dataSize );

		dd.m_CustomCSConstantBuffer.initAsConstantBuffer( mem, dataSize );
		dd.m_CustomCSConstantBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );
	}

	void SetShaderDebugPath( const ShaderRef& shader, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_Shaders.IsInUse(shader) );
		SShaderData &data = GetDeviceData().m_Shaders.Data(shader);
		Red::System::StringCopy( data.m_debugPath, debugPath, ARRAY_COUNT(data.m_debugPath) );
#endif
	}

	//----------------------------------------------------------------------------

	Bool g_enableGpuValidation = false;

	void ValidateContext(const char* msg)
	{
#ifndef RED_FINAL_BUILD
#ifdef SCE_GNMX_ENABLE_GFX_LCUE
		{
			SSwapChainData& swapChainData = GetSwapChainData();
			sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;

			if (gfxc.m_lwcue.m_bufferCurrent >= gfxc.m_lwcue.m_bufferEnd[gfxc.m_lwcue.m_bufferIndex])
			{
				GPUAPI_HALT("LCUE GFXContext has exceeded buffer space, please increase numRingEntries in gpuApiDevice.cpp. This will likely cause GPU hang.");
			}
		}
#endif
#endif

		if (g_enableGpuValidation)
		{
			SSwapChainData& swapChainData = GetSwapChainData();
			sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;
			Int32 val = ValidateGfxc(gfxc);

			if (val == sce::Gnm::kValidationErrorNotEnabled)
			{
				g_enableGpuValidation = false;
			}
			else if (val != sce::Gnm::kSubmissionSuccess)
			{
				GPUAPI_HALT("GFXC Validate failed! [%s] 0x%08x {%s}", msg ? msg : "UNKNOWN", val, GetProfilerMarkerString());
			}
		}
	}

	void SetVsWaveLimits(Uint32 waveLimitBy16, Uint32 lateAllocWavesMinus1)
	{
#if 0	// GPU hangs appeared, this is very likely a reason.
		SSwapChainData& swapChainData = GetSwapChainData();
		sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;
		
		sce::Gnm::GraphicsShaderControl graphicsShaderControl;
		graphicsShaderControl.init();
		graphicsShaderControl.setVsWaveLimit( waveLimitBy16, lateAllocWavesMinus1 );
		gfxc.setGraphicsShaderControl( graphicsShaderControl );
#endif
	}

	void ResetVsWaveLimits()
	{
#if 0	// GPU hangs appeared, this is very likely a reason.
		SSwapChainData& swapChainData = GetSwapChainData();
		sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;

		sce::Gnm::GraphicsShaderControl graphicsShaderControl;
		graphicsShaderControl.init();
		gfxc.setGraphicsShaderControl( graphicsShaderControl );
#endif
	}
}
