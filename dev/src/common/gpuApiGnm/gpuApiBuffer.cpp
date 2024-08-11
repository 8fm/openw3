/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"

namespace GpuApi
{

	// ----------------------------------------------------------------------

	// external member function

	Int8* SBufferData::GetMemoryPtr() const
	{
		if ( m_Desc.category == BCC_Constant )
		{
			return cbuf.m_mappedBuffer;
		}
		else if (m_Desc.usage == BUT_Dynamic)
		{
				// find the most recent buffer!
				return (Int8*)m_memoryRegion.GetRawPtr() + vsharp.m_latestHalf * m_Desc.size;
		}
		else
		{
			return (Int8*)m_memoryRegion.GetRawPtr();
		}
	}


	// ----------------------------------------------------------------------

	Int8* AllocateConstantBuffer( Uint32 bufferSize );
	Red::MemoryFramework::MemoryRegionHandle AllocateBuffer( eBufferChunkCategory category, Uint32 bufferSize, Bool isShortLived );
	
	void DeallocateConstantBuffer( Int8* buffer );
	void DeallocateBuffer( eBufferChunkCategory category, Red::MemoryFramework::MemoryRegionHandle memoryRegion );


	// ----------------------------------------------------------------------

	//inline ID3D11Buffer* GetD3DBuffer( const BufferRef &ref )
	//{
	//	GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Buffers.Data(ref).m_pBufferResource) );
	//	return ref ? GetDeviceData().m_Buffers.Data(ref).m_pBufferResource : NULL;
	//}


	//inline ID3D11Buffer* GetD3DConstantBuffer( const BufferRef &ref )
	//{
	//	GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Buffers.Data(ref).m_pBufferResource) );
	//	return ref ? GetDeviceData().m_Buffers.Data(ref).m_pBufferResource : NULL;
	//}

	// ----------------------------------------------------------------------

	BufferDesc::BufferDesc ()
		: size( 0 )
		, category( BCC_Vertex )
		, usage( BUT_Default )
		, accessFlags( 0 )
	{}

	// ----------------------------------------------------------------------

