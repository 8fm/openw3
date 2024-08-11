/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

//////////////////////////////////////////////////////////////////////////
// Memory Pool Definitions
//	This file should not be included directly unless you plan on defining the 
//	DECLARE_MEMORY_POOL macro to do something interesting!

// Macro Usage:
// To add a new pool, add DECLARE_MEMORY_POOL(Pool Label, Allocator Type, Pool flags)

// Default flags for physical memory (cached, mapped to CPU only)
#define DEFAULT_DURANGO_PHYSICAL_MEMFLAGS	Red::MemoryFramework::Allocator_AccessCpu | Red::MemoryFramework::Allocator_Cached | Red::MemoryFramework::Allocator_StaticSize
#define DURANGO_GPU_FLAGS ( Red::MemoryFramework::Allocator_AccessCpuGpu | Red::MemoryFramework::Allocator_Cached | Red::MemoryFramework::Allocator_GpuNonCoherent | Red::MemoryFramework::Allocator_StaticSize )
#define DURANGO_GPU_COHERENT_FLAGS ( Red::MemoryFramework::Allocator_AccessCpuGpu | Red::MemoryFramework::Allocator_Cached | Red::MemoryFramework::Allocator_Use64kPages | Red::MemoryFramework::Allocator_StaticSize )
#define ORBIS_GARLIC_FLAGS ( Red::MemoryFramework::Allocator_DirectMemory | Red::MemoryFramework::Allocator_UseGarlicBus | Red::MemoryFramework::Allocator_AccessCpuReadWrite | Red::MemoryFramework::Allocator_AccessGpuReadWrite | Red::MemoryFramework::Allocator_StaticSize )
#define ORBIS_ONION_FLAGS ( Red::MemoryFramework::Allocator_DirectMemory | Red::MemoryFramework::Allocator_UseOnionBus | Red::MemoryFramework::Allocator_AccessCpuReadWrite | Red::MemoryFramework::Allocator_AccessGpuReadWrite | Red::MemoryFramework::Allocator_StaticSize )
#define ORBIS_FLEXIBLE_ONION_FLAGS ( Red::MemoryFramework::Allocator_FlexibleMemory | Red::MemoryFramework::Allocator_UseOnionBus | Red::MemoryFramework::Allocator_AccessCpuReadWrite | Red::MemoryFramework::Allocator_AccessGpuReadWrite | Red::MemoryFramework::Allocator_StaticSize )

#ifdef DECLARE_MEMORY_POOL

#ifdef RED_PLATFORM_DURANGO

	// This contains texture data that is kept resident after they have been uploaded (i.e. dynamic textures written via cpu)
	DECLARE_MEMORY_POOL( GpuMemoryPool_Textures, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, DURANGO_GPU_FLAGS | RED_MEMORY_DURANGO_ALLOCID( 3 ) )

	// This contains all vertex/index buffers used by the GPU
	DECLARE_MEMORY_POOL( GpuMemoryPool_Buffers, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, DEFAULT_DURANGO_PHYSICAL_MEMFLAGS |  RED_MEMORY_DURANGO_ALLOCID( 4 ) )

	// This contains all constant buffers used by the GPU
	DECLARE_MEMORY_POOL( GpuMemoryPool_ConstantBuffers, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, DURANGO_GPU_COHERENT_FLAGS | RED_MEMORY_DURANGO_ALLOCID( 9 ) )

	// Locked buffers forward to buffers pools
	#define GpuMemoryPool_LockedBufferData GpuMemoryPool_Buffers

	// This needs to be big enough to contain the largest mesh vb that we will write to 
	DECLARE_MEMORY_POOL( GpuMemoryPool_ShaderInclude, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, DEFAULT_DURANGO_PHYSICAL_MEMFLAGS |  RED_MEMORY_DURANGO_ALLOCID( 5 ) )

		// This needs to be big enough to hold SDeviceData
	DECLARE_MEMORY_POOL( GpuMemoryPool_Device, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, DEFAULT_DURANGO_PHYSICAL_MEMFLAGS | RED_MEMORY_DURANGO_ALLOCID( 6 ) )

	// This contains just the streamed env probe textures
	DECLARE_MEMORY_POOL( GpuMemoryPool_EnvProbes, ::Red::MemoryFramework::GpuAllocatorThreadSafe, 0 )

	// In-place cooked data is loaded into here
	DECLARE_MEMORY_POOL( GpuMemoryPool_InPlaceRenderData, ::Red::MemoryFramework::GpuAllocatorThreadSafe, Red::MemoryFramework::Allocator_NoBreakOnOOM )

	// In-place cooked meshes
	DECLARE_MEMORY_POOL( GpuMemoryPool_InPlaceMeshBuffers, ::Red::MemoryFramework::GpuAllocatorThreadSafe, Red::MemoryFramework::Allocator_NoBreakOnOOM )

	DECLARE_MEMORY_POOL( GpuMemoryPool_InPlaceDefragTemp, ::Red::MemoryFramework::GpuAllocatorThreadSafe, 0 )

