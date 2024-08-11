/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "..\gpuApiUtils\gpuApiVertexFormats.h"


namespace GpuApi
{


	// HACK : We're using the ShaderRef ID as an index into a static array. IDs start at 1, so we need to subtract to get a 0-based index.
	static Int32 ShaderRefToIndex( const ShaderRef& shader )
	{
		return shader.id - 1;
	}


	void AddRef( const VertexLayoutRef &layoutRef )
	{
		GPUAPI_ASSERT( GetDeviceData().m_VertexLayouts.IsInUse(layoutRef) );
		GetDeviceData().m_VertexLayouts.IncRefCount( layoutRef );
	}

	Int32 Release( const VertexLayoutRef &layoutRef )
	{	
		GPUAPI_ASSERT( GetDeviceData().m_VertexLayouts.IsInUse(layoutRef) );

		// Release
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_VertexLayouts.GetRefCount(layoutRef) >= 1 );
		Int32 refCount = dd.m_VertexLayouts.DecRefCount( layoutRef );

		// Optionally destroy
		if ( 0 == refCount )
		{	
			SVertexLayoutData &data = dd.m_VertexLayouts.Data( layoutRef );
			for ( Uint32 i = 0; i < MAX_SHADER_COUNT; ++i )
			{
				if ( data.m_inputLayouts[i] )
				{
					ULONG refcount = data.m_inputLayouts[i]->Release();
#ifdef NO_GPU_ASSERTS
					RED_UNUSED( refcount );
#endif
					GPUAPI_ASSERT( refcount == 0, TXT( "D3DInputLayout refcount > 0, object won't be destroyed" ) );
				}
			}
			dd.m_VertexLayouts.Destroy( layoutRef );
		}
		return refCount;
	}


	VertexLayoutRef CreateVertexLayout( const VertexLayoutDesc &desc )
	{
		if ( !IsInit() )
		{
			GPUAPI_HALT( "Not init during attempt to create sampler state" );
			return VertexLayoutRef::Null();
		}

		if ( !IsDescSupported( desc ) )
		{
			return VertexLayoutRef::Null();
		}

		SDeviceData &dd = GetDeviceData();

		VertexLayoutRef id = VertexLayoutRef( dd.m_VertexLayouts.FindDescAddRef( desc ) );
		if ( !id )
		{
			id = VertexLayoutRef( dd.m_VertexLayouts.Create( 1 ) );
			if ( id )
			{	
				SVertexLayoutData &data = dd.m_VertexLayouts.Data( id );
				data.m_Desc = desc;
			}
		}

		return id;
	}

	Bool IsDescSupported( const VertexLayoutDesc &desc )
	{
		// Check for conflicting slot types. Each slot should have a single type (can't mix instance/vertex elements within a single slot)
		VertexPacking::eSlotType slotTypes[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS];
		for ( size_t i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_SLOTS; ++i ) slotTypes[i] = VertexPacking::ST_Invalid;

		// Also check for duplicate usages. Each usage can appear at most once.
		Bool usedUsages[VertexPacking::PS_Max * GPUAPI_VERTEX_LAYOUT_USAGE_INDEX_COUNT] = { 0 };


		for ( size_t i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
		{
			const VertexPacking::PackingElement& element = desc.m_elements[i];

			if ( element.IsEmpty() )
			{
				break;
			}

			// Make sure properties are in range.
			if ( element.m_usage < 0 || element.m_usage >= VertexPacking::PS_Max ||
				 element.m_usageIndex >= GPUAPI_VERTEX_LAYOUT_USAGE_INDEX_COUNT ||
				 element.m_type < 0 || element.m_type >= VertexPacking::PT_Max ||
				 element.m_slotType < 0 || element.m_slotType >= VertexPacking::ST_Max ||
				 element.m_slot >= GPUAPI_VERTEX_LAYOUT_MAX_SLOTS )
			{
				return false;
			}

			// Make sure slot types are consistent...
			if ( slotTypes[element.m_slot] != VertexPacking::ST_Invalid && slotTypes[element.m_slot] != element.m_slotType )
			{
				return false;
			}
			slotTypes[element.m_slot] = element.m_slotType;

			// Make sure usage hasn't already been seen.
			if ( element.m_usage != VertexPacking::PS_Padding )
			{
				Uint32 usageUsagesIdx = element.m_usage * GPUAPI_VERTEX_LAYOUT_USAGE_INDEX_COUNT + element.m_usageIndex;
				if ( usedUsages[usageUsagesIdx] )
				{
					return false;
				}
				usedUsages[usageUsagesIdx] = true;
			}
		}

		return true;
	}



	// Added for SF. Their device independent API will doesn't do things in the same order as the aggregate GpuApi normally wants.
	// Possibly instead of this, create even more Draw*Primitive functions instead.
	void SetVertexFormatRaw( eBufferChunkType chunkType, Bool setInputLayout )
	{
		SetVertexFormatRaw( GetVertexLayoutForChunkType( chunkType ), setInputLayout );
	}

	void SetVertexFormatRaw( VertexLayoutRef vertexLayout, Bool setInputLayout )
	{
		SDeviceData &dd = GetDeviceData();
		if ( dd.m_VertexLayout != vertexLayout )
		{
			dd.m_VertexLayout = vertexLayout;
			if(setInputLayout)
			{
				GetDeviceContext()->IASetInputLayout(GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ));
			}
		}
	}
	

	VertexLayoutRef GetVertexLayoutForChunkType( eBufferChunkType chunkType )
	{
		VertexLayoutRef layout = GetDeviceData().m_ChunkTypeVertexLayouts[ chunkType ];
		GPUAPI_ASSERT( !layout.isNull(), TXT( "BufferChunkType %d has no vertex layout. Need to set up Packing Elements in gpuApiVertexDeclarations.h and gpuApiVertexFormats.cpp." ), chunkType );
		return layout;
	}

	eBufferChunkType GetChunkTypeForVertexLayout( VertexLayoutRef layout )
	{
		SDeviceData &dd = GetDeviceData();
		for ( Uint32 i = 0; i < BCT_Max; ++i )
		{
			if ( dd.m_ChunkTypeVertexLayouts[i] == layout )
			{
				return static_cast< eBufferChunkType >( i );
			}
		}
		return BCT_Max;
	}


	void InitVertexDeclarations( bool assumeRefsPresent )
	{
		RED_UNUSED(assumeRefsPresent);

		SDeviceData &dd = GetDeviceData();
		// Get packing elements for each BCT, initialize layout for any that have proper data.
		for ( Uint32 i = 0; i < BCT_Max; ++i )
		{
			const eBufferChunkType vertexFormat = static_cast< eBufferChunkType >( i );
			const VertexPacking::PackingElement* packingElements = GetPackingForFormat( vertexFormat );
			if ( packingElements )
			{
				dd.m_ChunkTypeVertexLayouts[i] = CreateVertexLayout( VertexLayoutDesc( packingElements ) );
			}
		}

		// Make sure all vertex formats are initialized
		// TODO: Some of the "instanced" chunk types don't have a vertex format defined, so we can't initialize everything.
		//for ( Uint32 i = 0; i < BCT_Max; ++i )
		//{
		//	// The index buffer formats don't count...
		//	if ( i == BCT_IndexUInt || i == BCT_IndexUShort ) continue;
		//
		//	SDeviceData &dd = GetDeviceData();
		//	GPUAPI_ASSERT( !dd.m_ChunkTypeVertexLayouts[i].isNull() );
		//}
	}

	void ShutVertexDeclarations( bool dropRefs )
	{
		SDeviceData &dd = GetDeviceData();

		// Loop over all possible vertex layout IDs, release the internal D3D objects. They'll get recreated if they're needed again.
		for ( Uint32 i = 1; i <= dd.m_VertexLayouts._MaxResCount; ++i )
		{
			if ( !dd.m_VertexLayouts.IsInUse( i ) )
			{
				continue;
			}

			SVertexLayoutData& data = dd.m_VertexLayouts.Data( i );

			for ( Uint32 j = 0; j < MAX_SHADER_COUNT; ++j )
			{
				SAFE_RELEASE( data.m_inputLayouts[j] );
			}
		}

		// If we're dropping all references as well, release the chunk type layouts.
		if ( dropRefs )
		{
			for ( Uint32 i = 0; i < BCT_Max; ++i )
			{
				SafeRelease( dd.m_ChunkTypeVertexLayouts[i] );
				dd.m_ChunkTypeVertexLayouts[i] = VertexLayoutRef::Null();
			}
		}
	}

	Uint32 GetChunkTypeStride( eBufferChunkType type, Uint32 slot )
	{
		// Special case for index chunks...
		switch ( type )
		{
		case BCT_IndexUInt:		return sizeof( Uint32 );
		case BCT_IndexUShort:	return sizeof( Uint16 );
		}

		return GetVertexLayoutStride( GetVertexLayoutForChunkType( type ), slot );
	}


	Uint32 GetVertexLayoutStride( const VertexLayoutRef& ref, Uint32 slot )
	{
		SDeviceData &dd = GetDeviceData();

		if ( !dd.m_VertexLayouts.IsInUse( ref ) )
		{
			return 0;
		}

		const SVertexLayoutData& layoutData = dd.m_VertexLayouts.Data( ref );
		return layoutData.m_Desc.GetSlotStride( slot );
	}


	const VertexLayoutDesc* GetVertexLayoutDesc( const VertexLayoutRef& ref )
	{
		SDeviceData &dd = GetDeviceData();

		if ( !dd.m_VertexLayouts.IsInUse( ref ) )
		{
			return nullptr;
		}

		const SVertexLayoutData& layoutData = dd.m_VertexLayouts.Data( ref );
		return &layoutData.m_Desc;
	}



	static DXGI_FORMAT PackingTypeToDXGI( VertexPacking::ePackingType packingType )
	{
		switch ( packingType )
		{
		case VertexPacking::PT_Float1:		return DXGI_FORMAT_R32_FLOAT;
		case VertexPacking::PT_Float2:		return DXGI_FORMAT_R32G32_FLOAT;
		case VertexPacking::PT_Float3:		return DXGI_FORMAT_R32G32B32_FLOAT;
		case VertexPacking::PT_Float4:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case VertexPacking::PT_Float16_2:	return DXGI_FORMAT_R16G16_FLOAT;
		case VertexPacking::PT_Float16_4:	return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case VertexPacking::PT_Color:		return DXGI_FORMAT_R8G8B8A8_UNORM;
		case VertexPacking::PT_UByte1:		return DXGI_FORMAT_R8_UINT;
		case VertexPacking::PT_UByte4:		return DXGI_FORMAT_R8G8B8A8_UINT;
		case VertexPacking::PT_UByte4N:		return DXGI_FORMAT_R8G8B8A8_UNORM;
		case VertexPacking::PT_Byte4N:		return DXGI_FORMAT_R8G8B8A8_SNORM;
		case VertexPacking::PT_UShort1:		return DXGI_FORMAT_R16_UINT;
		case VertexPacking::PT_UShort2:		return DXGI_FORMAT_R16G16_UINT;
		case VertexPacking::PT_UShort4:		return DXGI_FORMAT_R16G16B16A16_UINT;
		case VertexPacking::PT_UShort4N:	return DXGI_FORMAT_R16G16B16A16_UNORM;
		case VertexPacking::PT_Short1:		return DXGI_FORMAT_R16_SINT;
		case VertexPacking::PT_Short2:		return DXGI_FORMAT_R16G16_SINT;
		case VertexPacking::PT_Short4:		return DXGI_FORMAT_R16G16B16A16_SINT;
		case VertexPacking::PT_Short4N:		return DXGI_FORMAT_R16G16B16A16_SNORM;
		case VertexPacking::PT_UInt1:		return DXGI_FORMAT_R32_UINT;
		case VertexPacking::PT_UInt2:		return DXGI_FORMAT_R32G32_UINT;
		case VertexPacking::PT_UInt3:		return DXGI_FORMAT_R32G32B32_UINT;
		case VertexPacking::PT_UInt4:		return DXGI_FORMAT_R32G32B32A32_UINT;
		case VertexPacking::PT_Int1:		return DXGI_FORMAT_R32_SINT;
		case VertexPacking::PT_Int2:		return DXGI_FORMAT_R32G32_SINT;
		case VertexPacking::PT_Int3:		return DXGI_FORMAT_R32G32B32_SINT;
		case VertexPacking::PT_Int4:		return DXGI_FORMAT_R32G32B32A32_SINT;
		case VertexPacking::PT_Dec4:		return DXGI_FORMAT_R10G10B10A2_UNORM;
		default: GPUAPI_HALT( "Invalid packing element data type" ); return DXGI_FORMAT_UNKNOWN;
		}
	}

	static void FillSemanticInfo( D3D11_INPUT_ELEMENT_DESC* dxElement, const VertexPacking::PackingElement& element )
	{
		if ( !MapPackingElementToSemanticAndIndex( element, dxElement->SemanticName, dxElement->SemanticIndex ) )
		{
			GPUAPI_HALT( "Invalid packing element" );
			dxElement->SemanticName = nullptr;
			dxElement->SemanticIndex = 0;
		}
	}


	ID3D11InputLayout* GetInputLayout( const VertexLayoutRef& vertexLayout, const ShaderRef& vertexShader )
	{
		GPUAPI_ASSERT( !vertexShader.isNull() );
		if ( vertexShader.isNull() )
		{
			return NULL;
		}

		SDeviceData &dd = GetDeviceData();
		SShaderData& shaderData = dd.m_Shaders.Data(vertexShader);

		SVertexLayoutData& layoutData = dd.m_VertexLayouts.Data( vertexLayout );

		Int32 layoutIndex = ShaderRefToIndex( vertexShader );
		GPUAPI_ASSERT( layoutIndex >= 0 && layoutIndex < MAX_SHADER_COUNT );
		if ( layoutIndex < 0 || layoutIndex >= MAX_SHADER_COUNT )
		{
			return NULL;
		}

		ID3D11InputLayout* layout = layoutData.m_inputLayouts[layoutIndex];
		if ( layout == NULL )
		{
			// To make things a little more efficient, we'll cache the size for each slot as we build the DX element descs. Otherwise
			// figuring out offsets for each D3D element would be either slow or complicated or just annoying.
			Uint32 cachedSlotOffsets[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = { 0 };

			D3D11_INPUT_ELEMENT_DESC declElements[GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS];
			Red::System::MemoryZero( declElements, sizeof(declElements) );

			Uint32 elementCount = 0;

			// Write newElements
			for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
			{
				const VertexPacking::PackingElement& element = layoutData.m_Desc.m_elements[i];

				if ( element.IsEmpty() )
				{
					break;
				}

				// If this element is padding, we don't create a INPUT_ELEMENT_DESC for it, just track the
				// space that's being skipped.
				if ( element.m_usage == VertexPacking::PS_Padding )
				{
					cachedSlotOffsets[element.m_slot] += GetPackingTypeSize( element.m_type );
					continue;
				}

				D3D11_INPUT_ELEMENT_DESC& decl = declElements[elementCount];

				FillSemanticInfo( &decl, element );
				decl.Format = PackingTypeToDXGI( element.m_type );
				
				decl.AlignedByteOffset = cachedSlotOffsets[element.m_slot];
				cachedSlotOffsets[element.m_slot] += GetPackingTypeSize( element.m_type );

				decl.InputSlot = element.m_slot;
				switch( element.m_slotType )
				{
				case VertexPacking::ST_PerVertex:
					decl.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					decl.InstanceDataStepRate = 0;
					break;

				case VertexPacking::ST_PerInstance:
					decl.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
					decl.InstanceDataStepRate = 1;
					break;
				}

				++elementCount;
			}


			HRESULT res = GetDevice()->CreateInputLayout( &declElements[0], elementCount, shaderData.m_byteCode->GetBufferPointer(), shaderData.m_byteCode->GetBufferSize(), &layoutData.m_inputLayouts[layoutIndex] );
			GPUAPI_ASSERT( SUCCEEDED( res ), TXT( "Failed to create input layout!" ) );
			if ( FAILED( res ) )
			{
				return nullptr;
			}

			layout = layoutData.m_inputLayouts[layoutIndex];
			GPUAPI_ASSERT( layout );
#ifdef GPU_API_DEBUG_PATH
			layout->SetPrivateData( WKPDID_D3DDebugObjectName, 2, "IL" );
#endif
		}
		return layout;
	}


	void ReleaseInputLayouts( const ShaderRef& shader )
	{
		if ( shader.isNull() )
		{
			return;
		}

		Int32 layoutIndex = ShaderRefToIndex( shader );
		GPUAPI_ASSERT( layoutIndex >= 0 && layoutIndex < MAX_SHADER_COUNT );
		if ( layoutIndex < 0 || layoutIndex >= MAX_SHADER_COUNT )
		{
			return;
		}

		SDeviceData &dd = GetDeviceData();
		for ( Uint32 i = 0; i < dd.m_VertexLayouts._MaxResCount; ++i )
		{
			if ( dd.m_VertexLayouts.IsInUse( i ) )
			{
				SVertexLayoutData& data = dd.m_VertexLayouts.Data( i );
				SAFE_RELEASE( data.m_inputLayouts[ layoutIndex ] );
			}
		}
	}


}