	void AddRef( const BufferRef &buffer )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Buffers.IsInUse(buffer) );
		GetDeviceData().m_Buffers.IncRefCount( buffer );
	}

	Int32 Release( const BufferRef &buffer )
	{
		GPUAPI_ASSERT( buffer );

		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Buffers.IsInUse(buffer) );
		GPUAPI_ASSERT( dd.m_Buffers.GetRefCount(buffer) >= 1 );
		Int32 refCount = dd.m_Buffers.DecRefCount( buffer );
		if ( 0 == refCount )
		{
			QueueForDestroy(buffer);
		}
		return refCount;
	}

	void Destroy(const BufferRef& buffer)
	{
		SDeviceData &dd = GetDeviceData();
		SBufferData &data = dd.m_Buffers.Data( buffer );

#ifndef RED_FINAL_BUILD
		if ( data.m_Desc.usage == BUT_ImmutableInPlace )
		{
			dd.m_MeshStats.RemoveBuffer( data.m_Desc.size, data.m_Desc.category );
		}
#endif

		if ( data.m_memoryRegion.IsValid() )
		{
			if ( data.m_Desc.usage == BUT_ImmutableInPlace )
			{
				ReleaseInPlaceMemoryRegion( INPLACE_Buffer, data.m_memoryRegion );
			}
			else
			{
				DeallocateBuffer( data.m_Desc.category, data.m_memoryRegion );
			}
			data.m_memoryRegion = nullptr;
		}
		else if ( data.m_Desc.category == BCC_Constant )
		{
			DeallocateConstantBuffer( data.cbuf.m_mappedBuffer );
			data.cbuf.m_mappedBuffer = nullptr;
		}

		// Destroy shit
		dd.m_Buffers.Destroy( buffer );
	}

	void GetBufferDesc( const BufferRef &ref, BufferDesc &outDesc )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Buffers.IsInUse(ref) );
		outDesc = GetDeviceData().m_Buffers.Data(ref).m_Desc;
	}

	const BufferDesc& GetBufferDesc( const BufferRef &ref )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Buffers.IsInUse(ref) );
		return GetDeviceData().m_Buffers.Data(ref).m_Desc;
	}

	const MeshStats *GetMeshStats()
	{
		return &GpuApi::GetDeviceData().m_MeshStats;
	}

	void BindVertexBuffers( Uint32 startIndex, Uint32 count, const BufferRef* buffers, const Uint32* strides, const Uint32* offsets )
	{
		GPUAPI_ASSERT( IsInit() );
		GPUAPI_ASSERT( startIndex < GPUAPI_VERTEX_LAYOUT_MAX_SLOTS && startIndex + count <= GPUAPI_VERTEX_LAYOUT_MAX_SLOTS, TXT("Binding vertex buffers out of bounds") );
		GPUAPI_ASSERT( buffers != nullptr && strides != nullptr && offsets != nullptr, TXT("All data must be provided") );

		SDeviceData& dd = GetDeviceData();

		// Cache garlic memory buffers prior to further processing
		Uint32 index = startIndex;
		for ( Uint32 b=0; b<count; ++b, ++index )
		{
			// Shadow state
			dd.m_VertexBuffers[ index ] = buffers[ b ];
			dd.m_VertexBufferOffsets[ index ] = offsets[ b ];
			dd.m_VertexBufferStrides[ index ] = strides[ b ];
		}
		dd.m_vbChanged = true;
		
		// Setting the buffers to GNM is done just before the draw call because we don't know the proper vertex format yet
		// See SetVertexBuffersInternal()
	}

	void BindNullVertexBuffers()
	{
		GPUAPI_ASSERT( IsInit() );

		SDeviceData& dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		// Unbind all slots
		gfxc.setVertexBuffers( sce::Gnm::kShaderStageVs, 0, sce::Gnm::kSlotCountVertexBuffer, NULL );
		Red::System::MemoryZero( dd.m_VertexBuffers, GPUAPI_VERTEX_LAYOUT_MAX_SLOTS * sizeof( BufferRef ) );
		Red::System::MemoryZero( dd.m_VertexBufferOffsets, sizeof( dd.m_VertexBufferOffsets ) );
		Red::System::MemoryZero( dd.m_VertexBufferStrides, sizeof( dd.m_VertexBufferStrides ) );

		dd.m_vbChanged = true;
	}


	void BindIndexBuffer( const BufferRef &buffer, Uint32 offset )
	{
		GPUAPI_ASSERT( IsInit(), TXT("GpuApi not ititialized") );
		SDeviceData &dd = GetDeviceData();

		// No GNM code here, index buffer pointer is required at draw call

		if ( buffer )
		{
			const SBufferData &data = dd.m_Buffers.Data( buffer );
			if ( data.m_Desc.category != BCC_Index16Bit && data.m_Desc.category != BCC_Index16BitUAV && data.m_Desc.category != BCC_Index32Bit )
			{
				GPUAPI_HALT( "Binding non-index buffer to index buffer pipeline!" );
				return;
			}
		}

		dd.m_IndexBuffer = buffer;
		dd.m_IndexBufferOffset = offset;
	}

	void BindIndirectArgs( const BufferRef &buffer )
	{
		SDeviceData &dd = GetDeviceData();

		const SBufferData &data = dd.m_Buffers.Data( buffer );
		if ( data.m_Desc.category != BCC_IndirectUAV )
		{
			GPUAPI_HALT( "Binding non-indirect-args buffer to indirect draw pipeline!" );
			return;
		}

		dd.m_indirectArgs = buffer;
	}

	void BindBufferSRV( const BufferRef &buffer, Uint32 slot, eShaderType shaderStage )
	{
		SDeviceData &dd = GetDeviceData();

		if (slot >= MAX_PS_SAMPLERS)
		{
			GPUAPI_HALT("BindBufferSRV slot %d is greater than MAX_PS_SAMPLERS!", slot);
			return;
		}

		dd.m_bufferSRVsSet[shaderStage][slot] = buffer;

		dd.m_vbChanged = true;
	}

	void BindBufferUAV( const BufferRef &buffer, Uint32 slot )
	{
		SDeviceData &dd = GetDeviceData();

		if (!buffer.isNull())
		{
			SBufferData& bufData = dd.m_Buffers.Data( buffer );
			sce::Gnm::Buffer tempBuffer;

			if (bufData.m_Desc.category == BCC_Raw)
			{
				tempBuffer.initAsByteBuffer( (void *)bufData.GetMemoryPtr(), bufData.m_Desc.size );
			}
			else if (bufData.m_Desc.category == BCC_Constant )
			{
				if ( bufData.cbuf.m_discarded || bufData.m_frameDiscarded < GpuApi::FrameIndex() )
				{
					bufData.cbuf.m_submitBuffer = static_cast< GpuApi::Int8* >( dd.m_constantBufferMem.Allocate(bufData.m_Desc.size, sce::Gnm::kEmbeddedDataAlignment8) );
					GPUAPI_ASSERT( bufData.cbuf.m_submitBuffer != nullptr );
					Red::MemoryCopy(bufData.cbuf.m_submitBuffer, bufData.GetMemoryPtr(), bufData.m_Desc.size);
					bufData.cbuf.m_discarded = false;
					bufData.m_frameDiscarded = GpuApi::FrameIndex();
				}

				GpuApi::Uint32 elemSize = 4;
				GpuApi::Uint32 elemCount = bufData.m_Desc.size / elemSize;
				GpuApi::Uint32 stride = bufData.m_Desc.size / elemCount;
				
				tempBuffer.initAsRegularBuffer((void *)bufData.cbuf.m_submitBuffer, stride, elemCount);
			}
			else if(bufData.m_Desc.category == BCC_IndirectUAV)
			{
				tempBuffer.initAsDataBuffer((void *)bufData.GetMemoryPtr(), sce::Gnm::kDataFormatR32Uint, bufData.vsharp.m_elementCount);
			}
			else if(bufData.m_Desc.category == BCC_Index16BitUAV)
			{
				Uint32 elemCount = bufData.m_Desc.size / 2;
				tempBuffer.initAsDataBuffer((void *)bufData.GetMemoryPtr(), sce::Gnm::kDataFormatR16Uint, elemCount);
			}
			else
			{
				Uint32 stride = bufData.m_Desc.size / bufData.vsharp.m_elementCount;
				tempBuffer.initAsRegularBuffer((void *)bufData.GetMemoryPtr(), stride, bufData.vsharp.m_elementCount);
			}
			tempBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
			//gfxc.setRwBuffers( sce::Gnm::kShaderStageCs, slot, 1, &tempBuffer );
			dd.m_bufferUAVsSet[slot] = tempBuffer;
		}
		else
		{
			//gfxc.setRwBuffers( sce::Gnm::kShaderStageCs, slot, 1, nullptr );
			dd.m_bufferUAVsSet[slot].initAsByteBuffer(nullptr, 0);
		}

		//GetDeviceContext()->CSSetUnorderedAccessViews( slot, 1, &uav, 0 );
	}

	void ClearBufferUAV_Uint( const BufferRef &buffer, const Uint32 values[4] )
	{
		GPUAPI_HALT("NOT SUPPORTED AT GPUAPI LEVEL - USE RENDERER::ClearBufferUAV_Uint()");
	}

	void BindStreamOutBuffers( Uint32 count, const BufferRef* buffers, const Uint32* offsets )
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT(count < GPUAPI_STREAMOUT_MAX_SLOTS);

		for (Uint32 i = 0; i<GPUAPI_STREAMOUT_MAX_SLOTS; ++i)
		{
			if (i < count && buffers != nullptr )
			{
				GPUAPI_ASSERT ((dd.m_Buffers.Data(buffers[i]).m_Desc.usage & GpuApi::BUT_StreamOut) != 0);
				dd.m_StreamOutBuffers[i] = buffers[i];
				dd.m_StreamOutOffsets[i] = offsets[i];
			}
			else
			{
				dd.m_StreamOutBuffers[i] = BufferRef::Null();
				dd.m_StreamOutOffsets[i] = 0;
			}
		}

		dd.m_vbChanged = true;
	}


	BufferRef CreateStagingCopyBuffer( BufferRef originalBuffer )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		return BufferRef::Null();
	}

	Int8* AllocateConstantBuffer( Uint32 bufferSize )
	{
		// Allocate the constant buffer memory
		return static_cast<Int8*>( GPU_API_ALLOCATE( GpuMemoryPool_ConstantBuffers, MC_ConstantBuffer, bufferSize, sce::Gnm::kAlignmentOfBufferInBytes ) );
	}

	Red::MemoryFramework::MemoryRegionHandle AllocateBuffer( eBufferChunkCategory category, Uint32 bufferSize, Bool isShortLived )
	{
		Red::MemoryFramework::MemoryRegionHandle handle;
		switch ( category )
		{
		case BCC_Vertex:
		case BCC_VertexSRV:
			{
				// Allocate the vertex buffer memory
				handle = GPU_API_ALLOCATE_REGION( GpuMemoryPool_Buffers, MC_VertexBuffer, bufferSize, sce::Gnm::kAlignmentOfBufferInBytes, Red::MemoryFramework::Region_Longlived );
			}
			break;
		case BCC_Index16Bit:
		case BCC_Index16BitUAV:
		case BCC_Index32Bit:
			{
				// Allocate the index buffer memory
				handle = GPU_API_ALLOCATE_REGION( GpuMemoryPool_Buffers, MC_IndexBuffer, bufferSize, sce::Gnm::kAlignmentOfBufferInBytes, Red::MemoryFramework::Region_Longlived );
			}
			break;
		case BCC_Raw:
		case BCC_Structured:
		case BCC_StructuredUAV:
			{
				// Allocate the raw or structured buffer memory
				handle = GPU_API_ALLOCATE_REGION( GpuMemoryPool_Buffers, MC_BufferObject, bufferSize, sce::Gnm::kAlignmentOfBufferInBytes, Red::MemoryFramework::Region_Longlived );
			}
			break;
		case BCC_IndirectUAV:
			{
				// Allocate the raw or structured buffer memory
				handle = GPU_API_ALLOCATE_REGION( GpuMemoryPool_Buffers, MC_BufferObject, bufferSize, sce::Gnm::kAlignmentOfIndirectArgsInBytes, Red::MemoryFramework::Region_Longlived );
			}
			break;
		default:	GPUAPI_HALT( "invalid buffer category" );	// This includes constant buffers, as they shouldn't be allocated through a gpu allocator
		}

		GPU_API_UNLOCK_REGION( GpuMemoryPool_Buffers, handle );	// These never move
		return handle;
	}

	void DeallocateConstantBuffer( Int8* buffer )
	{
		GPU_API_FREE( GpuMemoryPool_ConstantBuffers, MC_ConstantBuffer, buffer );
	}

	void DeallocateBuffer( eBufferChunkCategory category, Red::MemoryFramework::MemoryRegionHandle memoryRegion )
	{
		switch ( category )
		{
		case BCC_Vertex:
		case BCC_VertexSRV:
			{
				GPU_API_FREE_REGION( GpuMemoryPool_Buffers, memoryRegion );
			}
			break;
		case BCC_Index16Bit:
		case BCC_Index16BitUAV:
		case BCC_Index32Bit:
			{
				GPU_API_FREE_REGION( GpuMemoryPool_Buffers, memoryRegion );
			}
			break;
		case BCC_Raw:
		case BCC_Structured:
		case BCC_StructuredUAV:
		case BCC_IndirectUAV:
			{
				GPU_API_FREE_REGION( GpuMemoryPool_Buffers, memoryRegion );
			}
			break;
		default:	GPUAPI_HALT( "invalid buffer category" );	// This includes constant buffers, as they shouldn't be allocated through a gpu allocator
		}
	}

	BufferRef CreateBuffer( Uint32 bufferSize, eBufferChunkCategory category, eBufferUsageType usage, Uint32 accessFlags, const BufferInitData* initData /*= nullptr*/ )
	{
		SDeviceData &dd = GetDeviceData();

		// Test input conditions
		if ( !IsInit() )
		{
			GPUAPI_HALT(  "Not init during attempt to create buffer" );
			return BufferRef::Null();
		}

		if ( bufferSize < 1 )
		{
			GPUAPI_LOG_WARNING( TXT( "Invalid buffer size" ) );
			return BufferRef::Null();
		}

		if ( !dd.m_Buffers.IsCapableToCreate( 1 ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Failed to create buffer." ) );
			return BufferRef::Null();
		}

		// consistency checks to make sure that we do the same as the D3D implementation would do

		// D3D doesn't seem to like BIND_STREAM_OUT combined with BIND_UNORDERED_ACCESS...
		if ( ( category == BCC_Raw || category == BCC_Structured || category == BCC_StructuredUAV ) && usage == BUT_StreamOut )
		{
			GPUAPI_LOG_WARNING( TXT( "In D3D Raw/Structured buffers cannot have StreamOut usage, we are keeping the compatibility here" ) );
			return BufferRef::Null();
		}

		// D3D doesn't seem to like Unordered Access Views on dynamic buffers
		if ( category == BCC_StructuredUAV && usage == BUT_Dynamic )
		{
			GPUAPI_LOG_WARNING( TXT( "StructuredUAV buffers cannot have Dynamic usage." ) );
			return BufferRef::Null();
		}

		// Create GpuApi buffer
		Uint32 newBufferId = dd.m_Buffers.Create( 1 );
		if ( !newBufferId )
		{
			GPUAPI_HALT(  "Failed to create gpuapi buffer despite it was tested as possible" );
			return BufferRef::Null();
		}
		
		// Initialize new buffer
		SBufferData &bufferData = dd.m_Buffers.Data( newBufferId );
		bufferData.m_Desc.category = category;
		bufferData.m_Desc.size = bufferSize;
		bufferData.m_Desc.usage = usage;
		bufferData.m_Desc.accessFlags = accessFlags;

		bufferData.vsharp.m_latestHalf = 0;
		bufferData.vsharp.m_discardOnBatch = 1<<16; // just make it big
		bufferData.m_frameDiscarded = 0;

		if ( usage == BUT_ImmutableInPlace )
		{
			// For in-place created, just take ownership of the memory
			GPUAPI_ASSERT( initData && initData->m_memRegionHandle.IsValid(), TXT("Missing init data for an in place buffer!") );
			bufferData.m_memoryRegion = initData->m_memRegionHandle;
		}
		else
		{
			if ( category == BCC_Constant )
			{
				// Constant buffer goes to Onion, as opposed to any other buffer type. 
				bufferData.cbuf.m_mappedBuffer = AllocateConstantBuffer( bufferSize );
				bufferData.cbuf.m_submitBuffer = nullptr;
				bufferData.cbuf.m_discarded = false;

				if ( initData && initData->m_buffer )
				{
					Red::System::MemoryCopy( bufferData.cbuf.m_mappedBuffer, initData->m_buffer, bufferSize );
					bufferData.cbuf.m_discarded = true;
				}
			}
			else
			{
				// Any other non in-place buffer. Allocate a double buffer in case of dynamics.
				const Bool isDynamic = ( usage == BUT_Dynamic );
				const Uint32 allocBufferSize = isDynamic ? 2.0f * bufferSize : bufferSize;
				bufferData.m_memoryRegion = AllocateBuffer( category, allocBufferSize, isDynamic );

				if ( initData  )
				{
					bufferData.vsharp.m_elementCount = initData->m_elementCount;
					if ( initData->m_buffer )
					{
						Red::System::MemoryCopy( bufferData.m_memoryRegion.GetRawPtr(), initData->m_buffer, bufferSize );
					}
				}
			}
		}

#ifndef RED_FINAL_BUILD
		if ( usage == BUT_ImmutableInPlace )
		{
			dd.m_MeshStats.AddBuffer( bufferSize, category );
		}
#endif

		// Finalize
		GPUAPI_ASSERT( newBufferId && dd.m_Buffers.IsInUse( newBufferId ) );
		return BufferRef( newBufferId );
	}

	// GetMemoryClassForBufferType
	//	Returns a memory class for a particular buffer type
	EMemoryClass GetMemoryClassForBufferType( const BufferDesc& bufferDescription )
	{
		switch( bufferDescription.category )
		{
		case BCC_Vertex:
		case BCC_VertexSRV:
			return MC_VertexBuffer;

		case BCC_Index16Bit:
		case BCC_Index32Bit:
		case BCC_Index16BitUAV:
			return MC_IndexBuffer;

		case BCC_Constant:
			return MC_ConstantBuffer;

		case BCC_Structured:
		case BCC_StructuredUAV:
			return MC_StructuredBuffer;

		case BCC_Raw:
			return MC_RawBuffer;

		default:
			return MC_LockedBuffer;
		}
	}

	void* LockBuffer( const BufferRef &ref, Uint32 flags, Uint32 offset, Uint32 size )
	{
		if ( !ref || size < 1 )
		{
			return nullptr;
		}

		GPUAPI_ASSERT( IsInit() );

		const BufferDesc& bufferDesc = GetBufferDesc(ref);

		GPUAPI_ASSERT( ( offset + size ) <= bufferDesc.size );

		// Lock buffer
		void *resultPtr = nullptr;

		if ( bufferDesc.usage == BUT_Dynamic || bufferDesc.usage == BUT_Staging )
		{
			SBufferData& bufferData = GetDeviceData().m_Buffers.Data(ref);

			if (flags & BLF_Discard && bufferDesc.category != BCC_Constant)
			{
				// if we've already DISCARD'ed this frame AND we've issued a drawcall between which may be using the buffer data
				// then warn because this is not safe to modify
				if ( bufferData.vsharp.m_discardOnBatch < BatchIndex() && bufferData.m_frameDiscarded == FrameIndex() )
				{
					GPUAPI_FATAL("Calling discard multiple times in a frame is not supported!");
				}
				else
				{
					bufferData.vsharp.m_latestHalf = ( bufferData.vsharp.m_latestHalf + 1 ) % 2;
					bufferData.vsharp.m_discardOnBatch = BatchIndex();
					bufferData.m_frameDiscarded = FrameIndex();
				}
			}

			if ( bufferDesc.category == BCC_Constant )
			{
				bufferData.cbuf.m_discarded = true;
			}

			resultPtr = bufferData.GetMemoryPtr() + offset;
		}
		else
		{
			GPUAPI_HALT( "LOCKING THE NON-DYNAMIC BUFFER!!!" );
#if 0	// This path is never used and let's keep it this way!
			if ( flags & BLF_Read )
			{
				GPUAPI_HALT( "can't lock non readable buffer for read" );
				return nullptr;
			}

			GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer = GPU_API_ALLOCATE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( bufferDesc ), size, 16 );
			GetDeviceData().m_Buffers.Data(ref).m_lockedSize = size;
			GetDeviceData().m_Buffers.Data(ref).m_lockedOffset = offset;
			return GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer;
#endif
		}

		// Finish
		return resultPtr;
	}

	void* LockBufferAsync( const BufferRef &ref, Uint32 flags, Uint32 offset, Uint32 size, void* deferredContext )
	{
		if ( !ref || size < 1 )
		{
			return nullptr;
		}

		GPUAPI_ASSERT( IsInit() );
		GPUAPI_ASSERT( ( offset + size ) <= GetBufferDesc(ref).size );

		// Lock buffer
		void *resultPtr = nullptr;

		GPUAPI_HALT("NOT IMPLEMENTED");

		// Finish
		return resultPtr;
	}

	void UnlockBuffer( const BufferRef &ref )
	{
		GPUAPI_ASSERT( ref );

		eBufferUsageType usage = GetDeviceData().m_Buffers.Data(ref).m_Desc.usage;

		if ( usage == BUT_Dynamic || usage == BUT_Staging )
		{
			// Seems like nothing to do here really. It was just a pointer returned when locking. 
		}
		else
		{
			GPUAPI_HALT( "UNLOCKING THE NON-DYNAMIC BUFFER!!!" );
		}
	}


	void LoadBufferData( const GpuApi::BufferRef &destBuffer, Uint32 offset, Uint32 size, const void* srcMemory )
	{
		GPUAPI_FATAL( "NOT IMPLEMENTED!!! Use CRenderInterface::LoadBufferData" );
	}


	void CopyBuffer( const BufferRef &dest, Uint32 destOffset, const BufferRef &source, Uint32 sourceOffset, Uint32 length )
	{
		SDeviceData &dd = GetDeviceData();

		GPUAPI_ASSERT( !dest.isNull() && !source.isNull() );
		GPUAPI_ASSERT( dd.m_Buffers.IsInUse( dest ) && dd.m_Buffers.IsInUse( source ) );

		SBufferData &destData = dd.m_Buffers.Data( dest );
		SBufferData &srcData = dd.m_Buffers.Data( source );

		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		// ensure that the GPU has finished producing the data before issuing DMA (which happens async with GPU)
		volatile uint32_t* label = static_cast<uint32_t*>(gfxc.allocateFromCommandBuffer(sizeof(uint32_t), sce::Gnm::kEmbeddedDataAlignment8));
		*label = 0;
		gfxc.writeImmediateDwordAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches, const_cast<uint32_t*>(label), 1, sce::Gnm::kCacheActionNone);
		gfxc.waitOnAddress(const_cast<uint32_t*>(label), 0xFFFFFFFF, sce::Gnm::kWaitCompareFuncEqual, 1);

		// issue DMA
		gfxc.copyData(destData.GetMemoryPtr() + destOffset, srcData.GetMemoryPtr() + sourceOffset, length, sce::Gnm::kDmaDataBlockingEnable);
	}

	void UnlockBufferAsync( const BufferRef &ref, void* deferredContext )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");
	}

	void SetBufferDebugPath( const BufferRef &ref, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_Buffers.IsInUse(ref) );
		if ( !ref )
		{
			return;
		}
		SBufferData &data = GetDeviceData().m_Buffers.Data(ref);
		Red::System::StringCopy( data.m_debugPath, debugPath, ARRAY_COUNT(data.m_debugPath) );