#elif defined ( RED_PLATFORM_ORBIS )

	// This contains arbitrary types of onion allocations
	DECLARE_MEMORY_POOL( GpuMemoryPool_DefaultOnion, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, ORBIS_ONION_FLAGS )

	// This contains just the shader headers
	DECLARE_MEMORY_POOL( GpuMemoryPool_SmallOnion, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, ORBIS_ONION_FLAGS )

	// This contains just the constant buffers
	DECLARE_MEMORY_POOL( GpuMemoryPool_ConstantBuffers, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, ORBIS_FLEXIBLE_ONION_FLAGS )

	// This contains arbitrary types of garlic allocations
	DECLARE_MEMORY_POOL( GpuMemoryPool_DefaultGarlic, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, ORBIS_GARLIC_FLAGS )

	// This contains buffers critical for hardware stages to exchange data (global resource table, ring buffers, etc.)
	DECLARE_MEMORY_POOL( GpuMemoryPool_GPUInternal, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, ORBIS_GARLIC_FLAGS )

	// This just contains swap chain surfaces
	DECLARE_MEMORY_POOL( GpuMemoryPool_SwapChain, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, ORBIS_GARLIC_FLAGS | ::Red::MemoryFramework::Allocator_TextureMemory )

	// This contains render targets and depth stencil targets, nothing more. Should be filled up to full, as the amount of render targets we use is constant.
	DECLARE_MEMORY_POOL( GpuMemoryPool_RenderTargets, ::Red::MemoryFramework::GpuAllocatorThreadSafe, 0 )

	// This contains just the streamed env probe textures
	DECLARE_MEMORY_POOL( GpuMemoryPool_EnvProbes, ::Red::MemoryFramework::GpuAllocatorThreadSafe, 0 )

	// This contains data that is kept resident after they have been uploaded (i.e. dynamic textures written via cpu)
	DECLARE_MEMORY_POOL( GpuMemoryPool_RenderData, ::Red::MemoryFramework::GpuAllocatorThreadSafe, 0 )			

	// In-place cooked data is loaded into here
	DECLARE_MEMORY_POOL( GpuMemoryPool_InPlaceRenderData, ::Red::MemoryFramework::GpuAllocatorThreadSafe, Red::MemoryFramework::Allocator_NoBreakOnOOM )

	DECLARE_MEMORY_POOL( GpuMemoryPool_InPlaceDefragTemp, ::Red::MemoryFramework::GpuAllocatorThreadSafe, 0 )

	// Alias the specific pools into unified ones
	#define GpuMemoryPool_DisplayBuffer_Onion	GpuMemoryPool_DefaultOnion
	#define GpuMemoryPool_DisplayBuffer_Garlic	GpuMemoryPool_DefaultGarlic
	#define GpuMemoryPool_Shaders				GpuMemoryPool_DefaultGarlic	
	#define GpuMemoryPool_Misc					GpuMemoryPool_DefaultOnion
	#define GpuMemoryPool_Textures				GpuMemoryPool_RenderData
	#define GpuMemoryPool_Buffers				GpuMemoryPool_RenderData
	#define GpuMemoryPool_BuffersDynamic		GpuMemoryPool_DefaultOnion
	#define GpuMemoryPool_LockedBufferData		GpuMemoryPool_DefaultGarlic
	#define GpuMemoryPool_Device				GpuMemoryPool_DefaultOnion

#else

	// This contains texture data that is kept resident after they have been uploaded (i.e. dynamic textures written via cpu)
	DECLARE_MEMORY_POOL( GpuMemoryPool_Textures, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, 0 )

	// This contains all vertex/index buffers used by the GPU
	DECLARE_MEMORY_POOL( GpuMemoryPool_Buffers, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, 0 )

	// This needs to be big enough to contain the largest mesh vb that we will write to 
	DECLARE_MEMORY_POOL( GpuMemoryPool_LockedBufferData, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, 0 )

	// This needs to be big enough to contain the largest mesh vb that we will write to 
	DECLARE_MEMORY_POOL( GpuMemoryPool_ShaderInclude, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, 0 )

	// This needs to be big enough to hold SDeviceData
	DECLARE_MEMORY_POOL( GpuMemoryPool_Device, ::Red::MemoryFramework::TLSFAllocatorThreadSafe, 0 )

	// In-place cooked data is loaded into here
	DECLARE_MEMORY_POOL( GpuMemoryPool_InPlaceRenderData, ::Red::MemoryFramework::GpuAllocatorThreadSafe, 0 )

	#define GpuMemoryPool_EnvProbes GpuMemoryPool_InPlaceRenderData
#endif

#else

#error DECLARE_MEMORY_POOL must be defined to include gpuApiMemoryPools.h

#endif
