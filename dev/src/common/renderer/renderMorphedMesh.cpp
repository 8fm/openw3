#include "build.h"
#include "renderMorphedMesh.h"
#include "renderHelpers.h"
#include "renderShaderPair.h"
#include "renderTexture.h"
#include "../engine/morphedMeshComponent.h"
#include "../engine/mesh.h"
#include "../engine/material.h"
#include "../engine/bitmapTexture.h"

static void GetCombinedQuantization( const CRenderMesh& mesh1, const CRenderMesh& mesh2, Vector& outScale, Vector& outOffset )
{
	Vector scale1, offset1;
	mesh1.GetQuantization( &scale1, &offset1 );

	Vector scale2, offset2;
	mesh2.GetQuantization( &scale2, &offset2 );


	Box morphBox( Box::RESET_STATE );
	morphBox.AddPoint( offset1 );
	morphBox.AddPoint( scale1 + offset1 );

	morphBox.AddPoint( offset2 );
	morphBox.AddPoint( scale2 + offset2 );

	outScale = morphBox.Max - morphBox.Min;
	outOffset = morphBox.Min;

	outScale.W = 1;
	outOffset.W = 0;
}

// Get the SO Shader and input layout that we should use to generate morphed vertices for a given chunk.
static Bool GetSOShaderAndInputLayoutForChunk( const CRenderMesh::Chunk& renderChunk, Bool canUseControlTexture, CRenderShaderStreamOut** outShader, GpuApi::VertexLayoutRef* outLayout )
{
	switch ( renderChunk.m_chunkVertices.type )
	{
	case GpuApi::BCT_VertexMeshStaticSimple:
	case GpuApi::BCT_VertexMeshStaticExtended:
		if ( canUseControlTexture )
		{
			if ( outLayout != nullptr )
			{
				*outLayout = GpuApi::GetVertexLayoutForChunkType( GpuApi::BCT_VertexMeshStaticControlMorphGenIn );
			}
			if ( outShader != nullptr )
			{
				*outShader = GetRenderer()->m_shaderMorphGen_Static_ControlTex;
			}
		}
		else
		{
			if ( outLayout != nullptr )
			{
				*outLayout = GpuApi::GetVertexLayoutForChunkType( GpuApi::BCT_VertexMeshStaticMorphGenIn );
			}
			if ( outShader != nullptr )
			{
				*outShader = GetRenderer()->m_shaderMorphGen_Static;
			}
		}
		break;

	case GpuApi::BCT_VertexMeshSkinnedSimple:
	case GpuApi::BCT_VertexMeshSkinnedExtended:
		if ( canUseControlTexture )
		{
			if ( outLayout != nullptr )
			{
				*outLayout = GpuApi::GetVertexLayoutForChunkType( GpuApi::BCT_VertexMeshSkinnedControlMorphGenIn );
			}
			if ( outShader != nullptr )
			{
				*outShader = GetRenderer()->m_shaderMorphGen_Skinned_ControlTex;
			}
		}
		else
		{
			if ( outLayout != nullptr )
			{
				*outLayout = GpuApi::GetVertexLayoutForChunkType( GpuApi::BCT_VertexMeshSkinnedMorphGenIn );
			}
			if ( outShader != nullptr )
			{
				*outShader = GetRenderer()->m_shaderMorphGen_Skinned;
			}
		}
		break;

	case GpuApi::BCT_VertexMeshStaticPositionOnly:
		// Fail silently. We can't create a morphed chunk for this, but it's not really an error.
		// TODO : Support this? We could morph it uniformly, but a control texture won't work (no UVs).
		return false;

	default:
		RED_HALT( "Unsupported vertex factory for morphed mesh: %d", renderChunk.m_vertexFactory );
		return false;
	}

	return true;
}

// Get the SO Shader and input layout that we should use to generate morphed vertices for a given chunk.
static Bool GetCSShaderAndStrideForChunk( const CRenderMesh::Chunk& renderChunk, Bool canUseControlTexture, CRenderShaderCompute** outShader, Uint32& outStride )
{
	switch ( renderChunk.m_chunkVertices.type )
	{
	case GpuApi::BCT_VertexMeshStaticSimple:
	case GpuApi::BCT_VertexMeshStaticExtended:
		outStride = 16;
		if ( canUseControlTexture )
		{
			if ( outShader != nullptr )
			{
				*outShader = GetRenderer()->m_shaderMorphGen_Static_ControlTexCS;
			}
		}
		else
		{
			if ( outShader != nullptr )
			{
				*outShader = GetRenderer()->m_shaderMorphGen_StaticCS;
			}
		}
		break;

	case GpuApi::BCT_VertexMeshSkinnedSimple:
	case GpuApi::BCT_VertexMeshSkinnedExtended:
		outStride = 24;
		if ( canUseControlTexture )
		{
			if ( outShader != nullptr )
			{
				*outShader = GetRenderer()->m_shaderMorphGen_Skinned_ControlTexCS;
			}
		}
		else
		{
			if ( outShader != nullptr )
			{
				*outShader = GetRenderer()->m_shaderMorphGen_SkinnedCS;
			}
		}
		break;

	case GpuApi::BCT_VertexMeshStaticPositionOnly:
		// Fail silently. We can't create a morphed chunk for this, but it's not really an error.
		// TODO : Support this? We could morph it uniformly, but a control texture won't work (no UVs).
		return false;

	default:
		RED_HALT( "Unsupported vertex factory for morphed mesh: %d", renderChunk.m_vertexFactory );
		return false;
	}

	return true;
}

