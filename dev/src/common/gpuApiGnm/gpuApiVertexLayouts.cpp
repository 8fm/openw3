/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "..\gpuApiUtils\gpuApiVertexFormats.h"
#include "..\gpuApiUtils\gpuApiMemory.h"


namespace GpuApi
{
	void AddRef( const VertexLayoutRef &layoutRef )
	{
		GPUAPI_ASSERT( GetDeviceData().m_VertexLayouts.IsInUse(layoutRef) );
		GetDeviceData().m_VertexLayouts.IncRefCount( layoutRef );
	}

	Int32 Release( const VertexLayoutRef &layoutRef )
	{	
		GPUAPI_ASSERT( GetDeviceData().m_VertexLayouts.IsInUse(layoutRef) );

#if 0 // For now, leak it. Fixes a P0. We need to destroy fetch shaders that were made for this vertex layout.
		// Release
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_VertexLayouts.GetRefCount(layoutRef) >= 1 );
		Int32 newRefCount = dd.m_VertexLayouts.DecRefCount( layoutRef );

		// Optionally destroy
		if ( 0 == newRefCount )
		{	
			dd.m_VertexLayouts.Destroy( layoutRef );
		}
#endif

		return 0; //HACK?
	}

	static sce::Gnm::DataFormat PackingTypeToGnmDataFormat( VertexPacking::ePackingType packingType )
	{
		switch ( packingType )
		{
		case VertexPacking::PT_Float1:		return sce::Gnm::kDataFormatR32Float;
		case VertexPacking::PT_Float2:		return sce::Gnm::kDataFormatR32G32Float;
		case VertexPacking::PT_Float3:		return sce::Gnm::kDataFormatR32G32B32Float;
		case VertexPacking::PT_Float4:		return sce::Gnm::kDataFormatR32G32B32A32Float;
		case VertexPacking::PT_Float16_2:	return sce::Gnm::kDataFormatR16G16Float;
		case VertexPacking::PT_Float16_4:	return sce::Gnm::kDataFormatR16G16B16A16Float;
		case VertexPacking::PT_Color:		return sce::Gnm::kDataFormatR8G8B8A8Unorm;
		case VertexPacking::PT_UByte1:		return sce::Gnm::kDataFormatR8Uint;
		case VertexPacking::PT_UByte4:		return sce::Gnm::kDataFormatR8G8B8A8Uint;
		case VertexPacking::PT_UByte4N:		return sce::Gnm::kDataFormatR8G8B8A8Unorm;
		case VertexPacking::PT_Byte4N:		return sce::Gnm::kDataFormatR8G8B8A8Snorm;
		case VertexPacking::PT_UShort1:		return sce::Gnm::kDataFormatR16Uint;
		case VertexPacking::PT_UShort2:		return sce::Gnm::kDataFormatR16G16Uint;
		case VertexPacking::PT_UShort4:		return sce::Gnm::kDataFormatR16G16B16A16Uint;
		case VertexPacking::PT_UShort4N:	return sce::Gnm::kDataFormatR16G16B16A16Unorm;
		case VertexPacking::PT_Short1:		return sce::Gnm::kDataFormatR16Sint;
		case VertexPacking::PT_Short2:		return sce::Gnm::kDataFormatR16G16Sint;
		case VertexPacking::PT_Short4:		return sce::Gnm::kDataFormatR16G16B16A16Sint;
		case VertexPacking::PT_Short4N:		return sce::Gnm::kDataFormatR16G16B16A16Snorm;
		case VertexPacking::PT_UInt1:		return sce::Gnm::kDataFormatR32Uint;
		case VertexPacking::PT_UInt2:		return sce::Gnm::kDataFormatR32G32Uint;
		case VertexPacking::PT_UInt3:		return sce::Gnm::kDataFormatR32G32B32Uint;
		case VertexPacking::PT_UInt4:		return sce::Gnm::kDataFormatR32G32B32A32Uint;
		case VertexPacking::PT_Int1:		return sce::Gnm::kDataFormatR32Sint;
		case VertexPacking::PT_Int2:		return sce::Gnm::kDataFormatR32G32Sint;
		case VertexPacking::PT_Int3:		return sce::Gnm::kDataFormatR32G32B32Sint;
		case VertexPacking::PT_Int4:		return sce::Gnm::kDataFormatR32G32B32A32Sint;
		case VertexPacking::PT_Dec4:		return sce::Gnm::kDataFormatR10G10B10A2Unorm;
		default: GPUAPI_HALT( "Invalid packing element data type"); return sce::Gnm::kDataFormatInvalid;
		}
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

				// To make things a little more efficient, we'll cache the size for each slot as we build the DX element descs. Otherwise
				// figuring out offsets for each D3D element would be either slow or complicated or just annoying.
				// ------------------ GNM NOTE!!! -------------
				// The way gnm buffers are organized, every vertex element is like a seperate slot/stream, so this slot offsets 
				// logic may be completely obsolete
				// ------------------ GNM NOTE!!! \ -----------
				Uint32 cachedSlotOffsets[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = { 0 };

				Uint32 elementCount = 0;

				//! Calculate sizes of element list
				Uint32 streamCount = GetStreamCount( data.m_Desc.m_elements );
				Uint32 elementSizes[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = {};
				CalcElementListSizes( data.m_Desc.m_elements, &(elementSizes[0]), streamCount );

				// Iterate elements and create Gnm buffers descriptors
				for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
				{
					const VertexPacking::PackingElement& element = data.m_Desc.m_elements[i];

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

					SVertexLayoutEntryDesc& decl = data.m_elements[elementCount];

					decl.m_format = PackingTypeToGnmDataFormat( element.m_type );
					decl.m_offset = cachedSlotOffsets[ element.m_slot ];
					decl.m_stride = elementSizes[ element.m_slot ];
					decl.m_slot = element.m_slot;

					cachedSlotOffsets[element.m_slot] += GetPackingTypeSize( element.m_type );
				
					// TODO: It may not be necessary to do in Gnm, so remove it after confirmation
					{
						switch( element.m_slotType )
						{
						case VertexPacking::ST_PerVertex:
							decl.m_slotInstanced = false;
							break;

						case VertexPacking::ST_PerInstance:
							decl.m_slotInstanced = true;
							break;

						default:
							GPUAPI_HALT("Unhandled slot type");
							break;
						}
					}
				

					++elementCount;
				}

				data.m_numElements = elementCount;
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
		RED_UNUSED( setInputLayout );

		SDeviceData &dd = GetDeviceData();
		if ( dd.m_VertexLayout != vertexLayout )
		{
			dd.m_VertexLayout = vertexLayout;
			dd.m_vbChanged = true;
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

		// Nothing to do in GNM, no GNM objects associated per Vertex Layout

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
		default:
			return GetVertexLayoutStride( GetVertexLayoutForChunkType( type ) );
		} 

		GPUAPI_HALT("GetChunkTypeStride() - failed");
		return 0;
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

	Bool MapPackingElementToPSSLSystemSemantic( const VertexPacking::PackingElement& element, sce::Shader::Binary::PsslSemantic& outSemantic )
	{
		switch (element.m_usage)
		{
		case VertexPacking::PS_SysPosition:			outSemantic = sce::Shader::Binary::kSemanticSPosition; break;
		default:
			return false;
		}

		return true;
	}

	Bool ValidateLayout( const SVertexLayoutData& vertexLayout, const sce::Shader::Binary::Program& program )
	{
		// verify that there is a vertex PackingElement for every vertex shader input
		Bool allDataAvailable = true;
		for (Uint32 i = 0; i < program.m_numInputAttributes; ++i)
		{
			sce::Shader::Binary::Attribute* attrib = program.m_inputAttributes + i;
			sce::Shader::Binary::PsslSemantic psslSemantic = (sce::Shader::Binary::PsslSemantic)attrib->m_psslSemantic;
			const char* psslSemanticName = attrib->getSemanticName();
			// An input with system semantic does not need a slot in the remap table; ignore it
			if(psslSemantic >= sce::Shader::Binary::kSemanticSClipDistance && psslSemantic <= sce::Shader::Binary::kSemanticSInsideTessFactor)
				continue;

			// In PSSL all input semantics to Vs/Ls/Es shaders where a fetch shader is required are user defined!
			// Note: this is not the case when using orbis-cgc
			GPUAPI_ASSERT (psslSemantic == sce::Shader::Binary::kSemanticUserDefined);

			bool found = false;
			for ( Uint32 j = 0; j < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++j )
			{
				const VertexPacking::PackingElement& element = vertexLayout.m_Desc.m_elements[j];
				if ( element.IsEmpty() )
					break;

				const char* semantic_name = 0;
				Uint32 semantic_index = 0;
				if (MapPackingElementToSemanticAndIndex(element, semantic_name, semantic_index))
				{
					GPUAPI_ASSERT( psslSemanticName != nullptr );
					GPUAPI_ASSERT( semantic_name != nullptr );
					if (strcmp (psslSemanticName, semantic_name) == 0 &&
						attrib->m_semanticIndex == semantic_index)
					{
						found = true;
						break;
					}
				}
			}

			if (!found)
			{
				GPUAPI_HALT ( "Error: vertex shader input semantic %s%d was not found in the vertex buffer semantic name list", attrib->getSemanticName(), attrib->m_semanticIndex );
				allDataAvailable = false;
			}
		}

		return allDataAvailable;
	}

	Uint32 GenerateSemanticRemapTable( const SVertexLayoutData& vertexLayout, const sce::Shader::Binary::Program& program, Int32* outTable, Uint32 maxElementsInRemapTable, sce::Gnm::FetchShaderInstancingMode* instancingModes )
	{
		// generate remap table
		Uint32 numElementsInRemapTable = 0;
		GPUAPI_ASSERT (maxElementsInRemapTable >= GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS);

		for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
		{
			const VertexPacking::PackingElement& element = vertexLayout.m_Desc.m_elements[i];
			if ( element.IsEmpty() )
				break;

			GPUAPI_ASSERT( i < maxElementsInRemapTable, TXT("Buffer overflow") );

			// Ignore padding elements
			if ( element.m_usage == VertexPacking::PS_Padding )
			{
				continue;
			}

			sce::Shader::Binary::Attribute *inputAttribute = NULL;
			sce::Shader::Binary::PsslSemantic pssl_semantic;

			const char* semantic_name = 0;
			Uint32 semantic_index = 0;
			if (MapPackingElementToPSSLSystemSemantic(element, pssl_semantic))
			{
				inputAttribute = program.getInputAttributeBySemantic (pssl_semantic);
			}
			else if (MapPackingElementToSemanticAndIndex(element, semantic_name, semantic_index))
			{
				inputAttribute = program.getInputAttributeBySemanticNameAndIndex (semantic_name, semantic_index);
			}
			else
			{
				GPUAPI_HALT( "Invalid packing element" );
			}

			if (inputAttribute)
			{
				outTable[numElementsInRemapTable] = inputAttribute->m_resourceIndex;
				*(instancingModes++) = element.m_slotType == VertexPacking::ST_PerInstance ? sce::Gnm::kFetchShaderUseInstanceId : sce::Gnm::kFetchShaderUseVertexIndex;
			}
			else
			{
				outTable[numElementsInRemapTable] = 0xFFFFFFFF; // unused
			}

			numElementsInRemapTable++;
		}

		// if no remapping is required, return 0
		bool remap_required = false;
		for (Int32 i = 0; i < numElementsInRemapTable; ++i)
		{
			remap_required |= outTable[i] != i;
		}

		return remap_required ? numElementsInRemapTable : 0;
	}



	struct FetchShaderInfo
	{
		const SShaderData*						m_shader;
		const SVertexLayoutData*				m_vertexLayout;
		Uint32									m_fetchShaderSize;			// calculate using computeVsFetchShaderSize etc
		const sce::Gnm::VertexInputSemantic*	m_inputSemantics;			// set from shader.m_program
		Uint32									m_numInputSemantics;		// set from shader.m_program
		sce::Gnm::FetchShaderInstancingMode		m_instancing[GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS];

		// these are filled in by the GenerateFetchShaderInfo() function
		SFetchShader*							m_fetchShader;
		Int32*									m_remapTable;
		Uint32									m_numRemapEntries;
		sce::Gnmx::InputResourceOffsets*		m_patchedResourceTable;
	};


	Bool GenerateFetchShaderInfo(FetchShaderInfo& info)
	{
		// clear these outputs before computing them
		info.m_numRemapEntries = 0;
		info.m_remapTable = nullptr;
		info.m_patchedResourceTable = nullptr;

		Bool isLayoutMatchingShader = ValidateLayout( *info.m_vertexLayout, info.m_shader->m_program );
		if ( !isLayoutMatchingShader )
		{
			return false;
		}

		SFetchShader* fetchShader = new SFetchShader();
		fetchShader->m_fetchShaderMemory = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, info.m_fetchShaderSize, sce::Gnm::kAlignmentOfFetchShaderInBytes );
		info.m_fetchShader = fetchShader;

		const Int32 MaxRemapEntries = GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS;
		Int32 remapTable [MaxRemapEntries];
		Int32 numElementsInRemapTable = GenerateSemanticRemapTable( *info.m_vertexLayout, info.m_shader->m_program, remapTable, MaxRemapEntries, &info.m_instancing[0] );

#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		// only patch SROT if remapping is required
		fetchShader->m_resourceTable = nullptr;
		if (numElementsInRemapTable > 0)
		{
			info.m_fetchShader->m_resourceTable = new sce::Gnmx::InputResourceOffsets;

			Int32 patchRet = sce::Gnmx::patchInputResourceOffsetTableWithSemanticTable(
											info.m_fetchShader->m_resourceTable, 
											&info.m_shader->m_resourceOffsets, 
											remapTable, 
											numElementsInRemapTable);
			if (patchRet != 0)
			{
				GPUAPI_HALT ("Failed to patchInputResourceOffsetTableWithSemanticTable: %d", patchRet);
			}
			// From SDK 2.5 it is expected to use this function but this is buggy, so I keep using the deprecated one.
			// TODO: fix or have it fixed
			//sce::Gnmx::generateInputResourceOffsetTable( info.m_fetchShader->m_resourceTable, MapShaderTypeToShaderStage( info.m_shader->m_stageType ), info.m_shader->m_shaderStruct, remapTable,  numElementsInRemapTable );
		}
#else
		if (numElementsInRemapTable > 0)
		{
			info.m_numRemapEntries  = numElementsInRemapTable;

			// create permanent copy of remap table in ONION memory
			info.m_remapTable = (Int32*)GPU_API_ALLOCATE(GpuApi::GpuMemoryPool_Misc, GpuApi::MC_ShaderHeaders, numElementsInRemapTable * sizeof (uint32_t), sce::Gnm::kAlignmentOfBufferInBytes);
			Red::MemoryCopy (info.m_remapTable, remapTable, sizeof (uint32_t) * numElementsInRemapTable);
		}
#endif

		return true;
	}



	SFetchShader* CreateFetchShader( const SVertexLayoutData& vertexLayout, const SShaderData& shaderData, sce::Gnm::ShaderStage shaderStage )
	{
		FetchShaderInfo info;
		info.m_shader = &shaderData;
		info.m_vertexLayout = &vertexLayout;

		switch (shaderStage)
		{
		case sce::Gnm::kShaderStageVs:
			info.m_fetchShaderSize = sce::Gnmx::computeVsFetchShaderSize(shaderData.m_vsShader);
			info.m_numInputSemantics = shaderData.m_vsShader->m_numInputSemantics;
			info.m_inputSemantics = shaderData.m_vsShader->getInputSemanticTable();
			break;
		case sce::Gnm::kShaderStageEs:
			info.m_fetchShaderSize = sce::Gnmx::computeEsFetchShaderSize(shaderData.m_esShader);
			info.m_numInputSemantics = shaderData.m_esShader->m_numInputSemantics;
			info.m_inputSemantics = shaderData.m_esShader->getInputSemanticTable();
			break;
		case sce::Gnm::kShaderStageLs:
			info.m_fetchShaderSize = sce::Gnmx::computeLsFetchShaderSize(shaderData.m_lsShader);
			info.m_numInputSemantics = shaderData.m_esShader->m_numInputSemantics;
			info.m_inputSemantics = shaderData.m_esShader->getInputSemanticTable();
			break;
		default:
			GPUAPI_HALT ("Invalid fetch shader stage %d!", shaderStage);
			return nullptr;
		}

		const Bool fetchShaderInfoGenResult = GenerateFetchShaderInfo(info);
		if ( !fetchShaderInfoGenResult )
		{
			return nullptr;
		}

		switch (shaderStage)
		{
		case sce::Gnm::kShaderStageVs:
			if (info.m_remapTable)
				sce::Gnmx::generateVsFetchShader(info.m_fetchShader->m_fetchShaderMemory, &(info.m_fetchShader->m_shaderModifier), shaderData.m_vsShader, &(info.m_instancing[0]), info.m_remapTable, info.m_numRemapEntries);
			else
				sce::Gnmx::generateVsFetchShader(info.m_fetchShader->m_fetchShaderMemory, &(info.m_fetchShader->m_shaderModifier), shaderData.m_vsShader, &(info.m_instancing[0]));
			break;

		case sce::Gnm::kShaderStageEs:
			if (info.m_remapTable)
				sce::Gnmx::generateEsFetchShader(info.m_fetchShader->m_fetchShaderMemory, &(info.m_fetchShader->m_shaderModifier), shaderData.m_esShader, &(info.m_instancing[0]), info.m_remapTable, info.m_numRemapEntries);
			else
				sce::Gnmx::generateEsFetchShader(info.m_fetchShader->m_fetchShaderMemory, &(info.m_fetchShader->m_shaderModifier), shaderData.m_esShader, &(info.m_instancing[0]));
			break;

		case sce::Gnm::kShaderStageLs:
			if (info.m_remapTable)
				sce::Gnmx::generateLsFetchShader(info.m_fetchShader->m_fetchShaderMemory, &(info.m_fetchShader->m_shaderModifier), shaderData.m_lsShader, &(info.m_instancing[0]), info.m_remapTable, info.m_numRemapEntries);
			else
				sce::Gnmx::generateLsFetchShader(info.m_fetchShader->m_fetchShaderMemory, &(info.m_fetchShader->m_shaderModifier), shaderData.m_lsShader, &(info.m_instancing[0]));
			break;

		default:
			GPUAPI_HALT ("Invalid fetch shader stage %d!", shaderStage);
			return nullptr;
		}


		return info.m_fetchShader;
	}
}
