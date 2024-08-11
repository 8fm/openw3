/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"

namespace GpuApi
{	

	// ----------------------------------------------------------------------

	inline ID3D11Buffer* GetD3DBuffer( const BufferRef &ref )
	{
		GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Buffers.Data(ref).m_pBufferResource) );
		return ref ? GetDeviceData().m_Buffers.Data(ref).m_pBufferResource : nullptr;
	}


	inline ID3D11Buffer* GetD3DConstantBuffer( const BufferRef &ref )
	{
		GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Buffers.Data(ref).m_pBufferResource) );
		return ref ? GetDeviceData().m_Buffers.Data(ref).m_pBufferResource : nullptr;
	}

	// ----------------------------------------------------------------------

	BufferDesc::BufferDesc ()
		: category( BCC_Vertex )
		, size( 0 )
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
			SBufferData& data = dd.m_Buffers.Data( buffer );
			if ( data.m_memoryRegion.IsValid() )
			{
				// If we have an in-place memory region, need to queue the buffer for later destruction.
				// Otherwise, maybe the memory is still involved in a GPU operation like defrag copy, and
				// we need to be sure that's finished before we actually release the memory back to the
				// memory system.
				QueueForDestroy( buffer );
			}
			else
			{
				// Can safely just destroy it.
				Destroy( buffer );
			}
		}
		return refCount;
	}

	void Destroy( const BufferRef &buffer )
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Buffers.IsInUse(buffer) );

		SBufferData &data = dd.m_Buffers.Data( buffer );

#ifndef RED_FINAL_BUILD
		if ( data.m_Desc.usage == BUT_ImmutableInPlace )
		{
			dd.m_MeshStats.RemoveBuffer( data.m_Desc.size, data.m_Desc.category );
		}
#endif

		// Release resources
		GPUAPI_ASSERT( nullptr != data.m_pBufferResource );
		SAFE_RELEASE( data.m_pBufferResource );

		SAFE_RELEASE( data.m_pUnorderedAccessView );
		SAFE_RELEASE( data.m_pShaderResourceView );

#ifdef RED_PLATFORM_DURANGO
		if ( data.m_memoryRegion.IsValid() )
		{
			ReleaseInPlaceMemoryRegion( INPLACE_Buffer, data.m_memoryRegion );
		}
#endif

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
		GPUAPI_ASSERT( startIndex < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT && startIndex + count <= D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, TXT("Binding vertex buffers out of bounds") );
		GPUAPI_ASSERT( buffers != nullptr && strides != nullptr && offsets != nullptr, TXT("All data must be provided") );

#ifdef RED_PLATFORM_WINPC
		ID3D11Buffer* d3dBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		for ( Uint32 i = 0; i < count; ++i )
		{
			GPUAPI_ASSERT( !buffers[i] || strides[i] > 0, TXT("Vertex stride cannot be 0") );
			d3dBuffers[i] = GetD3DBuffer( buffers[i] );
		}

		GetDeviceContext()->IASetVertexBuffers( startIndex, count, d3dBuffers, strides, offsets );
#else
		SDeviceData &dd = GetDeviceData();
		ID3D11DeviceContextX* contextX = (ID3D11DeviceContextX*)GetDeviceContext();
		for ( Uint32 i = 0; i < count; ++i )
		{
			Uint32 slot = startIndex + i;
			ID3D11Buffer* buffer = GetD3DBuffer( buffers[i] );
			if ( !buffers[i].isNull())
			{
				const SBufferData &data = dd.m_Buffers.Data(buffers[i]);
				const Red::MemoryFramework::MemoryRegionHandle& memRegion = data.m_memoryRegion;
				if ( memRegion.GetRegionInternal() == nullptr )
				{
					GetDeviceContext()->IASetVertexBuffers( slot, 1, &buffer, &strides[i], &offsets[i] );
					dd.m_needsUnbind[slot] = true;
				}
				else
				{
					if (dd.m_needsUnbind[slot])
					{
						ID3D11Buffer* nullBuffer = nullptr;
						GetDeviceContext()->IASetVertexBuffers( slot, 1, &nullBuffer, &strides[i], &offsets[i] );
					}
					contextX->IASetPlacementVertexBuffer( slot, buffer, ((Uint8*)memRegion.GetRawPtr()) + offsets[i], strides[i] );
					dd.m_needsUnbind[slot] = false;
				}
			}
			else
			{
				ID3D11Buffer* nullBuffer = nullptr;
				GetDeviceContext()->IASetVertexBuffers( slot, 1, &nullBuffer, &strides[i], &offsets[i] );
			}
		}