static GpuApi::VertexLayoutRef CreateModifiedCombinedVertexLayout( GpuApi::VertexLayoutRef sourceMeshLayout, Uint32& outTangentFrameOffset )
{
	RED_ASSERT( !sourceMeshLayout.isNull(), TXT("Cannot create morph vertex layout from null source layout") );
	if ( sourceMeshLayout.isNull() )
	{
		return GpuApi::VertexLayoutRef::Null();
	}

	const GpuApi::VertexLayoutDesc* originalDesc = GpuApi::GetVertexLayoutDesc( sourceMeshLayout );
	RED_ASSERT( originalDesc != nullptr, TXT("No vertex layout for source chunk") );
	if ( originalDesc == nullptr )
	{
		return GpuApi::VertexLayoutRef::Null();
	}

	// Adjust vertex layout, since positions and tangent frames are now interleaved in a single buffer. We modify the layout so that
	// it still treats the positions and tangent frames as separate input slots, but with padding between consecutive vertices.
	GpuApi::VertexLayoutDesc alteredDesc = *originalDesc;

	const Uint8 posIndex = (Uint8)RenderMeshStreamToIndex( RMS_PositionSkinning );
	const Uint8 tanIndex = (Uint8)RenderMeshStreamToIndex( RMS_TangentFrame );


	// Add padding to end of each stream, where the other stream's data is.
	GpuApi::VertexPacking::PackingElement paddingElements[GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS];
	Uint32 numPadding = 0;
	outTangentFrameOffset = 0;

	for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
	{
		if ( originalDesc->m_elements[i].IsEmpty() )
		{
			break;
		}

		// Add padding to tangent frame stream
		if ( originalDesc->m_elements[i].m_slot == posIndex )
		{
			paddingElements[numPadding] = originalDesc->m_elements[i];
			paddingElements[numPadding].m_slot = tanIndex;
			paddingElements[numPadding].m_usage = GpuApi::VertexPacking::PS_Padding;
			++numPadding;
			outTangentFrameOffset += GpuApi::VertexPacking::GetPackingTypeSize( originalDesc->m_elements[i].m_type );
		}
		// Add padding to position stream
		if ( originalDesc->m_elements[i].m_slot == tanIndex )
		{
			paddingElements[numPadding] = originalDesc->m_elements[i];
			paddingElements[numPadding].m_slot = posIndex;
			paddingElements[numPadding].m_usage = GpuApi::VertexPacking::PS_Padding;
			++numPadding;
		}
	}
	paddingElements[numPadding] = GpuApi::VertexPacking::PackingElement::END_OF_ELEMENTS;

	alteredDesc.AddElements( paddingElements );

	GpuApi::VertexLayoutRef layout = GpuApi::CreateVertexLayout( alteredDesc );

	return layout;
}

// Generate a new vertex layout suitable for rendering a target chunk with the source mesh's UVs for UV dissolve. Provided layout
// should be the one returned by CreateModifiedCombinedVertexLayout for the corresponding source chunk.
static GpuApi::VertexLayoutRef CreateModifiedTargetVertexLayout( GpuApi::VertexLayoutRef morphSourceLayout )
{
	RED_ASSERT( !morphSourceLayout.isNull(), TXT("Cannot create target vertex layout from null source layout") );
	if ( morphSourceLayout.isNull() )
	{
		return GpuApi::VertexLayoutRef::Null();
	}

	const GpuApi::VertexLayoutDesc* originalLayoutDesc = GpuApi::GetVertexLayoutDesc( morphSourceLayout );
	RED_ASSERT( originalLayoutDesc != nullptr, TXT("Morph source layout has null layout desc?") );
	if ( originalLayoutDesc == nullptr )
	{
		return GpuApi::VertexLayoutRef::Null();
	}

	// Mostly use the original vertex layout.
	GpuApi::VertexLayoutDesc modifiedLayoutDesc = *originalLayoutDesc;

	Uint8 customSlot = ( Uint8 )RenderMeshStreamToIndex( RMS_Custom0 );

	GpuApi::VertexPacking::PackingElement texCoord2[] =
	{
		{ GpuApi::VertexPacking::PT_Float16_2, GpuApi::VertexPacking::PS_TexCoord, 2, customSlot, GpuApi::VertexPacking::ST_PerVertex },
		GpuApi::VertexPacking::PackingElement::END_OF_ELEMENTS
	};

	modifiedLayoutDesc.AddElements( texCoord2 );

	return GpuApi::CreateVertexLayout( modifiedLayoutDesc );
}


