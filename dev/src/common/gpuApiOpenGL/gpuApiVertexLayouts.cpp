/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "..\gpuApiUtils\gpuApiVertexFormats.h"


namespace GpuApi
{
	static Int32 PackingTypeElementCount( VertexPacking::ePackingType packingType )
	{
		switch ( packingType )
		{
		case VertexPacking::PT_Float1:		return 1;
		case VertexPacking::PT_Float2:		return 2;
		case VertexPacking::PT_Float3:		return 3;
		case VertexPacking::PT_Float4:		return 4;
		case VertexPacking::PT_Float16_2:	return 2;
		case VertexPacking::PT_Float16_4:	return 4;
		case VertexPacking::PT_Color:		return 4;
		case VertexPacking::PT_UByte1:		return 1;
		case VertexPacking::PT_UByte4:		return 4;
		case VertexPacking::PT_UByte4N:		return 4;
		case VertexPacking::PT_UShort1:		return 1;
		case VertexPacking::PT_UShort2:		return 2;
		case VertexPacking::PT_UShort4:		return 4;
		case VertexPacking::PT_Short1:		return 1;
		case VertexPacking::PT_Short2:		return 2;
		case VertexPacking::PT_Short4:		return 4;
		case VertexPacking::PT_Short4N:		return 4;
		case VertexPacking::PT_UInt1:		return 1;
		case VertexPacking::PT_UInt2:		return 2;
		case VertexPacking::PT_UInt3:		return 3;
		case VertexPacking::PT_UInt4:		return 4;
		case VertexPacking::PT_Int1:		return 1;
		case VertexPacking::PT_Int2:		return 2;
		case VertexPacking::PT_Int3:		return 3;
		case VertexPacking::PT_Int4:		return 4;
		default: GPUAPI_HALT( "Invalid packing element data type" ); return -1;
		}
	}

	static GLenum PackingTypeGLType( VertexPacking::ePackingType packingType )
	{
		switch ( packingType )
		{
		case VertexPacking::PT_Float1:		return GL_FLOAT;
		case VertexPacking::PT_Float2:		return GL_FLOAT;
		case VertexPacking::PT_Float3:		return GL_FLOAT;
		case VertexPacking::PT_Float4:		return GL_FLOAT;
		case VertexPacking::PT_Float16_2:	return GL_HALF_FLOAT;
		case VertexPacking::PT_Float16_4:	return GL_HALF_FLOAT;
		case VertexPacking::PT_Color:		return GL_UNSIGNED_BYTE;
		case VertexPacking::PT_UByte1:		return GL_UNSIGNED_BYTE;
		case VertexPacking::PT_UByte4:		return GL_UNSIGNED_BYTE;
		case VertexPacking::PT_UByte4N:		return GL_UNSIGNED_BYTE;
		case VertexPacking::PT_UShort1:		return GL_UNSIGNED_SHORT;
		case VertexPacking::PT_UShort2:		return GL_UNSIGNED_SHORT;
		case VertexPacking::PT_UShort4:		return GL_UNSIGNED_SHORT;
		case VertexPacking::PT_Short1:		return GL_SHORT;
		case VertexPacking::PT_Short2:		return GL_SHORT;
		case VertexPacking::PT_Short4:		return GL_SHORT;
		case VertexPacking::PT_Short4N:		return GL_SHORT;
		case VertexPacking::PT_UInt1:		return GL_UNSIGNED_INT;
		case VertexPacking::PT_UInt2:		return GL_UNSIGNED_INT;
		case VertexPacking::PT_UInt3:		return GL_UNSIGNED_INT;
		case VertexPacking::PT_UInt4:		return GL_UNSIGNED_INT;
		case VertexPacking::PT_Int1:		return GL_INT;
		case VertexPacking::PT_Int2:		return GL_INT;
		case VertexPacking::PT_Int3:		return GL_INT;
		case VertexPacking::PT_Int4:		return GL_INT;
		default: GPUAPI_HALT( "Invalid packing element data type" ); return -1;
		}
	}