#endif
	}

	void BindNullVertexBuffers()
	{
		GPUAPI_ASSERT( IsInit() );
		ID3D11Buffer* nullBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {}; // init to nullptrs
		Uint32 nullStrideOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {}; // init to 0s
		GetDeviceContext()->IASetVertexBuffers( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, &nullBuffers[0], nullStrideOffsets, nullStrideOffsets );
	}

	void BindIndexBuffer( const BufferRef &buffer, Uint32 offset )
	{
		// ace_optimize!!! dodac filtrowanie stanow, jak juz wszedzie zintegruje shit

		SDeviceData &dd = GetDeviceData();
		if ( buffer )
		{
			GPUAPI_ASSERT( IsInit() );		
			const SBufferData &data = dd.m_Buffers.Data(buffer);

			//HACK
			DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
			switch (data.m_Desc.category)
			{
			case BCC_Index16Bit:
			case BCC_Index16BitUAV:
			case BCC_Raw:
				indexFormat = DXGI_FORMAT_R16_UINT;
				break;
			case BCC_Index32Bit:
				indexFormat = DXGI_FORMAT_R32_UINT;
				break;
			default:
				GPUAPI_HALT( "" );
				return;
			}

#ifdef RED_PLATFORM_WINPC
			GetDeviceContext()->IASetIndexBuffer( GetD3DBuffer( buffer ), indexFormat, offset );
#else
			ID3D11DeviceContextX* contextX = (ID3D11DeviceContextX*)GetDeviceContext();
			if (data.m_memoryRegion.GetRegionInternal())
			{
				if (!dd.m_IndexBuffer.isNull() && dd.m_Buffers.Data(dd.m_IndexBuffer).m_memoryRegion.GetRegionInternal() == nullptr)
				{
					//if we had an index buffer bound without placement binding then we have to unbind it to have proper internal refcounting
					GetDeviceContext()->IASetIndexBuffer( nullptr, DXGI_FORMAT_R16_UINT, 0 );
				}

				contextX->IASetPlacementIndexBuffer( data.m_pBufferResource, ((Uint8*)data.m_memoryRegion.GetRawPtr()) + offset, indexFormat );
			}
			else
			{
				GetDeviceContext()->IASetIndexBuffer( GetD3DBuffer( buffer ), indexFormat, offset );
			}
#endif
			dd.m_IndexBuffer = buffer;
		}
		else
		{
			GPUAPI_ASSERT( IsInit() );
			GetDeviceContext()->IASetIndexBuffer( nullptr, DXGI_FORMAT_R16_UINT, 0 );
			dd.m_IndexBuffer = BufferRef::Null();
		}
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
		ID3D11ShaderResourceView* texture = nullptr;

		if (!buffer.isNull())
		{
			texture = (GetDeviceData().m_Buffers.Data(buffer).m_pShaderResourceView);
		}

		switch (shaderStage)
		{
		case PixelShader:
			{
				GetDeviceContext()->PSSetShaderResources( slot, 1, &texture );
				break;
			}
		case VertexShader:
			{
				GetDeviceContext()->VSSetShaderResources( slot, 1, &texture );
				break;
			}
		case GeometryShader:
			{
				GetDeviceContext()->GSSetShaderResources( slot, 1, &texture );
				break;
			}
		case HullShader:
			{
				GetDeviceContext()->HSSetShaderResources( slot, 1, &texture );
				break;
			}
		case DomainShader:
			{
				GetDeviceContext()->DSSetShaderResources( slot, 1, &texture );
				break;
			}
		case ComputeShader:
			{
				GetDeviceContext()->CSSetShaderResources( slot, 1, &texture );
				break;
			}
		}
	}

	void BindBufferUAV( const BufferRef &buffer, Uint32 slot )
	{
		GPUAPI_ASSERT( !(buffer && !GetDeviceData().m_Buffers.Data(buffer).m_pUnorderedAccessView ) );

		ID3D11UnorderedAccessView* uav = nullptr;

		if (!buffer.isNull())
		{
			uav = GetDeviceData().m_Buffers.Data(buffer).m_pUnorderedAccessView;
		}

		GetDeviceContext()->CSSetUnorderedAccessViews( slot, 1, &uav, 0 );
	}

	void ClearBufferUAV_Uint( const BufferRef &buffer, const Uint32 values[4] )
	{
		GPUAPI_ASSERT( !(buffer && !GetDeviceData().m_Buffers.Data(buffer).m_pUnorderedAccessView ) );

		ID3D11UnorderedAccessView *uav = nullptr;

		if (!buffer.isNull())
		{
			uav = GetDeviceData().m_Buffers.Data(buffer).m_pUnorderedAccessView;			
		}

		if ( nullptr != uav )
		{
			GetDeviceContext()->ClearUnorderedAccessViewUint( uav, values );
		}
	}

	void BindStreamOutBuffers( Uint32 count, const BufferRef* buffers, const Uint32* offsets )
	{
		// There doesn't seem to be a D3D #define for max number of SO targets, but docs say it's 4.
		GPUAPI_ASSERT( count <= 4 );
		if ( count > 4 )
		{
			count = 4;
		}

		ID3D11Buffer* d3dBuffers[4] = { nullptr, nullptr, nullptr, nullptr };

		for ( Uint32 i = 0; i < count; ++i )
		{
			GPUAPI_ASSERT( !(buffers[i] && !GetDeviceData().m_Buffers.Data(buffers[i]).m_pBufferResource ) );
			if (!buffers[i].isNull())
			{
				d3dBuffers[i] = GetDeviceData().m_Buffers.Data(buffers[i]).m_pBufferResource;
			}
		}

		GetDeviceContext()->SOSetTargets( count, d3dBuffers, offsets );
	}


	BufferRef CreateStagingCopyBuffer( BufferRef originalBuffer )
	{
		SDeviceData &dd = GetDeviceData();

		BufferDesc originalDesc = GetDeviceData().m_Buffers.Data(originalBuffer).m_Desc;

		GPUAPI_ASSERT( originalDesc.usage != BUT_Dynamic && originalDesc.category != BCC_Structured, TXT("dynamic and structured buffers are not supported for staging copies") );

		ID3D11Buffer* d3dBuffer = nullptr;
		D3D11_BUFFER_DESC bd;
		bd.Usage = D3D11_USAGE_STAGING;
		bd.BindFlags = 0;
		bd.ByteWidth = originalDesc.size;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bd.MiscFlags = 0;

		HRESULT hRes = dd.m_pDevice->CreateBuffer( &bd, nullptr, &d3dBuffer);
#ifdef NO_GPU_ASSERTS
		RED_UNUSED( hRes );
#endif
		GPUAPI_ASSERT( hRes == S_OK, TXT("Can't create staging buffer") );

		GetDeviceContext()->CopySubresourceRegion( d3dBuffer, 0, 0, 0, 0, GetDeviceData().m_Buffers.Data(originalBuffer).m_pBufferResource, 0, nullptr );

		// Create GpuApi buffer
		Uint32 newBufferId = dd.m_Buffers.Create( 1 );
		if ( !newBufferId )
		{
			SAFE_RELEASE( d3dBuffer );
			GPUAPI_HALT( "Failed to create gpuapi buffer despite it was tested as possible" );
			return BufferRef::Null();
		}
#ifdef GPU_API_DEBUG_PATH
		d3dBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, 17, "stagingCopyBuffer" );