//////////////////////////////////////////////////////////////////////////


CRenderMorphedMesh::AdditionalChunkData::AdditionalChunkData()
	: m_material( nullptr )
	, m_materialParams( nullptr )
	, m_flags( 0 )
	, m_sourceChunk( -1 )
{
}

CRenderMorphedMesh::AdditionalChunkData::AdditionalChunkData( IMaterial* material, GpuApi::VertexLayoutRef vertexLayout, Int32 flags, Int8 sourceChunk )
	: m_material( nullptr )
	, m_materialParams( nullptr )
	, m_vertexLayout( vertexLayout )
	, m_flags( flags )
	, m_sourceChunk( sourceChunk )
{
	if ( material != nullptr )
	{
		ExtractRenderResource( material->GetMaterialDefinition(), m_material );
		ExtractRenderResource( material, m_materialParams );
	}

	RED_ASSERT( m_vertexLayout, TXT("Must provide vertex layout to AdditionalChunkData") );
	GpuApi::AddRefIfValid( m_vertexLayout );
}

CRenderMorphedMesh::AdditionalChunkData::~AdditionalChunkData()
{
	GpuApi::SafeRelease( m_vertexLayout );
	SAFE_RELEASE( m_material );
	SAFE_RELEASE( m_materialParams );
}

CRenderMorphedMesh::AdditionalChunkData::AdditionalChunkData( const AdditionalChunkData& other )
	: m_material( other.m_material )
	, m_materialParams( other.m_materialParams )
	, m_flags( other.m_flags )
	, m_sourceChunk( other.m_sourceChunk )
{
	if ( m_material != nullptr )
	{
		m_material->AddRef();
	}
	if ( m_materialParams != nullptr )
	{
		m_materialParams->AddRef();
	}

	GpuApi::SafeRefCountAssign( m_vertexLayout, other.m_vertexLayout );
}

CRenderMorphedMesh::AdditionalChunkData::AdditionalChunkData( AdditionalChunkData&& other )
	: m_material( other.m_material )
	, m_materialParams( other.m_materialParams )
	, m_vertexLayout( other.m_vertexLayout )
	, m_flags( other.m_flags )
	, m_sourceChunk( other.m_sourceChunk )
{
	other.m_material		= nullptr;
	other.m_materialParams	= nullptr;
	other.m_vertexLayout	= GpuApi::VertexLayoutRef::Null();
	other.m_flags			= 0;
	other.m_sourceChunk		= -1;
}

CRenderMorphedMesh::AdditionalChunkData& CRenderMorphedMesh::AdditionalChunkData::operator =( const AdditionalChunkData& other )
{
	if ( this == &other ) { return *this; }

	SAFE_COPY( m_material, other.m_material );
	SAFE_COPY( m_materialParams, other.m_materialParams );
	GpuApi::SafeRefCountAssign( m_vertexLayout, other.m_vertexLayout );
	m_flags = other.m_flags;
	m_sourceChunk = other.m_sourceChunk;

	return *this;
}

CRenderMorphedMesh::AdditionalChunkData& CRenderMorphedMesh::AdditionalChunkData::operator =( AdditionalChunkData&& other )
{
	if ( this == &other ) { return *this; }

	SAFE_RELEASE( m_material );
	m_material				= other.m_material;
	other.m_material		= nullptr;

	SAFE_RELEASE( m_materialParams );
	m_materialParams		= other.m_materialParams;
	other.m_materialParams	= nullptr;

	GpuApi::SafeRelease( m_vertexLayout );
	m_vertexLayout			= other.m_vertexLayout;
	other.m_vertexLayout	= GpuApi::VertexLayoutRef::Null();

	m_flags					= other.m_flags;
	m_sourceChunk			= other.m_sourceChunk;
	other.m_flags			= 0;
	other.m_sourceChunk		= -1;

	return *this;
}


//////////////////////////////////////////////////////////////////////////


CRenderMorphedMesh::CRenderMorphedMesh()
	: m_sourceMesh( nullptr )
	, m_targetMesh( nullptr )
	, m_useControlTexForMorph( false )
{
	// We aren't ready until we've generated the morphed buffer at least once.
	m_buffersPendingLoad.SetValue( 1 );
}

CRenderMorphedMesh::~CRenderMorphedMesh()
{
	SAFE_RELEASE( m_sourceMesh );
	SAFE_RELEASE( m_targetMesh );

	for ( auto& tex : m_controlTextures )
	{
		SAFE_RELEASE( tex );
	}
}