#endif
	}

	void BindConstantBuffer( Uint32 slot, BufferRef buffer, eShaderType shaderStage /*= PixelShader*/ )
	{
		GPUAPI_ASSERT( IsInit() );
		SDeviceData &dd = GetDeviceData();

		if (slot < MAX_CONSTANT_BUFFERS)
		{
			switch ( shaderStage )
			{
			case VertexShader:		//falldown
			case GeometryShader:	//falldown
			case HullShader:		//falldown
			case DomainShader:		//falldown
				// Emulate previous functionality although setting Es or Ls is not really supported...
				dd.m_ConstantBuffersSet[VertexShader][slot] = buffer;
				dd.m_ConstantBuffersSet[GeometryShader][slot] = buffer;
				dd.m_ConstantBuffersSet[HullShader][slot] = buffer;
				dd.m_ConstantBuffersSet[DomainShader][slot] = buffer;
				break;

			default:
				dd.m_ConstantBuffersSet[shaderStage][slot] = buffer;
				break;
			}
		}
		else
			GPUAPI_HALT ("CB%d out of range!", slot);

#if 0
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		sce::Gnm::Buffer* constantBufferPtr = nullptr;
		sce::Gnm::Buffer constantBuffer;
		if ( !buffer.isNull() )
		{
			const SBufferData& bufferData = dd.m_Buffers.Data( buffer );

            if (bufferData.m_Desc.usage == BUT_Dynamic)
            {
                void* mem = gfxc.allocateFromCommandBuffer(bufferData.m_Desc.size, sce::Gnm::kEmbeddedDataAlignment8);
                Red::MemoryCopy(mem, bufferData.GetMemoryPtr(), bufferData.m_Desc.size);
                constantBuffer.initAsConstantBuffer(mem, bufferData.m_Desc.size);
            }
            else
            {
                constantBuffer.initAsConstantBuffer( bufferData.GetMemoryPtr(), bufferData.m_Desc.size );
            }

			constantBufferPtr = &constantBuffer;
		}
		
		switch ( shaderStage )
		{
		case VertexShader:		//falldown
		case GeometryShader:	//falldown
		case HullShader:		//falldown
		case DomainShader:		//falldown
			gfxc.setConstantBuffers( sce::Gnm::kShaderStageLs, slot, 1, constantBufferPtr );
			gfxc.setConstantBuffers( sce::Gnm::kShaderStageVs, slot, 1, constantBufferPtr );
			gfxc.setConstantBuffers( sce::Gnm::kShaderStageEs, slot, 1, constantBufferPtr );
			gfxc.setConstantBuffers( sce::Gnm::kShaderStageHs, slot, 1, constantBufferPtr );
			gfxc.setConstantBuffers( sce::Gnm::kShaderStageGs, slot, 1, constantBufferPtr );
			break;
		case PixelShader:
			gfxc.setConstantBuffers( sce::Gnm::kShaderStagePs, slot, 1, constantBufferPtr );
			break;
		case ComputeShader:
			gfxc.setConstantBuffers( sce::Gnm::kShaderStageCs, slot, 1, constantBufferPtr );
			break;
		default:
			GPUAPI_HALT( "Unknown shader stage!" );
			break;
		}
#endif

	}
}