#endif

		// Initialize new buffer
		SBufferData &data = dd.m_Buffers.Data( newBufferId );
		data.m_Desc.category = BCC_Vertex;
		data.m_Desc.size = originalDesc.size;
		data.m_Desc.accessFlags = BAF_CPURead;
		data.m_Desc.usage = BUT_Staging;
		data.m_pBufferResource = d3dBuffer;

		// Finalize
		GPUAPI_ASSERT( newBufferId && dd.m_Buffers.IsInUse( newBufferId ) );
		return BufferRef( newBufferId );
	}

	BufferRef CreateBuffer( Uint32 bufferSize, eBufferChunkCategory category, eBufferUsageType usage, Uint32 accessFlags, const BufferInitData* initData /*= nullptr*/ )
	{
		SDeviceData &dd = GetDeviceData();

		// Test input conditions
		if ( !IsInit() )
		{
			GPUAPI_LOG_WARNING( TXT( "Trying to create a render buffer before GPU API is initialised" ) );
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

		// D3D doesn't seem to like BIND_STREAM_OUT combined with BIND_UNORDERED_ACCESS...
		if ( ( category == BCC_Raw || category == BCC_Structured || category == BCC_StructuredUAV ) && usage == BUT_StreamOut )
		{
			GPUAPI_LOG_WARNING( TXT( "Raw/Structured buffers cannot have StreamOut usage." ) );
			return BufferRef::Null();
		}

		// D3D doesn't seem to like Unordered Access Views on dynamic buffers
		if ( category == BCC_StructuredUAV && usage == BUT_Dynamic )
		{
			GPUAPI_LOG_WARNING( TXT( "StructuredUAV buffers cannot have Dynamic usage." ) );
			return BufferRef::Null();
		}

		// Create d3d buffer
		D3D11_BUFFER_DESC bd;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		if ( usage == BUT_Dynamic ) 
		{
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		if ( usage == BUT_Immutable || usage == BUT_ImmutableInPlace )
		{
			bd.Usage = D3D11_USAGE_IMMUTABLE;
		}

		bd.BindFlags = 0;
		if ( usage == BUT_StreamOut )
		{
			bd.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
		}

		bd.ByteWidth = bufferSize;
		bd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA subresData;
		D3D11_SUBRESOURCE_DATA* subresDataPtr = nullptr;
		Uint32 elementCount = 0;

		if ( initData )
		{
			if ( initData->m_memRegionHandle.IsValid() && usage != BUT_ImmutableInPlace )
			{
				subresData.pSysMem = initData->m_memRegionHandle.GetRawPtr();
				subresData.SysMemPitch = 0;
				subresData.SysMemSlicePitch = 0;
				subresDataPtr = &subresData;
			}
			else if ( initData->m_buffer )
			{
				subresData.pSysMem = initData->m_buffer;
				subresData.SysMemPitch = 0;
				subresData.SysMemSlicePitch = 0;
				subresDataPtr = &subresData;
			}
			elementCount = initData->m_elementCount;
		}

		const char* name = nullptr;
		Uint32 nameLength;


		HRESULT hr = S_OK;
		switch ( category )
		{
		case BCC_Vertex:
			{
				bd.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
				name = "vertexB";
				nameLength = 7;
			}
			break;
		case BCC_VertexSRV:
			{
				bd.BindFlags |= D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_SHADER_RESOURCE;
				name = "vertexUAV";
				nameLength = 9;
			}
			break;
		case BCC_Index16Bit:	// falldown
		case BCC_Index32Bit:
			{
				bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
				if (category == BCC_Index16Bit)
				{
					name = "indexB16";
					nameLength = 8;
				}
				else
				{
					name = "indexB32";
					nameLength = 8;
				}
			}
			break;
		case BCC_Index16BitUAV:
			{
				bd.BindFlags |= D3D11_BIND_INDEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
				name = "indexB16UAV";
				nameLength = 11;
			}
			break;
		case BCC_Constant:
			{
				GPUAPI_ASSERT( ( bufferSize & 0xf ) == 0, TXT("Constant buffer size must be multiple of 16") );

				bd.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
				name = "constB";
				nameLength = 6;
			}
			break;
		case BCC_Structured:
			{
				bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				bd.StructureByteStride = bufferSize / elementCount;
				name = "structB";
				nameLength = 7;
			}
			break;
		case BCC_StructuredUAV:
		case BCC_StructuredAppendUAV:
			{
				bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
				bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				bd.StructureByteStride = bufferSize / elementCount;
				name = "structUAVB";
				nameLength = 10;
			}
			break;
		case BCC_IndirectUAV:
			{
				bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
				bd.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
				bd.StructureByteStride = bufferSize / elementCount;
				name = "structUAVB";
				nameLength = 10;
			}
			break;
		case BCC_Raw:
			{
				bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;
				bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
				name = "rawB";
				nameLength = 4;
			}
			break;
		default:
			GPUAPI_HALT( "invalid buffer category" );
			return BufferRef::Null();
		}


		ID3D11Buffer *d3dBuff = nullptr;
#ifdef RED_PLATFORM_DURANGO
		// Xbox can use placement creation for BUT_ImmutableInPlace.
		if ( usage == BUT_ImmutableInPlace && initData->m_memRegionHandle.IsValid() )
		{
			GPUAPI_ASSERT( initData->m_memRegionHandle.IsValid() );
			ID3D11DeviceX* deviceX = (ID3D11DeviceX*)GetDevice();
			hr = deviceX->CreatePlacementBuffer( &bd, nullptr, &d3dBuff );
		}
		else
#endif
		{
			hr = GetDevice()->CreateBuffer( &bd, subresDataPtr, &d3dBuff );
		}

		if ( FAILED( hr ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Failed to create d3d buffer. HRESULT: %d" ), hr );
			return BufferRef::Null();
		}

#ifdef GPU_API_DEBUG_PATH
		d3dBuff->SetPrivateData( WKPDID_D3DDebugObjectName, nameLength, name );
#endif


		// Create GpuApi buffer
		Uint32 newBufferId = dd.m_Buffers.Create( 1 );
		if ( !newBufferId )
		{
			SAFE_RELEASE( d3dBuff );
			GPUAPI_HALT( "Failed to create gpuapi buffer despite it was tested as possible" );
			return BufferRef::Null();
		}

		// Initialize new buffer
		SBufferData &bufferData = dd.m_Buffers.Data( newBufferId );
		bufferData.m_Desc.category = category;
		bufferData.m_Desc.size = bufferSize;
		bufferData.m_Desc.usage = usage;
		bufferData.m_Desc.accessFlags = accessFlags;
		bufferData.m_pBufferResource = d3dBuff;

		if ( bd.BindFlags & D3D11_BIND_SHADER_RESOURCE )
		{
			GPUAPI_ASSERT( category == BCC_Raw || category == BCC_Structured || category == BCC_StructuredUAV || category == BCC_IndirectUAV 
				|| category == BCC_StructuredAppendUAV || category == BCC_VertexSRV || category == BCC_Index16BitUAV );

			ID3D11ShaderResourceView* srv = nullptr;

			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			ZeroMemory( &desc, sizeof(desc) );
			desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			desc.BufferEx.FirstElement = 0;
			desc.BufferEx.NumElements = elementCount;

			if ( category == BCC_Raw )
			{
				desc.Format = DXGI_FORMAT_R32_TYPELESS;
				desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
			} 
			else if( category == BCC_IndirectUAV )
			{
				desc.Format = DXGI_FORMAT_R32_UINT;
			}
			else if(category == BCC_VertexSRV )
			{
				// This is a Vertex Buffer that can be binded as CS UAV
				desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
			}
			else if(category == BCC_Index16BitUAV)
			{
				// This is an Index Buffer that can be binded as CS UAV
				desc.Format = DXGI_FORMAT_R16_UINT;
			}
			else
			{
				desc.Format = DXGI_FORMAT_UNKNOWN;
			}
			GPUAPI_MUST_SUCCEED( GetDevice()->CreateShaderResourceView( d3dBuff, &desc, &srv ) );
			
			bufferData.m_pShaderResourceView = srv;
		}

		if( bd.BindFlags & D3D11_BIND_UNORDERED_ACCESS )
		{
			GPUAPI_ASSERT( category == BCC_Raw || category == BCC_StructuredUAV || category == BCC_IndirectUAV 
				|| category == BCC_StructuredAppendUAV || category == BCC_Index16BitUAV );

			ID3D11UnorderedAccessView* uav = nullptr;

			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			ZeroMemory( &uavDesc, sizeof(uavDesc) );
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = elementCount; 
			
			if ( category == BCC_Raw )
			{
				uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;			// Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
				uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
			} 
			else if(category == BCC_StructuredAppendUAV)
			{
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;				// Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
				uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
			}
			else if(category == BCC_IndirectUAV)
			{
				uavDesc.Format = DXGI_FORMAT_R32_UINT;				// This is a indirect Buffer
			}
			else if(category == BCC_VertexSRV )
			{
				uavDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;	// This is a Vertex Buffer that can be binded as CS UAV
			}
			else if(category == BCC_Index16BitUAV)
			{
				uavDesc.Format = DXGI_FORMAT_R16_UINT;				// This is an Index Buffer that can be binded as CS UAV
			}
			else
			{
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;				// Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffe
			}
			GPUAPI_MUST_SUCCEED( GetDevice()->CreateUnorderedAccessView( d3dBuff, &uavDesc, &uav ) );

			bufferData.m_pUnorderedAccessView = uav;
		}

		if ( usage == BUT_ImmutableInPlace)
		{
#ifdef RED_PLATFORM_DURANGO
			if ( initData->m_memRegionHandle.IsValid() )
			{
				GPUAPI_ASSERT( initData->m_memRegionHandle.IsValid() );

				// Store the memory region ptr if we initialized from it
				bufferData.m_memoryRegion = initData->m_memRegionHandle;
			}
#endif
#ifndef RED_FINAL_BUILD
			dd.m_MeshStats.AddBuffer( bufferSize, category );
#endif
		}

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

		GPUAPI_ASSERT( bufferDesc.usage != BUT_Immutable && bufferDesc.usage != BUT_ImmutableInPlace, TXT("Cannot lock an immutable buffer") );
		if ( bufferDesc.usage == BUT_Immutable || bufferDesc.usage == BUT_ImmutableInPlace )
		{
			return nullptr;
		}


		if ( bufferDesc.category == BCC_Constant )
		{
			GPUAPI_ASSERT( offset == 0 && size == bufferDesc.size, TXT("Cannot lock only part of a constant buffer") );
			if ( offset != 0 || size != bufferDesc.size )
			{
				return nullptr;
			}
		}


		// Lock buffer
		void *resultPtr = nullptr;

		// HACK DX10 locking with offset is not supported, allocating memory for temp buffer
		GPUAPI_ASSERT( offset == 0 || bufferDesc.usage == BUT_Dynamic, TXT("locking with offset is not supported") );

		if ( bufferDesc.usage == BUT_Dynamic || bufferDesc.usage == BUT_Staging )
		{
			ID3D11Buffer* buff = GetDeviceData().m_Buffers.Data(ref).m_pBufferResource;
			D3D11_MAPPED_SUBRESOURCE mapped;
			HRESULT res = GetDeviceContext()->Map( buff, 0, (D3D11_MAP)MapBuffLockFlagsToD3DLockType( flags ), 0, &mapped );
			GPUAPI_ASSERT( res == S_OK, TXT( "Locking buffer for write failed with error code: %li, out of memory?" ), res);
			if (res != S_OK)
			{
				GPUAPI_LOG( TXT( "Locking buffer failed with error code: %li, out of memory?" ), res );
				return nullptr;
			}
			resultPtr = (void*)((Uint8*)mapped.pData + offset);
		}
		else
		{
			if ( flags & BLF_Read )
			{
				GPUAPI_HALT( "can't lock non readable buffer for read");
				return nullptr;
			}

			GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer = GPU_API_ALLOCATE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( bufferDesc ), size, 16 );
			GetDeviceData().m_Buffers.Data(ref).m_lockedSize = size;
			GetDeviceData().m_Buffers.Data(ref).m_lockedOffset = offset;
			return GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer;
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

		const BufferDesc& bufferDesc = GetBufferDesc(ref);

		GPUAPI_ASSERT( ( offset + size ) <= bufferDesc.size );

		GPUAPI_ASSERT( bufferDesc.usage != BUT_Immutable && bufferDesc.usage != BUT_ImmutableInPlace, TXT("Cannot lock an immutable buffer") );
		if ( bufferDesc.usage == BUT_Immutable || bufferDesc.usage == BUT_ImmutableInPlace )
		{
			return nullptr;
		}

		if ( bufferDesc.category == BCC_Constant )
		{
			GPUAPI_ASSERT( offset == 0 && size == bufferDesc.size, TXT("Cannot lock only part of a constant buffer") );
			if ( offset != 0 || size != bufferDesc.size )
			{
				return nullptr;
			}
		}

		// Lock buffer
		void *resultPtr = nullptr;

		// HACK DX10 locking with offset is not supported, allocating memory for temp buffer
		GPUAPI_ASSERT( offset == 0 || bufferDesc.usage == BUT_Dynamic, TXT("locking with offset is not supported") );

		if ( bufferDesc.usage == BUT_Dynamic || bufferDesc.usage == BUT_Staging )
		{
			ID3D11Buffer* buff = GetDeviceData().m_Buffers.Data(ref).m_pBufferResource;
			D3D11_MAPPED_SUBRESOURCE mapped;
			HRESULT res = ((ID3D11DeviceContext*)deferredContext)->Map( buff, 0, (D3D11_MAP)MapBuffLockFlagsToD3DLockType( flags ), 0, &mapped );
			GPUAPI_ASSERT(res == S_OK, TXT("Locking buffer for write failed with error code: %li, out of memory?"), res);
			if (res != S_OK)
			{
				GPUAPI_LOG( TXT( "Locking buffer failed with error code: %li, out of memory?" ), res);
				return nullptr;
			}
			resultPtr = (void*)((Uint8*)mapped.pData + offset);
		}
		else
		{
			// TODO : Maybe disable this path? GpuApiGnm doesn't support it. Can use LoadBufferData instead (through CRenderInterface)
			if ( flags & BLF_Read )
			{
				GPUAPI_HALT( "can't lock non readable buffer for read" );
				return nullptr;
			}

			GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer = GPU_API_ALLOCATE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( bufferDesc ), size, 16 );
			GetDeviceData().m_Buffers.Data(ref).m_lockedSize = size;
			return GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer;
		}

		// Finish
		return resultPtr;
	}

	void UnlockBuffer( const BufferRef &ref )
	{
		GPUAPI_ASSERT( ref );

		SBufferData& data = GetDeviceData().m_Buffers.Data(ref);

		eBufferChunkCategory category = data.m_Desc.category;
		eBufferUsageType usage = data.m_Desc.usage;

		if ( usage == BUT_Dynamic || usage == BUT_Staging )
		{
			GetDeviceContext()->Unmap( data.m_pBufferResource, 0 ); 
		}
		else
		{
			// HACK DX10 deallocating memory for temp buffer
			void* updateBuffer = data.m_lockedBuffer;
			if (updateBuffer)
			{
				D3D11_BOX boxUpdate;
				D3D11_BOX* dstBox = nullptr;
				if (category!=BCC_Constant) //constant buffers require null box pointers
				{
					boxUpdate.left = data.m_lockedOffset;
					boxUpdate.top = 0;
					boxUpdate.front = 0;
					boxUpdate.right = data.m_lockedOffset + data.m_lockedSize;
					boxUpdate.bottom = 1;
					boxUpdate.back = 1;
					dstBox = &boxUpdate;
				}

				GetDeviceContext()->UpdateSubresource( data.m_pBufferResource, 0, dstBox, data.m_lockedBuffer, 0, 0 );
				GPU_API_FREE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( data.m_Desc ), data.m_lockedBuffer );
				data.m_lockedBuffer = nullptr;
			}
		}
	}

	void UnlockBufferAsync( const BufferRef &ref, void* deferredContext )
	{
		GPUAPI_ASSERT( ref );

		eBufferUsageType usage = GetDeviceData().m_Buffers.Data(ref).m_Desc.usage;

		if ( usage == BUT_Dynamic || usage == BUT_Staging )
		{
			((ID3D11DeviceContext*)deferredContext)->Unmap( GetDeviceData().m_Buffers.Data(ref).m_pBufferResource, 0 ); 
		}
		else
		{
			// HACK DX10 deallocating memory for temp buffer
			SBufferData& data = GetDeviceData().m_Buffers.Data(ref);
			void* updateBuffer = GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer;
			if (updateBuffer)
			{
				((ID3D11DeviceContext*)deferredContext)->UpdateSubresource( data.m_pBufferResource, 0, nullptr, data.m_lockedBuffer, data.m_lockedSize, 0 );
				GPU_API_FREE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( data.m_Desc ), data.m_lockedBuffer );
				data.m_lockedBuffer = nullptr;
			}
		}
	}


	void LoadBufferData( const GpuApi::BufferRef &destBuffer, Uint32 offset, Uint32 size, const void* srcMemory )
	{
		SDeviceData &dd = GetDeviceData();

		GPUAPI_ASSERT( destBuffer );
		GPUAPI_ASSERT( dd.m_Buffers.IsInUse( destBuffer ) );

		SBufferData &destData = dd.m_Buffers.Data( destBuffer );

		if ( destData.m_Desc.category == BCC_Constant )
		{
			GPUAPI_ASSERT( offset == 0 && size == destData.m_Desc.size, TXT("Cannot update only part of a constant buffer") );
			if ( offset != 0 || size != destData.m_Desc.size )
			{
				return;
			}
		}

		if ( destData.m_Desc.usage == BUT_Default )
		{
			// HACK DX10 deallocating memory for temp buffer
			D3D11_BOX boxUpdate;
			boxUpdate.left = offset;
			boxUpdate.top = 0;
			boxUpdate.front = 0;
			boxUpdate.right = offset + size;
			boxUpdate.bottom = 1;
			boxUpdate.back = 1;

			D3D11_BOX* boxPtr = &boxUpdate;
			if ( destData.m_Desc.category == BCC_Constant )
			{
				boxPtr = nullptr;
			}

			GetDeviceContext()->UpdateSubresource( destData.m_pBufferResource, 0, boxPtr, srcMemory, 0, 0 );
		}
		else
		{
			GPUAPI_HALT( "Cannot load buffer data. Buffer must have BUT_Default. Could maybe support Dynamic or Staging here by mapping (with some consideration about Discard/NoOverwrite/etc), but not needed yet." );
		}
	}


	void CopyBuffer( const BufferRef &dest, Uint32 destOffset, const BufferRef &source, Uint32 sourceOffset, Uint32 length )
	{
		SDeviceData &dd = GetDeviceData();

		GPUAPI_ASSERT( !dest.isNull() && !source.isNull() );
		GPUAPI_ASSERT( dd.m_Buffers.IsInUse( dest ) && dd.m_Buffers.IsInUse( source ) );

		SBufferData &destData = dd.m_Buffers.Data( dest );
		SBufferData &srcData = dd.m_Buffers.Data( source );

		D3D11_BOX srcBox;
		srcBox.left = sourceOffset;
		srcBox.right = sourceOffset + length;
		srcBox.top = srcBox.front = 0;
		srcBox.bottom = srcBox.back = 1;
		GetDeviceContext()->CopySubresourceRegion( destData.m_pBufferResource, 0, destOffset, 0, 0, srcData.m_pBufferResource, 0, &srcBox );
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
		Red::System::StringCopy( data.m_debugPath, debugPath, ARRAYSIZE(data.m_debugPath) );

		Uint32 pathLen = ( Uint32 )Red::System::StringLength( data.m_debugPath );

		// Destroy previous data
		data.m_pBufferResource->SetPrivateData( WKPDID_D3DDebugObjectName, 0, nullptr );

		if (pathLen > 0)
		{
			data.m_pBufferResource->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, data.m_debugPath );
		}
#endif
	}

	void BindConstantBuffer( Uint32 slot, BufferRef buffer, eShaderType shaderStage /*= PixelShader*/ )
	{
		GPUAPI_ASSERT( IsInit() );

		ID3D11Buffer* d3dBuff = nullptr;

		if ( buffer )
		{
			d3dBuff = GetD3DConstantBuffer( buffer );

			GPUAPI_ASSERT( d3dBuff );
		}

		switch ( shaderStage )
		{
		case PixelShader: 
			GetDeviceContext()->PSSetConstantBuffers( slot, 1, &d3dBuff );
			break;
		case VertexShader: 
			GetDeviceContext()->VSSetConstantBuffers( slot, 1, &d3dBuff );
			break;
		case GeometryShader: 
			GetDeviceContext()->GSSetConstantBuffers( slot, 1, &d3dBuff );
			break;
		case HullShader: 
			GetDeviceContext()->HSSetConstantBuffers( slot, 1, &d3dBuff );
			break;
		case DomainShader: 
			GetDeviceContext()->DSSetConstantBuffers( slot, 1, &d3dBuff );
			break;
		case ComputeShader: 
			GetDeviceContext()->CSSetConstantBuffers( slot, 1, &d3dBuff );
			break;
		}		
	}
}