CRenderMorphedMesh* CRenderMorphedMesh::Create( const CMorphedMeshComponent* component, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderMorphedMesh > morphedMesh ( partialRegistrationHash );

	if ( component == nullptr )
	{
		RED_HALT( "Trying to create morphed mesh with null component" );
		return nullptr;
	}

	// If we don't have either meshes, we can't really do anything further.
	CMesh* morphSource = component->GetMorphSource();
	CMesh* morphTarget = component->GetMorphTarget();

	// In the event of only one mesh being set, rather than failing entirely, we'll just use the one mesh for both source and target.
	if ( morphSource == nullptr )
	{
		morphSource = morphTarget;
	}
	if ( morphTarget == nullptr )
	{
		morphTarget = morphSource;
	}

	// If neither is set (after sharing in case only one is set), we can't do anything.
	if ( morphSource == nullptr || morphTarget == nullptr )
	{
		return nullptr;
	}


	CRenderMesh* sourceRenderMesh = static_cast< CRenderMesh* >( morphSource->GetRenderResource() );
	CRenderMesh* targetRenderMesh = static_cast< CRenderMesh* >( morphTarget->GetRenderResource() );
	if ( sourceRenderMesh == nullptr || targetRenderMesh == nullptr )
	{
		return nullptr;
	}

	// Source and target must have the same number of chunks.
	if ( sourceRenderMesh->GetNumChunks() != targetRenderMesh->GetNumChunks() )
	{
		ERR_RENDERER( TXT("Source and target mesh chunk counts don't match! Source: %u, Target: %u"), sourceRenderMesh->GetNumChunks(), targetRenderMesh->GetNumChunks() );
		return nullptr;
	}
	// Also the same number of LODs.
	if ( sourceRenderMesh->GetNumLODs() != targetRenderMesh->GetNumLODs() )
	{
		ERR_RENDERER( TXT("Source and target mesh LOD counts don't match! Source: %u, Target: %u"), sourceRenderMesh->GetNumLODs(), targetRenderMesh->GetNumLODs() );
		return nullptr;
	}

	morphedMesh.InitResource( new CRenderMorphedMesh );
	
	SAFE_COPY( morphedMesh->m_sourceMesh, sourceRenderMesh );
	SAFE_COPY( morphedMesh->m_targetMesh, targetRenderMesh );


	// Source and target have quantized positions, but may have different quantization parameters. Do deal with this, we extract
	// the original bounding boxes from the quantization parameters of each mesh, find their union, and use that as our own bounds.
	// Then we can have our positions quantized as well.
	GetCombinedQuantization( *sourceRenderMesh, *targetRenderMesh, morphedMesh->m_quantizationScale, morphedMesh->m_quantizationOffset );


	// Copy over LOD distances. Just use the Min of source and target.
	const Uint32 numLODs = sourceRenderMesh->GetNumLODs();
	morphedMesh->m_lods.Resize( numLODs );
	for ( Uint32 i = 0; i < numLODs; ++i )
	{
		morphedMesh->m_lods[ i ].m_distance = Min( sourceRenderMesh->m_lods[ i ].m_distance, targetRenderMesh->m_lods[ i ].m_distance );
	}


	const Uint32 numMaterials	= morphSource->GetMaterials().Size();
	const Uint32 numChunks		= sourceRenderMesh->GetNumChunks();
	const auto& sourceChunks	= sourceRenderMesh->GetChunks();
	const auto& targetChunks	= targetRenderMesh->GetChunks();


	// Get the control textures. There should be one for each material.
	morphedMesh->m_useControlTexForMorph = component->UseControlTexturesForMorph();
	morphedMesh->m_controlTextures.Resize( numMaterials );
	for ( Uint32 i = 0; i < numMaterials; ++i )
	{
		ExtractRenderResource( component->GetMorphControlTexture( i ), morphedMesh->m_controlTextures[i] );
	}


	// Create the vertex buffer to hold the interpolated data.
	Uint32 interpolatedDataSize = 0;
	for ( Uint32 i = 0; i < numChunks; ++i )
	{
		const Chunk& sourceChunk = sourceChunks[ i ];
		const Chunk& targetChunk = targetChunks[ i ];
#ifdef RED_PLATFORM_ORBIS
		CRenderShaderCompute* csShader = nullptr;
		Uint32 stride = 0;
		if ( GetCSShaderAndStrideForChunk( sourceChunk, morphedMesh->m_useControlTexForMorph, &csShader, stride ) )
		{
			interpolatedDataSize += stride * sourceChunk.m_numVertices;
		}
#else
		CRenderShaderStreamOut* soShader = nullptr;
		if ( GetSOShaderAndInputLayoutForChunk( sourceChunk, morphedMesh->m_useControlTexForMorph, &soShader, nullptr ) )
		{
			Uint32 stride = GpuApi::GetChunkTypeStride( soShader->GetOutputChunkType(), 0 );
			interpolatedDataSize += stride * sourceChunk.m_numVertices;
		}
#endif
	}
#ifdef RED_PLATFORM_ORBIS
	GpuApi::BufferInitData initData;
	initData.m_elementCount = interpolatedDataSize / 4;
	morphedMesh->m_vertices = GpuApi::CreateBuffer( interpolatedDataSize, GpuApi::BCC_Raw, GpuApi::BUT_Default, 0, &initData );
#else
	morphedMesh->m_vertices = GpuApi::CreateBuffer( interpolatedDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_StreamOut, 0 );
#endif


	// Create a blend material instance for each source material. We assume that there's a one-to-one mapping between source and target
	// materials, so we can just create a single blend material instance for each source material, using whichever matching target material
	// we find first. CMorphedMeshComponent should check for problems with this and give data asserts when it fails.
	TDynArray< IMaterial* > blendMaterials( numMaterials );
	for ( Uint32 i = 0; i < numChunks; ++i )
	{
		const Uint8 sourceMtlId = sourceChunks[ i ].m_materialId;
		const Uint8 targetMtlId = targetChunks[ i ].m_materialId;

		if ( blendMaterials[ sourceMtlId ] == nullptr )
		{
			blendMaterials[ sourceMtlId ] = component->CreateBlendMaterialInstance( sourceMtlId, targetMtlId );
		}
	}


	// Set up chunks
	const Uint32 posStreamIndex = RenderMeshStreamToIndex( RMS_PositionSkinning );
	const Uint32 tanStreamIndex = RenderMeshStreamToIndex( RMS_TangentFrame );
	const Uint32 texStreamIndex = RenderMeshStreamToIndex( RMS_TexCoords );
	const Uint32 customStreamIndex = RenderMeshStreamToIndex( RMS_Custom0 );
	Uint32 offsetInInterpolatedData = 0;
	for ( Uint32 i = 0; i < numChunks; ++i )
	{
		const Chunk& sourceChunk = sourceChunks[ i ];
		const Chunk& targetChunk = targetChunks[ i ];

		if ( sourceChunk.m_numVertices != targetChunk.m_numVertices || sourceChunk.m_numIndices != targetChunk.m_numIndices || sourceChunk.m_vertexFactory != targetChunk.m_vertexFactory )
		{
			ERR_RENDERER( TXT("Chunk %u in source mesh in incompatible with same chunk in target mesh"), i );
			continue;
		}

		IMaterial* blendMaterial = blendMaterials[ sourceChunk.m_materialId ];


		//
		// Set up new chunk corresponding to the source mesh.
		// If we're using a blended material, this will be the only chunk we add (don't need to add for target mesh).
		//
		Uint32 tangentOffset = 0;
		GpuApi::VertexLayoutRef sourceLayout = CreateModifiedCombinedVertexLayout( sourceRenderMesh->GetChunkVertexLayout( i ), tangentOffset );
		if ( sourceLayout.isNull() )
		{
			ERR_RENDERER( TXT("Couldn't create vertex layout for source chunk %u"), i );
			continue;
		}

		if ( blendMaterial != nullptr )
		{
			morphedMesh->m_chunkData.PushBack( AdditionalChunkData( blendMaterial, sourceLayout, RMMCF_BlendedMaterial, i ) );
		}
		else
		{
			morphedMesh->m_chunkData.PushBack( AdditionalChunkData( morphSource->GetMaterials()[ sourceChunk.m_materialId ], sourceLayout, 0, i ) );
		}

		// Set up offsets for interpolated streams
		Chunk newSourceChunk = sourceChunk;

		// Position/skinning data is first in new buffer. Tangent frame comes after. We could have added the tangent padding
		// to the start of the vertex layout, and then the offset would be the same, but this is simpler.
		newSourceChunk.m_chunkVertices.byteOffsets[ posStreamIndex ] = offsetInInterpolatedData;
		newSourceChunk.m_chunkVertices.byteOffsets[ tanStreamIndex ] = offsetInInterpolatedData + tangentOffset;

		morphedMesh->m_chunks.PushBack( newSourceChunk );


		//
		// If we aren't using a blended material, also set up a chunk for the target mesh.
		// This chunk is similar to the source, except that it also needs to use the source mesh's UVs in order for the
		// UV dissolve to work.
		//
		if ( blendMaterial == nullptr )
		{
			GpuApi::VertexLayoutRef targetLayout = CreateModifiedTargetVertexLayout( sourceLayout );
			if ( targetLayout.isNull() )
			{
				ERR_RENDERER( TXT("Couldn't create vertex layout for target chunk %u"), i );
				continue;
			}

			morphedMesh->m_chunkData.PushBack( AdditionalChunkData( morphTarget->GetMaterials()[ targetChunk.m_materialId ], targetLayout, RMMCF_FromTargetMesh, i ) );

			GpuApi::SafeRelease( targetLayout );

			Chunk newTargetChunk = targetChunk;

			// Position/skinning data is first in new buffer. Tangent frame comes after. We could have added the tangent padding
			// to the start of the vertex layout, and then the offset would be the same, but this is simpler.
			newTargetChunk.m_chunkVertices.byteOffsets[ posStreamIndex ] = offsetInInterpolatedData;
			newTargetChunk.m_chunkVertices.byteOffsets[ tanStreamIndex ] = offsetInInterpolatedData + tangentOffset;
			newTargetChunk.m_chunkVertices.byteOffsets[ customStreamIndex ] = newSourceChunk.m_chunkVertices.byteOffsets[ texStreamIndex ];

			morphedMesh->m_chunks.PushBack( newTargetChunk );
		}


		// Position and Tangent streams have the same stride.
		offsetInInterpolatedData += newSourceChunk.m_numVertices * GpuApi::GetVertexLayoutStride( sourceLayout, posStreamIndex );

		GpuApi::SafeRelease( sourceLayout );
	}
	RED_FATAL_ASSERT( morphedMesh->m_chunkData.Size() == morphedMesh->m_chunks.Size(), "Mismatch between chunks size and chunk data size" );


	// Now we can discard any blend materials we created.
	for ( Uint32 i = 0; i < blendMaterials.Size(); ++i )
	{
		if ( blendMaterials[i] != nullptr )
		{
			blendMaterials[i]->Discard();
		}
	}
	blendMaterials.ClearFast();


	// Use directly from the source mesh.
	GpuApi::SafeRefCountAssign( morphedMesh->m_indices, sourceRenderMesh->m_indices );
	GpuApi::SafeRefCountAssign( morphedMesh->m_bonePositions, sourceRenderMesh->m_bonePositions );

	morphedMesh->m_isTwoSided = sourceRenderMesh->m_isTwoSided || targetRenderMesh->m_isTwoSided;

	return morphedMesh.RetrieveSuccessfullyCreated();
}


