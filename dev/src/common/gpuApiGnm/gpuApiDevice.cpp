/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../redMath/numericalutils.h"
#include "../redSystem/crt.h"
#include "../redThreads/redThreadsThread.h"

//---

typedef Red::System::Uint64 (*TGetGPUMemoryFunc)();
extern TGetGPUMemoryFunc			GSystemGPUMemoryStatFunc;

Red::System::Uint64 GetGPUMemoryStat()
{
	return GpuApi::GpuApiMemory::GetInstance()->GetMetricsCollector().GetTotalBytesAllocated();
}

//---

namespace GpuApi
{
	bool g_ValidateOnSubmit = false;

	void AddRef( const SwapChainRef &swapChain )
	{
		GPUAPI_ASSERT( GetDeviceData().m_SwapChains.IsInUse(swapChain) );
		GetDeviceData().m_SwapChains.IncRefCount( swapChain );
	}

	Int32 Release( const SwapChainRef &swapChain )
	{
		GPUAPI_ASSERT( swapChain );

		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_SwapChains.IsInUse(swapChain) );
		GPUAPI_ASSERT( dd.m_SwapChains.GetRefCount(swapChain) >= 1 );
		if ( 0 == dd.m_SwapChains.DecRefCount( swapChain ) )
		{
			SSwapChainData &data = dd.m_SwapChains.Data( swapChain );

			// Release resources
			GPUAPI_ASSERT( nullptr != data.surfaceAddresses[0] );

			for ( Uint32 i=0; i<SSwapChainData::displayBufferCount; ++i )
			{
				if ( data.surfaceAddresses[i] != nullptr )
				{
					GPU_API_FREE( GpuMemoryPool_SwapChain, MC_SwapChainTexture, data.surfaceAddresses[i] );
				}
			}

			// Destroy shit
			dd.m_SwapChains.Destroy( swapChain );
		}

