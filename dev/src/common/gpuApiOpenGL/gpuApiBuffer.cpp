/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"

namespace GpuApi
{	

	// ----------------------------------------------------------------------

	inline GLuint GetGLBuffer( const BufferRef &ref )
	{
		GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Buffers.Data(ref).m_buffer) );
		return ref ? GetDeviceData().m_Buffers.Data(ref).m_buffer : 0;
	}


	//inline ID3D11Buffer* GetD3DConstantBuffer( const BufferRef &ref )
	//{
	//	GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Buffers.Data(ref).m_pBufferResource) );
	//	return ref ? GetDeviceData().m_Buffers.Data(ref).m_pBufferResource : NULL;
	//}

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
		if ( 0 == dd.m_Buffers.DecRefCount( buffer ) )
		{
			SBufferData &data = dd.m_Buffers.Data( buffer );

			glDeleteBuffers( 1, &data.m_buffer );

			// Destroy shit
			dd.m_Buffers.Destroy( buffer );
		}

		return 0;
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

	void BindVertexBuffers( Uint32 startIndex, Uint32 count, const BufferRef* buffers, const Uint32* strides, const Uint32* offsets )
	{
		const Uint32 maxVertexBufferCount = GPUAPI_VERTEX_LAYOUT_MAX_SLOTS;

		if (count == 0 && startIndex == 0 && buffers == nullptr)
		{
			GPUAPI_ASSERT( IsInit() );
			GLuint nullBuffers[maxVertexBufferCount] = {};
			GLintptr nullOffsets[maxVertexBufferCount] = {};
			GLsizei nullStrides[maxVertexBufferCount] = {};
			glBindVertexBuffers( 0, GPUAPI_VERTEX_LAYOUT_MAX_SLOTS, &nullBuffers[0], &nullOffsets[0], &nullStrides[0] );
			return;
		}

		GLuint glBuffers[maxVertexBufferCount];
		GLintptr glOffsets[maxVertexBufferCount];
		GLsizei glStrides[maxVertexBufferCount];

		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( buffers == nullptr )
			{
				glBuffers[i] = 0;
			}
			else
			{
				glBuffers[i] = GetGLBuffer( buffers[i] );
				glOffsets[i] = offsets[i];
				glStrides[i] = strides[i];
			}
		}

		//// TODO GL fallback when 4.4 is not supported
		glBindVertexBuffers( startIndex, count, &glBuffers[0], &glOffsets[0], &glStrides[0] );

		//// alternative from the documentation for 4.3
		//for (Uint32 i = 0; i < count; ++i) 
		//{
		//	if (buffers == NULL) 
		//	{
		//		glBindVertexBuffer( startIndex + i, 0, 0, 0 );
		//	}
		//	else 
		//	{
		//		glBindVertexBuffer( startIndex + i, glBuffers[i], glOffsets[i], glStrides[i] );
		//	}
		//}

		//// for now use 4.2
		//if (buffers == NULL) 
		//{
		//	glBindBuffer( GL_ARRAY_BUFFER, 0 );
		//	glDisableVertexAttribArray( 0 );
		//}
		//else 
		//{
		//	glBindBuffer( GL_ARRAY_BUFFER, glBuffers[0] );
		//}
	}

	void BindIndexBuffer( const BufferRef &buffer, Uint32 offset )
	{
		SDeviceData &dd = GetDeviceData();
		if ( buffer )
		{
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, GetGLBuffer(buffer) );
			dd.m_IndexBuffer = buffer;
		}
		else
		{
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
			dd.m_IndexBuffer = BufferRef::Null();
		}
	}

	void BindBufferSRV( const BufferRef &buffer, Uint32 slot, eShaderType shaderStage )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//ID3D11ShaderResourceView* texture = NULL;

		//if (!buffer.isNull())
		//{
		//	texture = (GetDeviceData().m_Buffers.Data(buffer).m_pShaderResourceView);
		//}

		//switch (shaderStage)
		//{
		//case PixelShader:
		//	{
		//		GetDeviceContext()->PSSetShaderResources( slot, 1, &texture );
		//		break;
		//	}
		//case VertexShader:
		//	{
		//		GetDeviceContext()->VSSetShaderResources( slot, 1, &texture );
		//		break;
		//	}
		//case GeometryShader:
		//	{
		//		GetDeviceContext()->GSSetShaderResources( slot, 1, &texture );
		//		break;
		//	}
		//case HullShader:
		//	{
		//		GetDeviceContext()->HSSetShaderResources( slot, 1, &texture );
		//		break;
		//	}
		//case DomainShader:
		//	{
		//		GetDeviceContext()->DSSetShaderResources( slot, 1, &texture );
		//		break;
		//	}
		//case ComputeShader:
		//	{
		//		GetDeviceContext()->CSSetShaderResources( slot, 1, &texture );
		//		break;
		//	}
		//}
	}

	void BindBufferUAV( const BufferRef &buffer, Uint32 slot )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//GPUAPI_ASSERT( !(buffer && !GetDeviceData().m_Buffers.Data(buffer).m_pUnorderedAccessView ) );

		//ID3D11UnorderedAccessView* uav = NULL;

		//if (!buffer.isNull())
		//{
		//	uav = GetDeviceData().m_Buffers.Data(buffer).m_pUnorderedAccessView;
		//}

		//GetDeviceContext()->CSSetUnorderedAccessViews( slot, 1, &uav, 0 );
	}


	void BindStreamOutBuffers( Uint32 count, const BufferRef* buffers, const Uint32* offsets )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//// There doesn't seem to be a D3D #define for max number of SO targets, but docs say it's 4.
		//GPUAPI_ASSERT( count <= 4 );
		//if ( count > 4 )
		//{
		//	count = 4;
		//}

		//ID3D11Buffer* d3dBuffers[4] = { nullptr, nullptr, nullptr, nullptr };

		//for ( Uint32 i = 0; i < count; ++i )
		//{
		//	GPUAPI_ASSERT( !(buffers[i] && !GetDeviceData().m_Buffers.Data(buffers[i]).m_pBufferResource ) );
		//	if (!buffers[i].isNull())
		//	{
		//		d3dBuffers[i] = GetDeviceData().m_Buffers.Data(buffers[i]).m_pBufferResource;
		//	}
		//}

		//GetDeviceContext()->SOSetTargets( count, d3dBuffers, offsets );
	}


	BufferRef CreateStagingCopyBuffer( BufferRef originalBuffer )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");
		return BufferRef::Null();