void CRenderMorphedMesh::GetChunkGeometry( Uint8 chunkIndex, ChunkGeometry& outGeometry ) const
{
	RED_FATAL_ASSERT( chunkIndex < m_chunks.Size(), "Chunk index out of bounds! Caller is responsible for this!" );

	const Chunk& chunk						= m_chunks[ chunkIndex ];
	const AdditionalChunkData& chunkData	= m_chunkData[ chunkIndex ];

	const GpuApi::BufferRef myVertices		= m_vertices;
	const GpuApi::BufferRef sourceVertices	= m_sourceMesh->m_vertices;
	const GpuApi::BufferRef targetVertices	= m_targetMesh->m_vertices;

	// The same interpolated streams are used for all chunks
	outGeometry.m_vertexBuffers[ 0 ]		= myVertices;				//RMS_PositionSkinning
	outGeometry.m_vertexBuffers[ 2 ]		= myVertices;				//RMS_TangentFrame

	// Get correct vertex buffers, depending on whether the chunk belongs to the source or target mesh.
	if ( ( chunkData.m_flags & RMMCF_FromTargetMesh ) != 0 )
	{
		outGeometry.m_vertexBuffers[ 1 ]	= targetVertices;			//RMS_TexCoords
		outGeometry.m_vertexBuffers[ 3 ]	= targetVertices;			//RMS_Extended
		outGeometry.m_vertexBuffers[ 4 ]	= sourceVertices;			//RMS_Custom0
	}
	else
	{
		outGeometry.m_vertexBuffers[ 1 ]	= sourceVertices;			//RMS_TexCoords
		outGeometry.m_vertexBuffers[ 3 ]	= sourceVertices;			//RMS_Extended
		outGeometry.m_vertexBuffers[ 4 ]	= GpuApi::BufferRef();		//RMS_Custom0
	}
	static_assert( RENDER_MESH_NUM_STREAMS == 5, "# of render mesh streams changed. Probably need to update this?" );

	// Vertex layout
	outGeometry.m_vertexLayout = chunkData.m_vertexLayout;

	// Nothing special about the vertex offsets or index buffer stuff.
	Red::System::MemoryCopy( outGeometry.m_vertexOffsets, chunk.m_chunkVertices.byteOffsets, sizeof( outGeometry.m_vertexOffsets ) );

	outGeometry.m_indexBuffer = m_indices;
	outGeometry.m_indexOffset = chunk.m_chunkIndices.byteOffset;
}