		return 0; //HACK?
	}

	// ----------------------------------------------------------------------

	const eTextureFormat BACKBUFFER_FORMAT = TEXFMT_R8G8B8X8;

	// ----------------------------------------------------------------------

	Red::Threads::CMutex	g_ResourceMutex;
	SDeviceData*			g_DeviceData;

	// ----------------------------------------------------------------------

	void ResetGfxcContext(sce::Gnmx::GfxContext& gfxc, sce::Gnmx::ComputeContext& cmpc)
	{
		gfxc.reset();
		gfxc.initializeDefaultHardwareState();

		// global resource table is now allocated from constantBufferMem so must be set every frame
		void* globalResourceTable = g_DeviceData->m_constantBufferMem.Allocate(SCE_GNM_SHADER_GLOBAL_TABLE_SIZE, sce::Gnm::kEmbeddedDataAlignment16);
		gfxc.setGlobalResourceTableAddr(globalResourceTable);
		gfxc.setGlobalDescriptor( sce::Gnm::kShaderGlobalResourceTessFactorBuffer, &g_DeviceData->m_tessFactorsBuffer );


		cmpc.reset();
		cmpc.initializeDefaultHardwareState();

		// Set low priority, so we hopefully don't affect other operations too much.
		// Not entirely sure if this is needed here, or if it should just be set once at init??
		cmpc.setQueuePriority( g_DeviceData->m_computeQueue.m_vqueueId, 0 );
	}

	Int32 SubmitAllAndFlip(sce::Gnmx::GfxContext& gfxc, uint32_t videoOutHandler, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg)
	{
		return gfxc.submitAndFlip(videoOutHandler, rtIndex, flipMode, flipArg);
	}

	Int32 SubmitAll(sce::Gnmx::GfxContext& gfxc)
	{
		return gfxc.submit();
	}

	Int32 ValidateGfxc(sce::Gnmx::GfxContext& gfxc)
	{
		return gfxc.validate();
	}

	Int32 ValidatePreviousFrame(sce::Gnmx::GfxContext& gfxc)
	{
		// sorry cannot validate this at the moment!
		return 0;
	}


	// ----------------------------------------------------------------------

	Bool InitEnv()
	{
		GSystemGPUMemoryStatFunc = &GetGPUMemoryStat;

		const size_t c_deviceDataSize = sizeof( SDeviceData );
		void* deviceDataMemory = GPU_API_ALLOCATE( GpuMemoryPool_Device, MC_Device, c_deviceDataSize, 16 );
		if( !deviceDataMemory )
		{
			GPUAPI_FATAL( "Device data pool is not big enough to hold SDeviceData. Fatal error!" );
			return false;
		}

		g_DeviceData = new (deviceDataMemory) SDeviceData( g_ResourceMutex );

		return true;
	}

	Bool InitDevice( Uint32 width, Uint32 height, Bool fullscreen, Bool vsync )
	{
		if ( g_DeviceData->m_DeviceInitDone )
		{
			// It's possible to init device only once..
			return !g_DeviceData->m_DeviceShutDone;
		}

		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( 0 == dd.m_videoOutHandle );

		// Create swap chain
		dd.m_SwapChainRef = SwapChainRef( dd.m_SwapChains.Create( 1 ) );
		if ( dd.m_SwapChainRef.isNull() )
		{
			GPUAPI_HALT("Too many swapchains");
			return SwapChainRef::Null();
		}
		SSwapChainData &swapChainData = dd.m_SwapChains.Data( dd.m_SwapChainRef );

		// Allocate memory needed by geometry and tessellation stages, that will be shared between display buffers
		{
			// Global resource table will be assigned to display buffers only once
			dd.m_globalResourceTable = GPU_API_ALLOCATE( GpuMemoryPool_GPUInternal, MC_GlobalResTable, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE, sce::Gnm::kAlignmentOfBufferInBytes );
			if ( !dd.m_globalResourceTable )
			{
				GPUAPI_ERROR( TXT("Cannot allocate the global resource table memory.") );
				return false;
			}

			// The ring buffers will be set each time we bind a geometry shader
			dd.m_EsGsRingSizeInBytes = 4 * 1024 * 1024;
			dd.m_GsVsRingSizeInBytes = 4 * 1024 * 1024;
			dd.m_EsGsRingBufferMem = GPU_API_ALLOCATE( GpuMemoryPool_GPUInternal, MC_GPURingBuffers, dd.m_EsGsRingSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes );
			dd.m_GsVsRingBufferMem = GPU_API_ALLOCATE( GpuMemoryPool_GPUInternal, MC_GPURingBuffers, dd.m_GsVsRingSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes );

			// Allocate the tessellation factors ring buffer.
			{
				const Uint32 alignment = 64 * 1024;
				off_t tsFactorDirectMemOffset = 0;

				// NOTE: WHY bypassing our memory allocator? To be able to map the memory into -> sce::Gnm::getTheTessellationFactorRingBufferBaseAddress
				Int32 ret = sceKernelAllocateDirectMemory( 0, SCE_KERNEL_MAIN_DMEM_SIZE, sce::Gnm::kTfRingSizeInBytes, alignment, SCE_KERNEL_WC_GARLIC, &tsFactorDirectMemOffset );

				// The system expects the tessellation factors ring buffer to be mapped to a specific virtual address,...
				dd.m_tessFactorsMem = sce::Gnm::getTessellationFactorRingBufferBaseAddress();
				if ( ret == SCE_OK )
				{
					// ... map our newly allocated memory to that address then.
					ret = sceKernelMapDirectMemory( &dd.m_tessFactorsMem, sce::Gnm::kTfRingSizeInBytes, 
												SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_ALL,
												0, tsFactorDirectMemOffset, alignment );
				}
				if ( ret != SCE_OK || dd.m_tessFactorsMem != sce::Gnm::getTessellationFactorRingBufferBaseAddress() )
				{
					GPUAPI_HALT( "Allocation of the tessellation factor ring buffer failed!" );
				}

				// Create a Buffer object wrapping the tessellation factors ring buffer.
				dd.m_tessFactorsBuffer.initAsTessellationFactorBuffer( dd.m_tessFactorsMem, sce::Gnm::kTfRingSizeInBytes );
				dd.m_tessFactorsBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC ); // it's written to, so it's GPU-coherent
			}
		}

		const uint32_t dcbSizeInBytes = 8 * 1024 * 1024;	// 8Mb DCB size
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		const uint32_t resBufferSizeInBytes = 8 * 1024 * 1024;
#endif

		for ( Uint32 i=0; i<SSwapChainData::displayBufferCount; ++i )
		{
			SSwapChainData::DisplayBuffer& displayBuffer = swapChainData.displayBuffers[ i ];

			// Initialize GFX context
			{
				// I had to increase this from the default of 16 to get LCUE working with witcher3.redgame
				const uint32_t numRingEntries = 90;
				displayBuffer.cueHeap = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Garlic, MC_CUEHeap, sce::Gnmx::ConstantUpdateEngine::computeHeapSize( numRingEntries ), sce::Gnm::kAlignmentOfBufferInBytes );
				if( !displayBuffer.cueHeap )
				{
					GPUAPI_ERROR( TXT("Cannot allocate the CUE heap memory.") );
					return false;
				}

				// Allocate heap for Draw and Constant Command Buffers
				
#if !defined(SCE_GNMX_ENABLE_GFX_LCUE)
				const uint32_t ccbSizeInBytes = 4 * 1024 * 1024;

				// Allocate heap for Constant Update Engine
				const Uint32 cpRamShadowSize = sce::Gnmx::ConstantUpdateEngine::computeCpRamShadowSize();
				if ( cpRamShadowSize > 0 )
				{
					displayBuffer.cpRamShadow = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_CPRAMShadow, cpRamShadowSize, 16 );
				}
				else
				{
					displayBuffer.cpRamShadow = nullptr;
				}

				displayBuffer.dcbBuffer = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_DCB, dcbSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes );
				if( !displayBuffer.dcbBuffer )
				{
					GPUAPI_ERROR( TXT("Cannot allocate the draw command buffer memory.") );
					return false;
				}
				
				displayBuffer.ccbBuffer = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_CCB, ccbSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes );
				if( !displayBuffer.ccbBuffer )
				{
					GPUAPI_ERROR( TXT("Cannot allocate the constants command buffer memory.") );
					return false;
				}
				
				displayBuffer.context.init(	
					displayBuffer.cpRamShadow, displayBuffer.cueHeap, numRingEntries,
					displayBuffer.dcbBuffer, dcbSizeInBytes,
					displayBuffer.ccbBuffer, ccbSizeInBytes
					);


				// TODO : Compute context is not set up here