//		SDeviceData &dd = GetDeviceData();
//
//		BufferDesc originalDesc = GetDeviceData().m_Buffers.Data(originalBuffer).m_Desc;
//
//		GPUAPI_ASSERT( originalDesc.usage != BUT_Dynamic && originalDesc.category != BCC_Structured, TXT("dynamic and structured buffers are not supported for staging copies") );
//
//		ID3D11Buffer* d3dBuffer = nullptr;
//		D3D11_BUFFER_DESC bd;
//		bd.Usage = D3D11_USAGE_STAGING;
//		bd.BindFlags = 0;
//		bd.ByteWidth = originalDesc.size;
//		bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
//		bd.MiscFlags = 0;
//
//		HRESULT hRes = dd.m_pDevice->CreateBuffer( &bd, nullptr, &d3dBuffer);
//
//		GPUAPI_ASSERT( hRes == S_OK, TXT("Can't create staging buffer") );
//
//		GetDeviceContext()->CopySubresourceRegion( d3dBuffer, 0, 0, 0, 0, GetDeviceData().m_Buffers.Data(originalBuffer).m_pBufferResource, 0, nullptr );
//
//		// Create GpuApi buffer
//		Uint32 newBufferId = dd.m_Buffers.Create( 1 );
//		if ( !newBufferId )
//		{
//			SAFE_RELEASE( d3dBuffer );
//			GPUAPI_HALT( TXT( "Failed to create gpuapi buffer despite it was tested as possible" ) );
//			return BufferRef::Null();
//		}
//#ifdef GPU_API_DEBUG_PATH
//		d3dBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, 17, "stagingCopyBuffer" );
//#endif
//
//		// Initialize new buffer
//		SBufferData &data = dd.m_Buffers.Data( newBufferId );
//		data.m_Desc.category = BCC_Vertex;
//		data.m_Desc.size = originalDesc.size;
//		data.m_Desc.accessFlags = BAF_CPURead;
//		data.m_Desc.usage = BUT_Staging;
//		data.m_pBufferResource = d3dBuffer;
//
//		// Finalize
//		GPUAPI_ASSERT( newBufferId && dd.m_Buffers.IsInUse( newBufferId ) );
//		return BufferRef( newBufferId );
	}

	BufferRef CreateBuffer( Uint32 bufferSize, eBufferChunkCategory category, eBufferUsageType usage, Uint32 accessFlags, const BufferInitData* data )
	{
		SDeviceData &dd = GetDeviceData();

		// Test input conditions

		if ( bufferSize < 1 )
		{
			GPUAPI_ERROR( TXT( "Invalid buffer size" ) );
			return BufferRef::Null();
		}

		if ( !dd.m_Buffers.IsCapableToCreate( 1 ) )
		{
			GPUAPI_ERROR( TXT( "Failed to create buffer." ) );
			return BufferRef::Null();
		}

		// D3D doesn't seem to like BIND_STREAM_OUT combined with BIND_UNORDERED_ACCESS...
		if ( ( category == BCC_Raw || category == BCC_Structured ) && usage == BUT_StreamOut )
		{
			GPUAPI_ERROR( TXT( "Raw/Structured buffers cannot have StreamOut usage." ) );
			return BufferRef::Null();
		}

		GLenum glUsage = MapAccessFlagsAndUsageToOGLUsage( accessFlags, usage );

		GLuint buffer;
		glGenBuffers(1, &buffer);

		const void* initData = nullptr;
		if (data != nullptr)
		{
			initData = data->m_buffer;
		}

		switch ( category )
		{
		case BCC_Vertex:
			{
				glBindBuffer( GL_ARRAY_BUFFER, buffer );
				glBufferData( GL_ARRAY_BUFFER, bufferSize, initData, glUsage );
				glBindBuffer( GL_ARRAY_BUFFER, 0 );
			}
			break;

		case BCC_Index16Bit:	// falldown
		case BCC_Index32Bit:
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize, initData, glUsage);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			break;
		case BCC_Constant:
			{
				glBindBuffer(GL_UNIFORM_BUFFER, buffer);
				glBufferData(GL_UNIFORM_BUFFER, bufferSize, initData, glUsage);
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}
			break;
		case BCC_Structured:
			{
				GPUAPI_HALT("not implemented");
			}
			break;
		case BCC_Raw:
			{
				GPUAPI_HALT("not implemented");
			}
			break;
		default:
			GPUAPI_HALT("invalid buffer category");
		}

		//if ( glIsBuffer(buffer) == GL_FALSE )
		//{
		//	GPUAPI_ERROR(TXT("Failed to create buffer."));
		//	return BufferRef::Null();
		//}

		// Create GpuApi buffer
		Uint32 newBufferId = dd.m_Buffers.Create( 1 );
		if ( !newBufferId )
		{
			glDeleteBuffers( 1, &buffer );
			GPUAPI_HALT( "Failed to create gpuapi buffer despite it was tested as possible" );
			return BufferRef::Null();
		}

		// Initialize new buffer
		SBufferData &bufferData = dd.m_Buffers.Data( newBufferId );
		bufferData.m_Desc.category = category;
		bufferData.m_Desc.size = bufferSize;
		bufferData.m_Desc.usage = usage;
		bufferData.m_Desc.accessFlags = accessFlags;
		bufferData.m_buffer = buffer;

		if ( category == BCC_Raw || category == BCC_Structured )
		{
			GPUAPI_HALT("NOT IMPLEMENTED");
			//ID3D11ShaderResourceView* srv;
			//ID3D11UnorderedAccessView* uav;

			//D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			//ZeroMemory( &desc, sizeof(desc) );
			//desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			//desc.BufferEx.FirstElement = 0;

			//if ( category == BCC_Raw )
			//{
			//	desc.Format = DXGI_FORMAT_R32_TYPELESS;
			//	desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
			//	desc.BufferEx.NumElements = elementCount;
			//} 
			//else
			//{
			//	desc.Format = DXGI_FORMAT_UNKNOWN;
			//	desc.BufferEx.NumElements = elementCount;
			//} 
			//GetDevice()->CreateShaderResourceView( buffer, &desc, &srv );

			//D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			//ZeroMemory( &uavDesc, sizeof(uavDesc) );
			//uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			//uavDesc.Buffer.FirstElement = 0;

			//if ( category == BCC_Raw )
			//{
			//	// This is a Raw Buffer
			//	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
			//	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
			//	uavDesc.Buffer.NumElements = elementCount; 
			//} 
			//else
			//{
			//	// This is a Structured Buffer
			//	uavDesc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
			//	uavDesc.Buffer.NumElements = elementCount; 
			//}

			//GetDevice()->CreateUnorderedAccessView( buffer, &uavDesc, &uav );

			//bufferData.m_pShaderResourceView = srv;
			//bufferData.m_pUnorderedAccessView = uav;
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
			return MC_VertexBuffer;

		case BCC_Index16Bit:
		case BCC_Index32Bit:
			return MC_IndexBuffer;

		case BCC_Constant:
			return MC_ConstantBuffer;

		case BCC_Structured:
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

		GPUAPI_ASSERT( offset == 0 || bufferDesc.usage == BUT_Dynamic, TXT("locking with offset is not supported") );

		if ( bufferDesc.usage == BUT_Dynamic || bufferDesc.usage == BUT_Staging )
		{
			GLuint bufferID = GetDeviceData().m_Buffers.Data(ref).m_buffer;
			glBindBuffer( GL_ARRAY_BUFFER, bufferID );

			if ( flags & BLF_Discard )
			{
				GLenum usage = MapAccessFlagsAndUsageToOGLUsage( bufferDesc.accessFlags, bufferDesc.usage );
				glBufferData( GL_ARRAY_BUFFER, bufferDesc.size, nullptr, usage );
			}

			GLenum lockType = MapBuffLockFlagsToOGLLockType( flags );
			Uint8* bufferData = (Uint8*)glMapBuffer( GL_ARRAY_BUFFER, lockType );
			resultPtr = bufferData + offset;
			glBindBuffer( GL_ARRAY_BUFFER, 0 );

			//ID3D11Buffer* buff = GetDeviceData().m_Buffers.Data(ref).m_pBufferResource;
			//D3D11_MAPPED_SUBRESOURCE mapped;
			//HRESULT res = GetDeviceContext()->Map( buff, 0, (D3D11_MAP)MapBuffLockFlagsToD3D( flags ), 0, &mapped );
			//GPUAPI_ASSERT( res == S_OK, TXT( "Locking buffer for write failed with error code: %li, out of memory?" ), res);
			//if (res != S_OK)
			//{
			//	GPUAPI_LOG( TXT( "Locking buffer failed with error code: %li, out of memory?" ), res );
			//}
			//resultPtr = (void*)((Uint8*)mapped.pData + offset);
		}
		else
		{
			//if ( flags & BLF_Readonly )
			//{
			//	GPUAPI_HALT( TXT("can't lock non readable buffer for read") );
			//	return nullptr;
			//}

			//GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer = GPU_API_ALLOCATE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( bufferDesc ), size, 16 );
			//GetDeviceData().m_Buffers.Data(ref).m_lockedSize = size;
			//GetDeviceData().m_Buffers.Data(ref).m_lockedOffset = offset;
			//return GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer;
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

		// Lock buffer
		void *resultPtr = nullptr;

		GPUAPI_HALT("NOT IMPLEMENTED");

		//// HACK DX10 locking with offset is not supported, allocating memory for temp buffer
		//GPUAPI_ASSERT( offset == 0 || bufferDesc.usage == BUT_Dynamic, TXT("locking with offset is not supported") );

		//if ( bufferDesc.usage == BUT_Dynamic || bufferDesc.usage == BUT_Staging )
		//{
		//	ID3D11Buffer* buff = GetDeviceData().m_Buffers.Data(ref).m_pBufferResource;
		//	D3D11_MAPPED_SUBRESOURCE mapped;
		//	HRESULT res = ((ID3D11DeviceContext*)deferredContext)->Map( buff, 0, (D3D11_MAP)MapBuffLockFlagsToD3D( flags ), 0, &mapped );
		//	GPUAPI_ASSERT(res == S_OK, TXT("Locking buffer for write failed with error code: %li, out of memory?"), res);
		//	if (res != S_OK)
		//	{
		//		GPUAPI_LOG( TXT( "Locking buffer failed with error code: %li, out of memory?" ), res);
		//	}
		//	resultPtr = (void*)((Uint8*)mapped.pData + offset);
		//}
		//else
		//{

		//	if ( flags & BLF_Readonly )
		//	{
		//		GPUAPI_HALT( TXT("can't lock non readable buffer for read") );
		//		return nullptr;
		//	}

		//	GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer = GPU_API_ALLOCATE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( bufferDesc ), size, 16 );
		//	GetDeviceData().m_Buffers.Data(ref).m_lockedSize = size;
		//	return GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer;
		//}

		// Finish
		return resultPtr;
	}

	void UnlockBuffer( const BufferRef &ref )
	{
		GPUAPI_ASSERT( ref );

		eBufferUsageType usage = GetDeviceData().m_Buffers.Data(ref).m_Desc.usage;

		if ( usage == BUT_Dynamic || usage == BUT_Staging )
		{
			//GetDeviceContext()->Unmap( GetDeviceData().m_Buffers.Data(ref).m_pBufferResource, 0 ); 
			GLuint bufferID = GetDeviceData().m_Buffers.Data(ref).m_buffer;
			glBindBuffer( GL_ARRAY_BUFFER, bufferID );
			glUnmapBuffer(GL_ARRAY_BUFFER);
			glBindBuffer( GL_ARRAY_BUFFER, 0 );
		}
		else
		{
			//// HACK DX10 deallocating memory for temp buffer
			//SBufferData& data = GetDeviceData().m_Buffers.Data(ref);
			//void* updateBuffer = GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer;
			//if (updateBuffer)
			//{
			//	D3D11_BOX boxUpdate;
			//	boxUpdate.left = data.m_lockedOffset;
			//	boxUpdate.top = 0;
			//	boxUpdate.front = 0;
			//	boxUpdate.right = data.m_lockedOffset + data.m_lockedSize;
			//	boxUpdate.bottom = 1;
			//	boxUpdate.back = 1;

			//	GetDeviceContext()->UpdateSubresource( data.m_pBufferResource, 0, &boxUpdate, data.m_lockedBuffer, 0, 0 );
			//	GPU_API_FREE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( data.m_Desc ), data.m_lockedBuffer );
			//	data.m_lockedBuffer = NULL;
			//}
		}
	}

	void UnlockBufferAsync( const BufferRef &ref, void* deferredContext )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//GPUAPI_ASSERT( ref );

		//eBufferUsageType usage = GetDeviceData().m_Buffers.Data(ref).m_Desc.usage;

		//if ( usage == BUT_Dynamic || usage == BUT_Staging )
		//{
		//	((ID3D11DeviceContext*)deferredContext)->Unmap( GetDeviceData().m_Buffers.Data(ref).m_pBufferResource, 0 ); 
		//}
		//else
		//{
		//	// HACK DX10 deallocating memory for temp buffer
		//	SBufferData& data = GetDeviceData().m_Buffers.Data(ref);
		//	void* updateBuffer = GetDeviceData().m_Buffers.Data(ref).m_lockedBuffer;
		//	if (updateBuffer)
		//	{
		//		((ID3D11DeviceContext*)deferredContext)->UpdateSubresource( data.m_pBufferResource, 0, NULL, data.m_lockedBuffer, data.m_lockedSize, 0 );
		//		GPU_API_FREE( GpuMemoryPool_LockedBufferData, GetMemoryClassForBufferType( data.m_Desc ), data.m_lockedBuffer );
		//		data.m_lockedBuffer = NULL;
		//	}
		//}
	}

	void SetBufferDebugPath( const BufferRef &ref, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_Buffers.IsInUse(ref) );
		SBufferData &data = GetDeviceData().m_Buffers.Data(ref);
		Red::System::StringCopy( data.m_debugPath, debugPath, ARRAYSIZE(data.m_debugPath) );

		Uint32 pathLen = ( Uint32 )Red::System::StringLength( data.m_debugPath );

		// Destroy previous data
		data.m_pBufferResource->SetPrivateData( WKPDID_D3DDebugObjectName, 0, NULL );

		if (pathLen > 0)
		{
			data.m_pBufferResource->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, data.m_debugPath );
		}
#endif
	}

	void BindConstantBuffer( Uint32 slot, BufferRef buffer, eShaderType shaderStage /*= PixelShader*/ )
	{
		GPUAPI_ASSERT( IsInit() );
		
		GLuint bufferID = 0;

		if ( !buffer.isNull() )
		{
			SBufferData &data = GetDeviceData().m_Buffers.Data(buffer);
			bufferID = data.m_buffer;
		}

		switch ( shaderStage )
		{
		case PixelShader: 
			GPUAPI_ASSERT(slot < 6);
			glBindBufferBase( GL_UNIFORM_BUFFER, slot, bufferID );
			break;
		case VertexShader: 
		case GeometryShader: 
		case HullShader: 
		case DomainShader: 
			GPUAPI_ASSERT(slot < 4);
			glBindBufferBase( GL_UNIFORM_BUFFER, slot + 6, bufferID );
			break;
		case ComputeShader: 
			GPUAPI_ASSERT(slot < 6);
			glBindBufferBase( GL_UNIFORM_BUFFER, slot + 10, bufferID );
			break;
		}
	}
}