GpuApi::VertexLayoutRef CRenderMorphedMesh::GetChunkVertexLayout( Uint8 chunkIndex ) const
{
	if ( chunkIndex >= m_chunks.Size() )
	{
		RED_HALT( "Mesh chunk index out of range. %d >= %d", chunkIndex, m_chunks.Size() );
		return GpuApi::VertexLayoutRef::Null();
	}

	const AdditionalChunkData& chunkData = m_chunkData[ chunkIndex ];
	return chunkData.m_vertexLayout;
}

static Uint32 AlignXYZ( Uint32 value, Uint32 alignment ) { return ( value + (alignment - 1) ) & ~(alignment - 1); }

Bool CRenderMorphedMesh::ApplyMorphRatio( Float ratio )
{
	PC_SCOPE_RENDER_LVL1( MorphedMesh_ApplyMorphRatio );

	// Not loaded yet, can't do anything
	if ( !m_sourceMesh->IsFullyLoaded() || !m_targetMesh->IsFullyLoaded() )
	{
		return false;
	}

	// If we haven't gotten an index buffer yet, get it now. It may not have been loaded when we first created the
	// morphed mesh.
	if ( m_indices.isNull() )
	{
		GpuApi::SafeRefCountAssign( m_indices, m_sourceMesh->m_indices );
	}
	// Likewise for the bone positions texture.
	if ( m_bonePositions.isNull() )
	{
		GpuApi::SafeRefCountAssign( m_bonePositions, m_sourceMesh->m_bonePositions );
	}

	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_VertexShader );
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );

	// Set quantization parameters for both meshes.
	Vector sourceQS, sourceQO, targetQS, targetQO;
	m_sourceMesh->GetQuantization( &sourceQS, &sourceQO );
	m_targetMesh->GetQuantization( &targetQS, &targetQO );