#else

				// LCUE does not require cpRamShadow or CCB
				displayBuffer.cpRamShadow = nullptr;
				displayBuffer.dcbBuffer = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_DCB, dcbSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes );
				displayBuffer.ccbBuffer = nullptr;

				// TODO : Check sizes. Especially DCB/Res. Can probably be smaller?
				displayBuffer.computeDcbBuffer = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_DCB, dcbSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes );
				displayBuffer.computeResBuffer = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Garlic, MC_DCB, resBufferSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes );
				displayBuffer.computeResTable = (uint32_t*)GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Garlic, MC_DCB, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE, sce::Gnm::kAlignmentOfBufferInBytes );

				displayBuffer.context.init(	displayBuffer.cueHeap, numRingEntries, displayBuffer.dcbBuffer, dcbSizeInBytes,	nullptr, 0 );

				displayBuffer.computeContext.init( displayBuffer.computeDcbBuffer, dcbSizeInBytes,
					displayBuffer.computeResBuffer, resBufferSizeInBytes, (void*)displayBuffer.computeResTable );

				// Only use half the compute units for async compute?
				//Uint32 mask = ( 1 << ( sce::Gnm::kNumCusPerSe / 2 ) ) - 1;
				//displayBuffer.computeContext.setComputeResourceManagement( sce::Gnm::kShaderEngine0, mask );
				//displayBuffer.computeContext.setComputeResourceManagement( sce::Gnm::kShaderEngine1, mask );