	static GLboolean PackingTypeIsNormalized( VertexPacking::ePackingType packingType )
	{
		switch ( packingType )
		{
		case VertexPacking::PT_Float1:		
		case VertexPacking::PT_Float2:		
		case VertexPacking::PT_Float3:		
		case VertexPacking::PT_Float4:		
		case VertexPacking::PT_Float16_2:	
		case VertexPacking::PT_Float16_4:	
		case VertexPacking::PT_Short1:		
		case VertexPacking::PT_Short2:		
		case VertexPacking::PT_Short4:		
		case VertexPacking::PT_Short4N:		
		case VertexPacking::PT_Int1:		
		case VertexPacking::PT_Int2:		
		case VertexPacking::PT_Int3:		
		case VertexPacking::PT_Int4:		
			return GL_FALSE;
		case VertexPacking::PT_Color:		
		case VertexPacking::PT_UByte1:		
		case VertexPacking::PT_UByte4:		
		case VertexPacking::PT_UByte4N:		
		case VertexPacking::PT_UShort1:		
		case VertexPacking::PT_UShort2:		
		case VertexPacking::PT_UShort4:		
		case VertexPacking::PT_UInt1:		
		case VertexPacking::PT_UInt2:		
		case VertexPacking::PT_UInt3:		
		case VertexPacking::PT_UInt4:		
			return GL_TRUE;
		default: GPUAPI_HALT( "Invalid packing element data type" ); return GL_FALSE;
		}
	}

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
		Int32 newRefCount = dd.m_VertexLayouts.DecRefCount( layoutRef );

		// Optionally destroy
		if ( 0 == newRefCount )
		{	
//			SVertexLayoutData &data = dd.m_VertexLayouts.Data( layoutRef );
//			for ( Uint32 i = 0; i < MAX_SHADER_COUNT; ++i )
//			{
//				if ( data.m_inputLayouts[i] )
//				{
//					ULONG refcount = data.m_inputLayouts[i]->Release();
//#ifdef NO_GPU_ASSERTS
//					RED_UNUSED( refcount );
//#endif
//					GPUAPI_ASSERT( refcount == 0, TXT( "D3DInputLayout refcount > 0, object won't be destroyed" ) );
//				}
//			}
			dd.m_VertexLayouts.Destroy( layoutRef );
		}

		return 0;
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
			Uint32 usageUsagesIdx = element.m_usage * GPUAPI_VERTEX_LAYOUT_USAGE_INDEX_COUNT + element.m_usageIndex;
			if ( usedUsages[usageUsagesIdx] )
			{
				return false;
			}
			usedUsages[usageUsagesIdx] = true;
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
			if( setInputLayout )
			{
				const VertexLayoutDesc& layoutDesc = dd.m_VertexLayouts.Data( vertexLayout ).m_Desc;
				Uint32 elementOffset = 0;
				Uint8 lastSlot = 0;
				for ( Uint32 i = 0; i < layoutDesc.m_elementCount; ++i )
				{
					const VertexPacking::PackingElement& element = layoutDesc.m_elements[i];

					// reset the offset every time we have a new slot
					if ( lastSlot != element.m_slot )
					{
						elementOffset = 0;
					}

					glEnableVertexAttribArray(i);
					glVertexAttribFormat( 
						i,												// attribute index
						PackingTypeElementCount(element.m_type),		// size
						PackingTypeGLType(element.m_type),				// type
						PackingTypeIsNormalized(element.m_type),		// normalized?
						elementOffset									// array buffer offset
						);
					glVertexAttribBinding( i, element.m_slot );
					glVertexBindingDivisor( element.m_slot, element.m_slotType == VertexPacking::ST_PerInstance ? 1 : 0 );
					elementOffset += GetPackingTypeSize( element.m_type );
					lastSlot = element.m_slot;
				}
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
		for ( Uint32 i = 0; i < BCT_Max; ++i )
		{
			// The index buffer formats don't count...
			if ( i == BCT_IndexUInt || i == BCT_IndexUShort ) continue;
		
			SDeviceData &dd = GetDeviceData();
			GPUAPI_ASSERT( !dd.m_ChunkTypeVertexLayouts[i].isNull() );
		}
	}

	void ShutVertexDeclarations( bool dropRefs )
	{
		SDeviceData &dd = GetDeviceData();

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

	void ReleaseInputLayouts( const ShaderRef& shader )
	{
		//if ( shader.isNull() )
		//{
		//	return;
		//}

		//Int32 layoutIndex = ShaderRefToIndex( shader );
		//GPUAPI_ASSERT( layoutIndex >= 0 && layoutIndex < MAX_SHADER_COUNT );
		//if ( layoutIndex < 0 || layoutIndex >= MAX_SHADER_COUNT )
		//{
		//	return;
		//}

		//SDeviceData &dd = GetDeviceData();
		//for ( Uint32 i = 0; i < dd.m_VertexLayouts._MaxResCount; ++i )
		//{
		//	if ( dd.m_VertexLayouts.IsInUse( i ) )
		//	{
		//		SVertexLayoutData& data = dd.m_VertexLayouts.Data( i );
		//		SAFE_RELEASE( data.m_inputLayouts[ layoutIndex ] );
		//	}
		//}
	}


}