#ifdef RED_PLATFORM_ORBIS
	SMorphGenCB customCBData;
	customCBData.Source_QS = sourceQS;
	customCBData.Source_QB = sourceQO;
	customCBData.Target_QS = targetQS;
	customCBData.Target_QB = targetQO;
	customCBData.Morph_QS = m_quantizationScale;
	customCBData.Morph_QB = m_quantizationOffset;
	customCBData.Weight = ratio;
#else
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0, Vector( ratio, 0, 0, 0 ) );
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_1, sourceQS );
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_2, sourceQO );
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_3, targetQS );
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_4, targetQO );

	// Set quantization parameters for the results.
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_5, m_quantizationScale );
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_6, m_quantizationOffset );
#endif

	const Uint32 posStreamIndex = RenderMeshStreamToIndex( RMS_PositionSkinning );
	const Uint32 tanStreamIndex = RenderMeshStreamToIndex( RMS_TangentFrame );
	const Uint32 texStreamIndex = RenderMeshStreamToIndex( RMS_TexCoords );

	// We have to do this chunk by chunk because the vertices are stored in one buffer for all chunks, and the vertex format can be different.
	const auto& sourceChunks = m_sourceMesh->GetChunks();
	const auto& targetChunks = m_targetMesh->GetChunks();
	const Uint32 numChunks = m_chunks.Size();
	for ( Uint8 chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex )
	{
		const CRenderMesh::Chunk& interpChunk = m_chunks[chunkIndex];
		const AdditionalChunkData& chunkData = m_chunkData[chunkIndex];

		// Skip Target chunks. We just want to process once per original chunk, so we do the blended and source.
		if ( chunkData.m_flags & RMMCF_FromTargetMesh )
		{
			continue;
		}

		const Int8 originalChunkIndex = chunkData.m_sourceChunk;
		RED_FATAL_ASSERT( originalChunkIndex >= 0 && originalChunkIndex < sourceChunks.SizeInt() && originalChunkIndex < targetChunks.SizeInt(), "Invalid chunk index" );
		const CRenderMesh::Chunk& sourceChunk = sourceChunks[originalChunkIndex];
		const CRenderMesh::Chunk& targetChunk = targetChunks[originalChunkIndex];

		CRenderTexture* tex = m_controlTextures[ sourceChunk.m_materialId ];
		const Bool canUseControlTexture = m_useControlTexForMorph && tex != nullptr;

		ChunkGeometry sourceGeometry, targetGeometry;
		m_sourceMesh->GetChunkGeometry( originalChunkIndex, sourceGeometry );
		m_targetMesh->GetChunkGeometry( originalChunkIndex, targetGeometry );

#ifdef RED_PLATFORM_ORBIS
		Uint32 stride;
		CRenderShaderCompute* csShader = nullptr;
		if ( !GetCSShaderAndStrideForChunk( sourceChunk, canUseControlTexture, &csShader, stride ) )
		{
			continue;
		}

		// Bind texturing details, if the control texture is to be used.
		if ( canUseControlTexture )
		{
			m_controlTextures[ sourceChunk.m_materialId ]->Bind( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, RST_ComputeShader );
		}

		// Bind output buffers
		{
			customCBData.NumVertices = sourceChunk.m_numVertices;
			customCBData.Source_PosByteOffset = sourceGeometry.m_vertexOffsets[ posStreamIndex ];
			customCBData.Source_TbnByteOffset = sourceGeometry.m_vertexOffsets[ tanStreamIndex ];
			customCBData.Source_TexByteOffset = sourceGeometry.m_vertexOffsets[ texStreamIndex ];
			customCBData.Target_PosByteOffset = targetGeometry.m_vertexOffsets[ posStreamIndex ];
			customCBData.Target_TbnByteOffset = targetGeometry.m_vertexOffsets[ tanStreamIndex ];
			customCBData.OutputByteOffset = interpChunk.m_chunkVertices.byteOffsets[ posStreamIndex ];

			RED_ASSERT( sourceGeometry.m_vertexBuffers[posStreamIndex] == sourceGeometry.m_vertexBuffers[tanStreamIndex] &&
				sourceGeometry.m_vertexBuffers[posStreamIndex] == sourceGeometry.m_vertexBuffers[texStreamIndex], TXT("Source vertex stream stored whithin different buffers.") );
			RED_ASSERT( targetGeometry.m_vertexBuffers[posStreamIndex] == targetGeometry.m_vertexBuffers[tanStreamIndex] &&
				targetGeometry.m_vertexBuffers[posStreamIndex] == targetGeometry.m_vertexBuffers[texStreamIndex], TXT("Target vertex stream stored whithin different buffers.") );

			GpuApi::BindBufferSRV( sourceGeometry.m_vertexBuffers[posStreamIndex], 0, GpuApi::ComputeShader );
			GpuApi::BindBufferSRV( targetGeometry.m_vertexBuffers[posStreamIndex], 1, GpuApi::ComputeShader );
			GpuApi::SetComputeShaderConsts( customCBData );
			GpuApi::BindBufferUAV( m_vertices, 0 );
		}

		Uint32 dispatchGroups = AlignXYZ( sourceChunk.m_numVertices, 256 ) / 256;
		csShader->Dispatch( dispatchGroups, 1, 1 );
#else
		GpuApi::VertexLayoutRef inputLayout;
		CRenderShaderStreamOut* soShader = nullptr;
		if ( !GetSOShaderAndInputLayoutForChunk( sourceChunk, canUseControlTexture, &soShader, &inputLayout ) )
		{
			continue;
		}

		soShader->Bind();

		// Bind the source/target vertex buffers as input
		{
			GpuApi::BufferRef vbSourcePos = sourceGeometry.m_vertexBuffers[ posStreamIndex ];
			GpuApi::BufferRef vbSourceTan = sourceGeometry.m_vertexBuffers[ tanStreamIndex ];
			GpuApi::BufferRef vbTargetPos = targetGeometry.m_vertexBuffers[ posStreamIndex ];
			GpuApi::BufferRef vbTargetTan = targetGeometry.m_vertexBuffers[ tanStreamIndex ];

			Uint32 srcPosStride = GpuApi::GetVertexLayoutStride( sourceGeometry.m_vertexLayout, posStreamIndex );
			Uint32 srcTanStride = GpuApi::GetVertexLayoutStride( sourceGeometry.m_vertexLayout, tanStreamIndex );
			Uint32 srcByteOffsetPos = sourceGeometry.m_vertexOffsets[ posStreamIndex ];
			Uint32 srcByteOffsetTan = sourceGeometry.m_vertexOffsets[ tanStreamIndex ];

			Uint32 trgPosStride = GpuApi::GetVertexLayoutStride( targetGeometry.m_vertexLayout, posStreamIndex );
			Uint32 trgTanStride = GpuApi::GetVertexLayoutStride( targetGeometry.m_vertexLayout, tanStreamIndex );
			Uint32 trgByteOffsetPos = targetGeometry.m_vertexOffsets[ posStreamIndex ];
			Uint32 trgByteOffsetTan = targetGeometry.m_vertexOffsets[ tanStreamIndex ];

			GpuApi::BufferRef buffers[ 4 ] = { vbSourcePos, vbSourceTan, vbTargetPos, vbTargetTan };
			Uint32 strides[ 4 ] = { srcPosStride, srcTanStride, trgPosStride, trgTanStride };
			Uint32 offsets[ 4 ] = { srcByteOffsetPos, srcByteOffsetTan, trgByteOffsetPos, trgByteOffsetTan };
			GpuApi::BindVertexBuffers( 0, 4, buffers, strides, offsets );
		}

		// Bind texturing details, if the control texture is to be used.
		if ( canUseControlTexture )
		{
			Uint32 uvStride = GpuApi::GetVertexLayoutStride( sourceGeometry.m_vertexLayout, texStreamIndex );
			Uint32 uvOffset = sourceGeometry.m_vertexOffsets[ texStreamIndex ];
			GpuApi::BufferRef uvBuffer = sourceGeometry.m_vertexBuffers[ texStreamIndex ];

			GpuApi::BindVertexBuffers( 4, 1, &uvBuffer, &uvStride, &uvOffset );

			m_controlTextures[ sourceChunk.m_materialId ]->Bind( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, RST_VertexShader );
		}

		// Bind output buffers
		{
			Uint32 byteOffsetPos = interpChunk.m_chunkVertices.byteOffsets[ RenderMeshStreamToIndex( RMS_PositionSkinning ) ];

			GpuApi::BufferRef buffers[] = { m_vertices };
			Uint32 offsets[] = { byteOffsetPos };
			GpuApi::BindStreamOutBuffers( 1, buffers, offsets );
		}

		GpuApi::SetVertexFormatRaw( inputLayout );

		GpuApi::DrawPrimitive( GpuApi::PRIMTYPE_PointList, 0, sourceChunk.m_numVertices );
#endif
	}

#ifdef RED_PLATFORM_ORBIS
	GpuApi::BindBufferSRV( GpuApi::BufferRef::Null(), 0, GpuApi::ComputeShader );
	GpuApi::BindBufferSRV( GpuApi::BufferRef::Null(), 1, GpuApi::ComputeShader );
	GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 0 );
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::ComputeShader );
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_ComputeShader );
#else
	GpuApi::BindStreamOutBuffers( 0, nullptr, nullptr );
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
#endif

	// We have successfully filled our interpolated vertex buffer, so we are fully ready now.
	m_buffersPendingLoad.SetValue( 0 );

	return true;
}