#endif

				displayBuffer.context.initializeDefaultHardwareState();
				
				// Set up the global resource table.
				displayBuffer.context.setGlobalResourceTableAddr( dd.m_globalResourceTable );

				displayBuffer.computeContext.initializeDefaultHardwareState();
			}
			
			// Allocate and initialize back buffer
			{
				// Init as GpuApi Texture Data
				displayBuffer.backBufferRef	= TextureRef( dd.m_Textures.Create( 1 ) );
				STextureData &backBufferData		= dd.m_Textures.Data( displayBuffer.backBufferRef );
				backBufferData.m_Desc.type			= TEXTYPE_2D;
				backBufferData.m_Desc.format		= GpuApi::BACKBUFFER_FORMAT;
				backBufferData.m_Desc.usage			= TEXUSAGE_BackBuffer;
				backBufferData.m_Desc.width			= width;
				backBufferData.m_Desc.height		= height;
				backBufferData.m_Desc.initLevels	= 1;

				// Init GNM object
				sce::Gnm::TileMode tileMode;
				sce::Gnm::DataFormat format = sce::Gnm::kDataFormatB8G8R8A8Unorm;
				if ( sce::GpuAddress::computeSurfaceTileMode( &tileMode, sce::GpuAddress::kSurfaceTypeColorTargetDisplayable, format, 1 ) != sce::GpuAddress::kStatusSuccess )
				{
					GPUAPI_ERROR( TXT("Cannot compute the tile mode for the back buffer surface.") );
					return false;
				}

				backBufferData.m_aliasedAsRenderTargets = new sce::Gnm::RenderTarget[1];
				backBufferData.m_aliasedAsRenderTargetsSize = 1;

				const sce::Gnm::SizeAlign sizeAlign = backBufferData.m_aliasedAsRenderTargets[0].init(
					width,
					height,
					1,						// Number of slices
					format,
					tileMode,
					sce::Gnm::kNumSamples1,		// Number of samples per pixel
					sce::Gnm::kNumFragments1,    // Number of fragments per pixel
					NULL,					// No CMASK
					NULL);					// No FMASK

				// Allocate actual surface buffer
				swapChainData.surfaceAddresses[i] = GPU_API_ALLOCATE( GpuMemoryPool_SwapChain, MC_SwapChainTexture, sizeAlign.m_size, sizeAlign.m_align );

				if( !swapChainData.surfaceAddresses[i] )
				{
					GPUAPI_ERROR( TXT("Cannot allocate the back buffer memory.") );
					return false;
				}

				backBufferData.m_aliasedAsRenderTargets[0].setAddresses( swapChainData.surfaceAddresses[i], 0, 0 );

				// We gonna need m_texture for CopyRect, but TODO: make this code less ugly
				backBufferData.m_texture.initFromRenderTarget( backBufferData.m_aliasedAsRenderTargets, false );
			}

			// Allocate and initialize depth/stencil buffers
			{
				/*// Compute the tiling mode for the depth buffer
				sce::Gnm::DataFormat depthFormat = sce::Gnm::DataFormat::build( sce::Gnm::kZFormat32Float);
				sce::Gnm::TileMode depthTileMode;
				if( !GpuAddress::computeSurfaceTileMode( &depthTileMode, sce::GpuAddress::kSurfaceTypeDepthOnlyTarget, depthFormat, 1 ) )
				{
					GPUAPI_ERROR( TXT("Cannot compute the tile mode for the depth surface.") );
					return false;
				}

				displayBuffer.depthStencilRef		= TextureRef( dd.m_Textures.Create( 1 ) );
				STextureData &depthBufferData		= dd.m_Textures.Data( displayBuffer.backBufferRef );
				depthBufferData.m_Desc.type			= TEXTYPE_2D;
				depthBufferData.m_Desc.format		= TEXFMT_D32F;
				depthBufferData.m_Desc.usage		= TEXUSAGE_DepthStencil;
				depthBufferData.m_Desc.width		= width;
				depthBufferData.m_Desc.height		= height;
				depthBufferData.m_Desc.initLevels	= 1;

				// Initialize the depth buffer descriptor
				Gnm::SizeAlign stencilSizeAlign;
				Gnm::SizeAlign htileSizeAlign;
				Gnm::SizeAlign depthTargetSizeAlign = depthBufferData.m_aliasedAsDepthStencil.init(
					width,
					height,
					depthFormat.getZFormat(),
					Gnm::kStencil8,
					depthTileMode,
					Gnm::kNumFragments1,
					kStencilFormat != Gnm::kStencilInvalid ? &stencilSizeAlign : NULL,
					kHtileEnabled ? &htileSizeAlign : NULL);

				// Initialize the HTILE buffer, if enabled
				if( kHtileEnabled )
				{
					void *htileMemory = allocator.allocate(htileSizeAlign, SCE_KERNEL_WC_GARLIC);
					if( !htileMemory )
					{
						printf("Cannot allocate the HTILE buffer\n");
						return SCE_KERNEL_ERROR_ENOMEM;
					}

					displayBuffers[i].depthTarget.setHtileAddress(htileMemory);
				}

				// Initialize the stencil buffer, if enabled
				void *stencilMemory = NULL;
				if( kStencilFormat != Gnm::kStencilInvalid )
				{
					stencilMemory = allocator.allocate(stencilSizeAlign, SCE_KERNEL_WC_GARLIC);
					if( !stencilMemory )
					{
						printf("Cannot allocate the stencil buffer\n");
						return SCE_KERNEL_ERROR_ENOMEM;
					}
				}

				// Allocate the depth buffer
				void *depthMemory = allocator.allocate(depthTargetSizeAlign, SCE_KERNEL_WC_GARLIC);
				if( !depthMemory )
				{
					printf("Cannot allocate the depth buffer\n");
					return SCE_KERNEL_ERROR_ENOMEM;
				}
				displayBuffers[i].depthTarget.setAddresses(depthMemory, stencilMemory);*/
			}

			// Initialize state flag
			displayBuffer.state = (volatile Uint32*) GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_Label, 4, 8 );
			if( !displayBuffer.state )
			{
				GPUAPI_ERROR( TXT("Cannot allocate a display buffer state label\n") );
				return false;
			}

			displayBuffer.state[0] = SSwapChainData::EDisplayBufferIdle;

			// Fix tess factors pointer in global resource table
			displayBuffer.context.setGlobalDescriptor( sce::Gnm::kShaderGlobalResourceTessFactorBuffer, &dd.m_tessFactorsBuffer );
			displayBuffer.context.setInstanceStepRate( 1, 1 );
			//displayBuffer.context.setTessellationFactorBuffer( dd.m_tessFactorsMem );
		}

		// Initialize video output
		{
			// Open the video output port.
			dd.m_videoOutHandle = sceVideoOutOpen(0, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL);
			if( dd.m_videoOutHandle < 0 )
			{
				GPUAPI_ERROR( TXT("sceVideoOutOpen failed: 0x%08X."), dd.m_videoOutHandle );
				return false;
			}

			// Set up the initialization parameters for the VideoOut library.
			GPUAPI_ASSERT( swapChainData.backBuffer->backBufferRef );
			STextureData &backBufferData = dd.m_Textures.Data( swapChainData.backBuffer->backBufferRef );
			SceVideoOutBufferAttribute videoOutBufferAttribute;
			sceVideoOutSetBufferAttribute(
				&videoOutBufferAttribute,
				SCE_VIDEO_OUT_PIXEL_FORMAT_A8R8G8B8_SRGB,
				SCE_VIDEO_OUT_TILING_MODE_TILE,
				SCE_VIDEO_OUT_ASPECT_RATIO_16_9,
				backBufferData.m_aliasedAsRenderTargets[0].getWidth(),
				backBufferData.m_aliasedAsRenderTargets[0].getHeight(),
				backBufferData.m_aliasedAsRenderTargets[0].getPitch() 
				);

			// Register the display buffers to the slot: [0..kDisplayBufferCount-1].
			Int32 ret = sceVideoOutRegisterBuffers(
				dd.m_videoOutHandle,
				0, // Start index.
				swapChainData.surfaceAddresses,
				swapChainData.displayBufferCount,
				&videoOutBufferAttribute
				);
			if( ret != SCE_OK )
			{
				GPUAPI_ERROR( TXT("sceVideoOutRegisterBuffers failed: 0x%08X."), ret );
				return false;
			}

			// Initialize the flip rate: 0: 60Hz, 1: 30Hz or 2: 20Hz.
			ret = sceVideoOutSetFlipRate( dd.m_videoOutHandle, 1 );
			if( ret != SCE_OK )
			{
				GPUAPI_ERROR( TXT("sceVideoOutSetFlipRate failed: 0x%08X."), ret );
				return false;
			}
		}

		// Initialize end-of-pipe event queue
		{
			// Create the event queue used to synchronize with end-of-pipe interrupts.
			Int32 ret = sceKernelCreateEqueue( &dd.m_eopEventQueue, "EOP QUEUE" );
			if( ret != SCE_OK )
			{
				GPUAPI_ERROR( TXT("sceKernelCreateEqueue failed: 0x%08X."), ret );
				return false;
			}

			// Register for the end-of-pipe events.
			ret = sce::Gnm::addEqEvent( dd.m_eopEventQueue, sce::Gnm::kEqEventGfxEop, NULL );
			if( ret != SCE_OK )
			{
				GPUAPI_ERROR( TXT("Gnm::addEqEvent failed: 0x%08X."), ret );
				return false;
			}
		}

		dd.m_VSConstants = static_cast< Float* >( GPU_API_ALLOCATE( GpuMemoryPool_ConstantBuffers, MC_ConstantBuffer, NUM_VSC_REGS * 4 * sizeof( Float ), sce::Gnm::kAlignmentOfBufferInBytes ) );
		dd.m_PSConstants = static_cast< Float* >( GPU_API_ALLOCATE( GpuMemoryPool_ConstantBuffers, MC_ConstantBuffer, NUM_PSC_REGS * 4 * sizeof( Float ), sce::Gnm::kAlignmentOfBufferInBytes ) );
		
		Uint32 constantBufferMemSize = 8 * 1024 * 1024;		// shared between 2 frames
		void* constantBufferMem = GPU_API_ALLOCATE(GpuMemoryPool_ConstantBuffers, MC_ConstantBuffer, constantBufferMemSize, 256);
		if( !constantBufferMem )
		{
			GPUAPI_ERROR( TXT("Cannot allocate ConstantBuffer ring buffer memory") );
			return false;
		}
		dd.m_constantBufferMem.Init( constantBufferMem, constantBufferMemSize);

		// Finalize
		GPUAPI_ASSERT( dd.m_SamplerStates.IsEmpty() );
		GPUAPI_ASSERT( dd.m_Buffers.IsEmpty() );
		GPUAPI_ASSERT( dd.m_Queries.IsEmpty() );


		// TODO : Check size of this buffer, and maybe computeQueue.initialize arguments. Mainly just ripped this from a sample.
		const Uint32 ringSize = 1 * 1024 * 1024;
		dd.m_computeQueueBuffer = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_DCB, ringSize, 256 );
		dd.m_computeQueueBufferReadPtr = GPU_API_ALLOCATE( GpuMemoryPool_DisplayBuffer_Onion, MC_DCB, 4, 16 );
		Red::System::MemoryZero( dd.m_computeQueueBuffer, ringSize );

		dd.m_computeQueue.initialize( 0, 3 );
		dd.m_computeQueue.map( dd.m_computeQueueBuffer, ringSize / 4, dd.m_computeQueueBufferReadPtr );


		// Create presets and internal resources
		InitInternalResources( false );

		// initialize sampler state cache
		GpuApi::InvalidateSamplerStates();

		// Finish :)
		g_DeviceData->m_DeviceInitDone = true;

		return true;
	}

	Bool IsInit()
	{
		return g_DeviceData->m_videoOutHandle > 0;
	}

	eDeviceCooperativeLevel TestDeviceCooperativeLevel()
	{
		// HACK DX10 testing device cooperative level is different now
		//SDeviceData &dd = GetDeviceData();
		//HRESULT hRet = dd.m_swapChain->Present(0, DXGI_PRESENT_TEST);
		return DEVICECOOPLVL_Operational;// MapCooperativeLevel( hRet );
	}

	Bool ResetDevice()
	{
		/*
		SDeviceData &dd = GetDeviceData();

		// Restore state to default
		{
			//GPUAPI_ASSERT( dd.m_BackBuffer );
			//RenderTargetSetup rtSetup;
			//rtSetup.SetColorTarget( 0, dd.m_BackBuffer );
			//rtSetup.SetViewportFromTarget( dd.m_BackBuffer );
			//SetupRenderTargets( rtSetup );

			BindSamplersNull( 0, MAX_PS_SAMPLERS, PixelShader );
			BindSamplersNull( 0, MAX_VS_SAMPLERS, VertexShader );
			SetSamplerStateCommon( 0, MAX_PS_SAMPLER_STATES, SAMPSTATEPRESET_WrapPointNoMip, PixelShader );
			SetSamplerStateCommon( 0, MAX_VS_SAMPLER_STATES, SAMPSTATEPRESET_WrapPointNoMip, VertexShader );
			ID3D11RenderTargetView* nullRTV = nullptr;
			GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);
			GetDeviceContext()->VSSetShader( nullptr, nullptr, 0 );
			GetDeviceContext()->PSSetShader( nullptr, nullptr, 0 );

			BindVertexBuffers( 0, 0, nullptr, nullptr, nullptr );
			BindIndexBuffer( BufferRef::Null() );
		}

		// Release some internal references

		ShutInternalResources( false );

		// Reset device

		//D3DPRESENT_PARAMETERS params;
		//Red::System::MemoryCopy( &params, &g_DeviceData->m_PresentParams, sizeof(D3DPRESENT_PARAMETERS) );
		////GPUAPI_ASSERT( DEVICECOOPLVL_NotReset == TestDeviceCooperativeLevel() );
		//if ( !SUCCEEDED( GetDevice()->Reset( &params ) ) )
		//{
		//	return false;
		//}

		// Reset succeeded, so rebuild internal references

		InitInternalResources( true );

		// Set implicit states

		ForceSetImplicitD3DStates();

		// Set explicit states

		ForceSetExplicitD3DStates();
		*/
		return true;
	}

	void ShutDevice()
	{
//		SDeviceData &dd = GetDeviceData();
//
//		if ( !dd.m_pDevice )
//		{
//			return;
//		}
//
//		GPUAPI_ASSERT(  dd.m_DeviceInitDone );
//		GPUAPI_ASSERT( !dd.m_DeviceShutDone );
//
//		// Restore state to default
//		// TODO clean this up
//		{
//			GPUAPI_ASSERT( dd.m_BackBuffer );
//			//RenderTargetSetup rtSetup;
//			//rtSetup.SetColorTarget( 0, dd.m_BackBuffer );
//			//rtSetup.SetViewportFromTarget( dd.m_BackBuffer );
//			//SetupRenderTargets( rtSetup );
//
//			ID3D11RenderTargetView* rts = NULL;
//			ID3D11DepthStencilView* dss = NULL;
//
//			GetDeviceContext()->OMSetRenderTargets(1, &rts, dss);
//
//			BindSamplersNull( 0, MAX_PS_SAMPLERS, PixelShader );
//			BindSamplersNull( 0, MAX_VS_SAMPLERS, VertexShader );
//			
//			ID3D11SamplerState* stateObject = NULL;
//			for (int i =0; i<8; ++i)
//			{
//				GetDeviceContext()->PSSetSamplers( i, 1, &stateObject );
//				GetDeviceContext()->VSSetSamplers( i, 1, &stateObject );
//				GetDeviceContext()->DSSetSamplers( i, 1, &stateObject );
//				GetDeviceContext()->HSSetSamplers( i, 1, &stateObject );
//				GetDeviceContext()->GSSetSamplers( i, 1, &stateObject );
//			}
//
//			BindVertexBuffers( 0, 0, nullptr, nullptr, nullptr );
//			BindIndexBuffer( BufferRef::Null() );
//
//			GpuApi::InvalidateSamplerStates();
//			GpuApi::SetCustomDrawContext(GpuApi::DSSM_Max, GpuApi::RASTERIZERMODE_Max, GpuApi::BLENDMODE_Max);
//			GetDeviceContext()->OMSetDepthStencilState( NULL, 0 );
//			GetDeviceContext()->OMSetBlendState( NULL, NULL, 0xFFFFFFFF );
//			GetDeviceContext()->RSSetState( NULL );
//
//			ULONG refCount = 0;
//			refCount = dd.m_GlobalVSConstantBuffer->Release();
//			refCount = dd.m_CustomVSConstantBuffer->Release();
//			refCount = dd.m_SkinningVSConstantBuffer->Release();
//			refCount = dd.m_SpecificVSConstantBuffer->Release();
//			refCount = dd.m_GlobalPSConstantBuffer->Release();
//			refCount = dd.m_CustomPSConstantBuffer->Release();
//			refCount = dd.m_SpecificPSConstantBuffer->Release();
//			refCount = dd.m_CustomCSConstantBuffer->Release();
//		}
//
//		// Remove shadow state references
//		dd.m_StateRenderTargetSetup.ChangeAllRefCounts( false );
//		dd.m_pImmediateContext->ClearState();
//		dd.m_pImmediateContext->Flush();
//		// Shut internal resources
//		ShutInternalResources( true );
//
//#if WAITING_FOR_DEX_TO_FIX_GLOBAL_TEXTURES
//		// Remove shit so that release ASSERTION would not fire up.
//		dd.m_Textures.DestroyAll();
//		dd.m_SamplerStates.DestroyAll();
//		dd.m_Buffers.DestroyAll();
//		dd.m_Queries.DestroyAll();
//#endif
//
//		dd.m_StateRenderStateCache.Clear();
//
//		GpuApi::SafeRelease(dd.m_drawPrimitiveUPIndexBuffer);
//		GpuApi::SafeRelease(dd.m_drawPrimitiveUPVertexBuffer);
//
//		for ( Uint32 ili = 0; ili < BCT_Max * MAX_SHADER_COUNT; ++ili )
//		{
//			if (dd.m_InputLayouts[ili])
//			{
//				ULONG refcount = dd.m_InputLayouts[ili]->Release();
//				GPUAPI_ASSERT(refcount==0, TXT( "input layout leak" ) );
//				dd.m_InputLayouts[ili] = NULL;
//			}
//		}
//
//		dd.m_pImmediateContext->ClearState();
//		dd.m_pImmediateContext->Flush();
//		ULONG contextRefCount = dd.m_pImmediateContext->Release();
//		dd.m_pImmediateContext = NULL;
//
//		// Remove device
//		ULONG deviceRefCount = dd.m_pDevice->Release();
//		dd.m_pDevice = nullptr;
//
//		// Safe info that we already had device init/shut
//		dd.m_DeviceShutDone = true;
//
//		//
//		GPUAPI_ASSERT( !IsInit() );
	}

	const Capabilities& GetCapabilities()
	{
		return GetDeviceData().m_Caps;
	}

	void SetRenderSettings( const RenderSettings &newSettings )
	{
		SDeviceData &dd = GetDeviceData();

		const Bool invalidateSamplerStatePresets = dd.m_RenderSettings.SamplersChanged( newSettings );

		// Copy
		dd.m_RenderSettings = newSettings;
		
		// Validate
		dd.m_RenderSettings.Validate();

		// Apply some immediate states
		dd.m_StateRenderStateCache.SetWireframe( newSettings.wireframe );

		if ( invalidateSamplerStatePresets )
		{
			// TODO: P.Czatrowski, no SamplerStates for now
			// ResetSamplerStates();
		}

	}

	void SetRenderSettingsWireframe( Bool enable )
	{
		if ( enable != GetRenderSettings().wireframe )
		{
			RenderSettings sett = GetRenderSettings();
			sett.wireframe = enable;
			SetRenderSettings( sett );
		}
	}

	const RenderSettings& GetRenderSettings()
	{
		return GetDeviceData().m_RenderSettings;
	}

	void ShutBackbufferRef( bool dropRefs )
	{
	}

	SwapChainRef CreateSwapChainWithBackBuffer( const SwapChainDesc& swapChainDesc )
	{
		SDeviceData &dd = GetDeviceData();
		
		return dd.m_SwapChainRef;
	}

	SSwapChainData& GetSwapChainData()
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_SwapChainRef );
		
		return dd.m_SwapChains.Data( dd.m_SwapChainRef );
	}

	Bool HasMinimumRequiredGPU()
	{
		return true;
	}

	void SetBackBufferFromSwapChain( const SwapChainRef& swapChain )
	{
		//GetDeviceData().m_SwapChainRef = swapChain;
	}

	void ResizeBackbuffer( Uint32 width, Uint32 height, const SwapChainRef& swapChain )
	{
		/*SDeviceData &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( backBuffer );
		ID3D11RenderTargetView* nullRT = NULL;
		GetDeviceContext()->OMSetRenderTargets( 1, &nullRT, NULL );

		td.m_pTexture->AddRef();
		ULONG texRefCount = td.m_pTexture->Release();

		td.m_pRenderTargetView->AddRef();
		ULONG rtvRefCount = td.m_pRenderTargetView->Release();

		SAFE_RELEASE( td.m_pRenderTargetView );
		SAFE_RELEASE( td.m_pTexture );

		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );

		if ( width != 0 && height != 0 )
		{
			HRESULT res = scd.m_swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
			if (res != S_OK)
			{
				GPUAPI_HALT( TXT( "Failed to resize backbuffer. Error code: %d" ), res );
			}
		}

		td.m_Desc.width = width;
		td.m_Desc.height = height;

		// Create a render target view
		ID3D11Texture2D *pBackBuffer;
		ID3D11RenderTargetView* renderTargetView;

		HRESULT hr = scd.m_swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
		if( FAILED( hr ) )
		{
			GPUAPI_HALT( TXT( "Failed to get backbuffer. Error code: %d" ), hr );
		}

		hr = GetDevice()->CreateRenderTargetView( pBackBuffer, NULL, &renderTargetView );
		if( FAILED( hr ) )
		{
			GPUAPI_HALT( TXT( "Failed to create rendertarget for backbuffer. Error code: %d" ), hr );
		}
#ifdef GPU_API_DEBUG_PATH
		else
		{
			renderTargetView->SetPrivateData( WKPDID_D3DDebugObjectName, 7, "backBuf" );
		}
#endif

		td.m_pTexture = pBackBuffer;
		td.m_pRenderTargetView = renderTargetView;*/
	}

	Bool GetFullscreenState( const SwapChainRef& swapChain )
	{
		return true;
	}

	Bool ToggleFullscreen( const SwapChainRef& swapChain, Bool fullscreen )
	{
		//SDeviceData &dd = GetDeviceData();
		//SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );

		/*HRESULT res = scd.m_swapChain->SetFullscreenState( fullscreen, nullptr );
		if (res == S_OK)
		{
			scd.m_fullscreen = fullscreen;
			return true;
		}
		else
		{
			return false;
		}*/
		return false;
	}

	Bool ToggleFullscreen( const SwapChainRef& swapChain, Bool fullscreen, Int32 outputMonitorIndex )
	{
		return false;
	}

	Int32 GetMonitorCount()
	{
		return 1;
	}

	Bool GetMonitorCoordinates( Int32 monitorIndex, Int32& top, Int32& left, Int32& bottom, Int32& right )
	{
		return false;
	}

	void SetGammaForSwapChain( const SwapChainRef& swapChain, Float gammaValue )
	{
		SceVideoOutColorSettings settings;
		int32_t result = sceVideoOutColorSettingsSetGamma( &settings, gammaValue );
		if (result == SCE_OK)
		{
			SDeviceData &dd = GetDeviceData();
			result = sceVideoOutAdjustColor( dd.m_videoOutHandle, &settings );
		}
	}

	void GetNativeResolution( Uint32 outputIndex, Int32& width, Int32& height )
	{
		width = 1920;
		height = 1080;
	}

	Bool EnumerateDisplayModes( Int32 monitorIndex, Uint32* outNum, DisplayModeDesc** outDescs /*= nullptr*/ )
	{
		// Just a stub
		(*outNum) = 0;
		return true;
	}

	void* CreateContext()
	{
		/*ID3D11DeviceContext* context = NULL;
		GetDevice()->CreateDeferredContext( 0, &context );
		return context;*/
		return NULL;
	}

	void SubmitContext( void* deferredContext )
	{
		/*ID3D11CommandList* commandList = NULL;
		((ID3D11DeviceContext*) deferredContext)->FinishCommandList( true, &commandList );
		GetDeviceContext()->ExecuteCommandList( commandList, true );
		ULONG refcount = ((ID3D11DeviceContext*) deferredContext)->Release();
		GPUAPI_ASSERT( refcount == 0, TXT( "Context not destroyed" ) );
		commandList->Release();*/
	}

	void CancelContext( void* deferredContext )
	{
		//ULONG refcount = ((ID3D11DeviceContext*) deferredContext)->Release();
		//GPUAPI_ASSERT( refcount == 0, TXT( "Context not destroyed" ) );
	}

	void* GetCommandListFromContext( void* deferredContext )
	{
		//ID3D11CommandList* commandList = NULL;
		//((ID3D11DeviceContext*) deferredContext)->FinishCommandList( true, &commandList );
		//return commandList;
		return NULL;
	}

	void SubmitCommandList( void* commandList )
	{
		//ID3D11CommandList* d3dCommandList = (ID3D11CommandList*)commandList;
		//GetDeviceContext()->ExecuteCommandList( d3dCommandList, true );
		//d3dCommandList->Release();
	}

	void CancelCommandList( void* commandList )
	{
		//ID3D11CommandList* d3dCommandList = (ID3D11CommandList*)commandList;
		//d3dCommandList->Release();
	}

	const int MAX_PROFILER_NAMES = 50;
	char	g_profilerNameStorage	[MAX_PROFILER_NAMES * 256]	= {0};
	char*	g_profilerNames			[MAX_PROFILER_NAMES]		= {0};
	int		g_profilerNumNames									= 0;

	const char* GetProfilerMarkerString()
	{
		return g_profilerNameStorage;
	}

	void BeginProfilerBlock( const Char* name )
	{
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		//HACK convert to ansi char
		AnsiChar blockName[ 256 ];
		Red::System::SNPrintF( &(blockName[0]), 256, "%ls", name );

		char* namePointer = 0;
		if (g_profilerNumNames == 0)
		{
			namePointer = g_profilerNames[0] = g_profilerNameStorage;
		}
		else if (g_profilerNumNames < MAX_PROFILER_NAMES)
		{
			namePointer = g_profilerNames[g_profilerNumNames] = g_profilerNames[g_profilerNumNames-1] + strlen(g_profilerNames[g_profilerNumNames-1]);
			namePointer = strcat (namePointer, " / ");
		}
		else
		{
			GPUAPI_HALT("Too many profiler blocks - increase MAX_PROFILER_NAMES")
		}

		if (namePointer)
		{
			strcat (namePointer, blockName);
		}

		g_profilerNumNames++;

		gfxc.pushMarker( &(blockName[0]) );
	}

	void EndProfilerBlock()
	{
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		gfxc.popMarker();

		if (g_profilerNumNames <= MAX_PROFILER_NAMES)
		{
			// Zero the start of the last marker to remove it from the string
			*g_profilerNames[g_profilerNumNames-1] = 0;
			g_profilerNames[g_profilerNumNames-1] = 0;	// clear this even though it is not necessary
		}
		g_profilerNumNames = g_profilerNumNames>0 ? g_profilerNumNames-1 : 0;
	}

	void SetMarker( const Char* name )
	{
	}

#ifndef RED_FINAL_BUILD
	void GetResourceUseStats( SResourceUseStats& stats )
	{
		SDeviceData &dd = GetDeviceData();

		stats.m_usedTextures		= dd.m_Textures.GetUsedCount();
		stats.m_usedBuffers			= dd.m_Buffers.GetUsedCount();
		stats.m_usedSamplerStates	= dd.m_SamplerStates.GetUsedCount();
		stats.m_usedQueries			= dd.m_Queries.GetUsedCount();
		stats.m_usedShaders			= dd.m_Shaders.GetUsedCount();
		stats.m_usedVertexLayouts	= dd.m_VertexLayouts.GetUsedCount();
		stats.m_usedSwapChains		= dd.m_SwapChains.GetUsedCount();

		stats.m_maxTextures			= dd.m_Textures._MaxResCount;
		stats.m_maxBuffers			= dd.m_Buffers._MaxResCount;
		stats.m_maxSamplerStates	= dd.m_SamplerStates._MaxResCount;
		stats.m_maxQueries			= dd.m_Queries._MaxResCount;
		stats.m_maxShaders			= dd.m_Shaders._MaxResCount;
		stats.m_maxVertexLayouts	= dd.m_VertexLayouts._MaxResCount;
		stats.m_maxSwapChains		= dd.m_SwapChains._MaxResCount;
	}
#endif

	Bool MultiGPU_IsActive()
	{
		return MultiGPU_GetNumGPUs() > 1;
	}

	Uint32 MultiGPU_GetNumGPUs()
	{
		return 1;
	}

	void MultiGPU_BeginEarlyPushTexture( const TextureRef &tex )
	{
		// empty
	}

	void MultiGPU_EndEarlyPushTexture( const TextureRef &tex )
	{
		// empty
	}

	namespace Hacks
	{
		sce::Gnmx::GfxContext& GetGfxContext()
		{
			sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;
			return gfxc;
		}

		sce::Gnm::RenderTarget& GetRenderTarget()
		{
			SDeviceData &dd = GetDeviceData();
			SSwapChainData& swapChainData = GetSwapChainData();

			GPUAPI_ASSERT( swapChainData.backBuffer->backBufferRef );
			STextureData& backBufferData = dd.m_Textures.Data( swapChainData.backBuffer->backBufferRef );
			return backBufferData.m_aliasedAsRenderTargets[0];
		}

		sce::Gnm::DepthRenderTarget& GetDepthStencil()
		{
			SDeviceData &dd = GetDeviceData();
			SSwapChainData& swapChainData = GetSwapChainData();

			GPUAPI_ASSERT( swapChainData.backBuffer->backBufferRef );
			STextureData& backBufferData = dd.m_Textures.Data( swapChainData.backBuffer->backBufferRef );
			return backBufferData.m_aliasedAsDepthStencils[0];
		}

		sce::Gnm::RenderTarget* GetTextureRTV( const TextureRef& ref )
		{
			GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
			return ref ? &(GetDeviceData().m_Textures.Data(ref).m_aliasedAsRenderTargets[0]) : nullptr;
		}

		sce::Gnm::Texture* GetTextureSRV( const TextureRef& ref )
		{
			GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
			return ref ? &(GetDeviceData().m_Textures.Data(ref).m_texture) : nullptr;
		}
	}
}
