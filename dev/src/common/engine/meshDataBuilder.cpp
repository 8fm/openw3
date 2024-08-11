#include "build.h"
#include "mesh.h"
#include "meshDataBuilder.h"
#include "materialInstance.h"
#include "materialDefinition.h"

#include "simplygonHelpers.h"
#include "../core/feedback.h"

CMeshData::CMeshData( const CMesh* mesh )
	: m_mesh( mesh )
	, m_chunks( mesh->GetUnpackedData() )
	, m_lodLevelInfo( mesh->GetMeshLODLevels() )
{
}

CMeshData::~CMeshData()
{
}

const Uint32 CMeshData::GetNumTriangles() const
{
	Uint32 numTriangles = 0;
	const auto& chunks = GetChunks();
	for ( Uint32 i=0; i<chunks.Size(); ++i )
	{
		numTriangles += chunks[i].m_indices.Size() / 3;
	}
	return numTriangles;
}

const Uint32 CMeshData::GetNumVertices() const
{
	Uint32 numVertices = 0;
	const auto& chunks = GetChunks();
	for ( Uint32 i=0; i<chunks.Size(); ++i )
	{
		numVertices += chunks[i].m_vertices.Size();
	}
	return numVertices;
}

#ifndef NO_RESOURCE_IMPORT

void CMeshData::Clear()
{
	// remove existing chunks
	auto& chunks = GetChunks();
	chunks.Clear();

	// reset LODs
	m_lodLevelInfo.Resize(1);
	m_lodLevelInfo[0].m_chunks.ClearFast();
	m_lodLevelInfo[0].m_meshTypeLOD.m_distance = 0.0f;
}

Bool CMeshData::FlushChanges( const Bool optimizeMesh /*= true*/ ) const
{
	CMesh* nonConstMesh = const_cast< CMesh* >( m_mesh );
	return nonConstMesh->SetMeshData( *m_chunks, m_lodLevelInfo, true, optimizeMesh );
}

Bool CMeshData::AddLOD( Float distance, Int32 insertAtIndex )
{
	if ( insertAtIndex == -1 )
	{
		m_lodLevelInfo.PushBack( CMesh::LODLevel( distance ) );
	}
	else
	{
		m_lodLevelInfo.Insert( insertAtIndex, CMesh::LODLevel( distance ) );
	}
	return true;
}

void CMeshData::RemoveLOD( const Uint32 lodIndex )
{
	auto& chunks = GetChunks();
	auto& chunkIndices = m_lodLevelInfo[ lodIndex ].m_chunks;

	for ( Int32 i = chunkIndices.SizeInt()-1; i >= 0; --i )
	{
		Uint16 removedIndex = chunkIndices[ i ];
		chunks.RemoveAt( removedIndex );

		// modify LOD chunk indices
		for ( auto& lodLevel : m_lodLevelInfo )
		{
			for ( Uint16& toCorrectIdx : lodLevel.m_chunks )
			{
				if ( toCorrectIdx > removedIndex )
				{
					toCorrectIdx -= 1;
				}
			}
		}
	}

	// Remove the LOD
	m_lodLevelInfo.RemoveAt( lodIndex );
}

void CMeshData::RemoveChunk( const Uint32 chunkIndex )
{
	auto& chunks = GetChunks();

	// Remove chunk
	chunks.RemoveAt( chunkIndex );

	// Go through LODs and remove chunk from list
	for ( auto& lodLevel : m_lodLevelInfo )
	{
		lodLevel.m_chunks.Remove( chunkIndex );
		for ( Uint16& chunkIdx : lodLevel.m_chunks )
		{
			if ( chunkIdx > chunkIndex )
			{
				--chunkIdx;
			}
		}
	}
}

SMeshChunk* CMeshData::AddChunkToLOD( Uint32 lodIndex, Uint32 materialId, EMeshVertexType vertexType, Uint32& newChunkIndex )
{
	ASSERT( lodIndex < m_lodLevelInfo.Size() );

	// search backwards for first, non-empty LOD to add a chunk after it
	newChunkIndex = 0; // this is the fallback value if all LODs back are empty - add at the beginning
	for ( Int32 idx = lodIndex; idx >= 0; --idx )
	{
		if ( !m_lodLevelInfo[ idx ].m_chunks.Empty() )
		{
			newChunkIndex = m_lodLevelInfo[ idx ].m_chunks.Back() + 1;
			break;
		}
	}


	// fix up the IDs
	for ( CMesh::LODLevel& lodLevel : m_lodLevelInfo )
	{
		for ( Uint16& chunkIdx : lodLevel.m_chunks )
		{
			if ( chunkIdx >= newChunkIndex )
			{
				++chunkIdx;
			}
		}
	}

	// insert chunk
	m_chunks->Insert( newChunkIndex, SMeshChunk() );
	m_lodLevelInfo[ lodIndex ].m_chunks.PushBack( newChunkIndex );

	// prepare chunk
	SMeshChunk& addedChunk = GetChunks()[ newChunkIndex ];
	addedChunk.m_materialID = materialId;
	addedChunk.m_vertexType = vertexType;
	return &addedChunk;
}

Int32 CMeshData::FindLODForChunk( Int32 index ) const
{
	for ( Uint32 lodIdx = 0; lodIdx < m_lodLevelInfo.Size(); ++lodIdx )
	{
		if ( m_lodLevelInfo[ lodIdx ].m_chunks.FindPtr( static_cast< Int16 >( index ) ) )
		{
			return lodIdx;
		}
	}

	return -1;
}

Bool CMeshData::CompareMaterials( Uint32 matIdx1, Uint32 matIdx2 ) const
{
	const auto& materials = m_mesh->m_materials;
	ASSERT ( matIdx1 < materials.Size() && matIdx2 < materials.Size() );

	if ( matIdx1 == matIdx2 )
	{
		return true; // easy thing
	}

	if ( materials[ matIdx1 ] == nullptr || materials[ matIdx2 ] == nullptr )
	{	// at least one of the materials is broken
		return false;
	}

	IMaterialDefinition* def1 = materials[ matIdx1 ]->GetMaterialDefinition();
	IMaterialDefinition* def2 = materials[ matIdx2 ]->GetMaterialDefinition();

	if ( def1 == nullptr || def2 == nullptr )
	{	// at least one of the instances is broken
		return false;
	}

	const auto& pp1 = def1->GetPixelParameters();
	const auto& pp2 = def2->GetPixelParameters();

	if ( pp1.Size() != pp2.Size() )
	{
		return false;
	}

	for ( Uint32 i=0; i < pp1.Size(); ++i )
	{
		const auto& p1 = pp1[i];
		const auto& p2 = pp2[i];

		if ( p1.m_name != p2.m_name || p1.m_type != p2.m_type )
		{
			return false; // different set of parameters
		}

		if ( p1.m_type == IMaterialDefinition::PT_Texture )
		{
			THandle< ITexture > tex1, tex2;
			materials[ matIdx1 ]->ReadParameter( p1.m_name, tex1 );
			materials[ matIdx2 ]->ReadParameter( p1.m_name, tex2 );
			if ( tex1 != tex2 )
			{
				return false;
			}
		}
	}

	return true;
}

Int32 CMeshData::MergeChunks( const TDynArray< Uint32 >& chunkIndices, Uint32 materialIndex, String* outErrorStr )
{
	if ( chunkIndices.Size() <= 1 )
	{
		return 0;
	}

	auto& chunks = GetChunks();

	// checks if all the chunk indices are valid
	for ( Uint32 i=0; i<chunkIndices.Size(); ++i )
	{
		if ( chunkIndices[i] >= chunks.Size() )
		{
			if ( outErrorStr )
			{
				*outErrorStr = TXT("Invalid chunk index found, cannot merge");
			}
			return 0;
		}
	}

	Int32 selectedLOD = FindLODForChunk( chunkIndices[0] );
	EMeshVertexType vertexType = chunks[ chunkIndices[0] ].m_vertexType;

	//check if all chunks are in the same LOD and vertex types
	for ( Uint32 i=1; i<chunkIndices.Size(); ++i )
	{
		if ( FindLODForChunk( chunkIndices[i] ) != selectedLOD )
		{
			if ( outErrorStr )
			{
				*outErrorStr = TXT("Chunks from different LODs selected, merging can't succeed");
			}
			return 0;
		}

		if ( chunks[ chunkIndices[i] ].m_vertexType != vertexType )
		{
			if ( outErrorStr )
			{
				*outErrorStr = TXT("Chunks have different vertex types, merging can't succeed");
			}
			return 0;
		}
	}

	TDynArray< Uint32 > chunksToMerge = chunkIndices;

	// = ensure that the result doesn't exceed 64k vertices cout =

 	Uint32 totalVertexCount = 0;
 	for ( Uint32 ci : chunksToMerge )
 	{
 		totalVertexCount += chunks[ ci ].m_numVertices;
 	}

	// sort chunks to merge by number of vertices
	Sort( chunksToMerge.Begin(), chunksToMerge.End(), 
		[&chunks]( Uint32 c1, Uint32 c2 ) { return chunks[ c1 ].m_numVertices < chunks[ c2 ].m_numVertices; } );

	// keep removing the largest chunk until we reach the limit or run out of chunks 
	// (the latter shouldn't really happen unless our smallest chunks is already bigger that 64k, but just in case)
	while ( totalVertexCount > 0x10000 && !chunksToMerge.Empty() )
	{
		totalVertexCount -= chunks[ chunksToMerge.Back() ].m_numVertices;
		chunksToMerge.PopBack();
 	}

	if ( chunksToMerge.Size() <= 1 )
	{
		return 0; // nothing to do
	}

	// this ordering ensures proper removing of chunks
	Sort( chunksToMerge.Begin(), chunksToMerge.End() );

	// all those chunks will be removed and a new chunk will be added to the list of the LOD

	// subtract 1 from subsequent IDs, and remove the references
	for ( Int32 ci = chunksToMerge.SizeInt() - 1; ci >= 0 ; --ci )
	{
		Uint32 chunkID = chunksToMerge[ci];
		for ( Uint32 i = selectedLOD; i < m_lodLevelInfo.Size(); ++i )
		{
			for ( Int32 j = m_lodLevelInfo[i].m_chunks.SizeInt() - 1; j >= 0 ; --j )
			{
				if ( m_lodLevelInfo[i].m_chunks[j] > chunkID )
				{
					m_lodLevelInfo[i].m_chunks[j] -=1;
				}
				else if ( m_lodLevelInfo[i].m_chunks[j] == chunkID )
				{
					m_lodLevelInfo[i].m_chunks.RemoveAt( j );
				}
			}
		}
	}

	//create new chunk
	SMeshChunk newChunk;
	newChunk.m_materialID = materialIndex;
	newChunk.m_vertexType = vertexType;

	Uint32 vertexCount = 0;
	Uint32 indexCount = 0;
	for ( Uint32 ci = 0; ci < chunksToMerge.Size(); ++ci )
	{
		const SMeshChunk& oldChunk = chunks[chunksToMerge[ci]];

		newChunk.m_vertices.PushBack( oldChunk.m_vertices );
		for ( Uint32 ii = 0; ii < oldChunk.m_indices.Size(); ++ii )
		{
			newChunk.m_indices.PushBack( oldChunk.m_indices[ii] + (Uint16)vertexCount );
		}
		vertexCount += oldChunk.m_numVertices;
		indexCount += oldChunk.m_numIndices;
	}
	newChunk.m_numIndices = indexCount;
	newChunk.m_numVertices = vertexCount;

	for ( Int32 ci = chunksToMerge.SizeInt() - 1; ci >= 0 ; --ci )
	{
		chunks.RemoveAt( chunksToMerge[ci] );
	}

	// we should keep the chunks in order so we have to find the chunk that is the last chunk of the specified LOD
	Uint16 maxChunkIndex = 0;

	// if this is the first LOD and we merged all chunks then 0 is the maxChunkIndex
	if ( selectedLOD > 0 || !m_lodLevelInfo[selectedLOD].m_chunks.Empty())
	{
		Int32 lastLOD = selectedLOD;

		// if we merge all the chunks then we have to find the last chunk of the previous LOD not the current one
		if (m_lodLevelInfo[selectedLOD].m_chunks.Empty())
		{
			lastLOD = selectedLOD - 1;
		}

		for ( Int32 chunkIndex=m_lodLevelInfo[lastLOD].m_chunks.SizeInt()-1; chunkIndex>=0; --chunkIndex )
		{
			Uint16 index = m_lodLevelInfo[lastLOD].m_chunks[(Uint32)chunkIndex];
			if(index>maxChunkIndex)
			{
				maxChunkIndex=index;
			}
		}

		// insert after the last found chunk
		chunks.Insert( maxChunkIndex + 1, newChunk );
		m_lodLevelInfo[selectedLOD].m_chunks.PushBack( maxChunkIndex + 1 );
	}
	else
	{
		// insert as first chunk
		chunks.Insert( 0, newChunk );
		m_lodLevelInfo[selectedLOD].m_chunks.PushBack( 0 );
	}

	// fix up the IDs
	for (Uint32 i = selectedLOD+1; i < m_lodLevelInfo.Size(); ++i)
	{
		for ( Uint32 j = 0; j < m_lodLevelInfo[i].m_chunks.Size(); ++j )
		{
			m_lodLevelInfo[i].m_chunks[j] +=1;
		}
	}

	return chunksToMerge.Size()-1;
}

Int32 CMeshData::MergeChunks( Uint32 lodIndex, Bool countOnly, String* outErrorStr )
{
	ASSERT ( lodIndex < m_lodLevelInfo.Size() );
	Int32 numChunksRemoved = 0;

	Bool mergeDone;
	do 
	{
		mergeDone = false;

		const auto& chunkIndices = m_lodLevelInfo[ lodIndex ].m_chunks;
		if ( chunkIndices.Empty() )
		{
			break;
		}

		auto& chunks = GetChunks();

		for ( Uint32 i=0; i<chunkIndices.Size()-1; ++i )
		{
			Uint32 firstChunkIdx = chunkIndices[ i ];

			TDynArray< Uint32 > chunksToMerge;
			Uint32 firstMatId = chunks[ firstChunkIdx ].m_materialID;
			chunksToMerge.PushBack( firstChunkIdx );

			for ( Uint32 j=i+1; j<chunkIndices.Size(); ++j )
			{
				Uint32 secondChunkIdx = chunkIndices[ j ];

				if ( chunks[ firstChunkIdx ].m_vertexType == chunks[ secondChunkIdx ].m_vertexType )
				{
					Uint32 secondMatId = chunks[ secondChunkIdx ].m_materialID;
					if ( CompareMaterials( firstMatId, secondMatId ) )
					{
						chunksToMerge.PushBack( secondChunkIdx );
					}
				}
			}

			if ( chunksToMerge.Size() > 1 )
			{
				if ( countOnly )
				{
					numChunksRemoved += chunksToMerge.Size()-1;
				}
				else if ( Int32 chunksReallyRemoved = MergeChunks( chunksToMerge, firstMatId, outErrorStr ) )
				{
					numChunksRemoved += chunksReallyRemoved;
					mergeDone = true;
					break; // after merging the chunks layout may change, so start again
				}
			}
		}
	} 
	while ( mergeDone );
	
	return numChunksRemoved;
}

Int32 CMeshData::MergeChunks( Bool countOnly, String* outErrorStr )
{
	Int32 totalNumOfChunks = 0;

	for ( Uint32 lodIdx = 0; lodIdx < m_lodLevelInfo.Size(); ++lodIdx )
	{
		totalNumOfChunks += MergeChunks( lodIdx, countOnly, outErrorStr );
	}

	return totalNumOfChunks;
}


template < typename GuardFunct >
struct ScopeExit
{
	ScopeExit( GuardFunct guard ) : m_guard( guard ) {}

	~ScopeExit( ) { m_guard(); }

	GuardFunct m_guard;
};

template < typename GuardFunct >
ScopeExit< GuardFunct > AtScopeExit( GuardFunct guard )
{
	return ScopeExit< GuardFunct >( guard );
}

Bool CMeshData::GenerateLODWithSimplygon( Int32 lodIndex, const SLODPresetDefinition& lodDefinition, Uint32& numOfRemovedChunks, String& message, Bool showProgress )
{
#if defined( USE_SIMPLYGON )
	const Bool fixWinding = true;
	const Bool useRemeshing = false;

	if ( lodIndex == -1 )
	{ // -1 means "add at the end"
		lodIndex = m_lodLevelInfo.Size(); 
	}

	numOfRemovedChunks = 0;

	if ( m_lodLevelInfo.Empty() )
	{
		message = TXT("No LODs in mesh, can't create LOD levels without initial data");
		return false;
	}

	SimplygonSDK::ISimplygonSDK* sgSDK;
	Int32 res = SimplygonHelpers::InitSDK( sgSDK );
	if ( res != SimplygonSDK::SG_ERROR_NOERROR || sgSDK == nullptr )
	{
		message = String::Printf( TXT("Unable to initialize Simplygon SDK; Code: %i; Msg: %s"), res, SimplygonHelpers::GetErrorText( res ) );
		return false;
	}

	auto scopeGuard = AtScopeExit( [](){ SimplygonHelpers::ShutdownSDK(); });

	// create a scene-object
	SimplygonSDK::spScene scene = sgSDK->CreateScene();

	TDynArray< TPair< Uint32, EMeshVertexType > > baseLodData;
	TDynArray< Uint8 > baseRenderMask;

	// put current mesh into the Simpligon
	{
		const CMeshData sourceData( m_mesh );
		const auto& chunks = sourceData.GetChunks();

		// create a mesh per chunk in the first LOD
		for ( Uint32 chunkID : m_lodLevelInfo[0].m_chunks )
		{
			const SMeshChunk& chunk = chunks[chunkID];

			// skip volumes - we don't want them to exist in any LOD except 0
			if ( m_mesh->GetMaterialNames()[ chunk.m_materialID ] == TXT( "volume" ) )
			{
				continue;
			}

			// copy some data aside to be used later, as we may want to mess with the data later, invalidating indices etc
			baseLodData.PushBack( MakePair( chunk.m_materialID, chunk.m_vertexType ) );
			baseRenderMask.PushBack( chunk.m_renderMask );

			SimplygonSDK::spSceneMesh mesh = sgSDK->CreateSceneMesh();

			mesh->SetName( UNICODE_TO_ANSI( m_mesh->GetFriendlyName().AsChar() ) );

			// get an unpacked copy of our packed geometry
			SimplygonSDK::spPackedGeometryData packedGeom = SimplygonHelpers::CreateGeometryFromMeshChunk( sgSDK, chunk, 0, fixWinding );

			SimplygonSDK::spGeometryData unpackedGeom = packedGeom->NewUnpackedCopy();

			if ( unpackedGeom.IsNull() )
			{
				message = TXT("Cannot create geometry");
				return false;
			}

			// put the unpacked geometry into the mesh
			mesh->SetGeometry( unpackedGeom );

			// add mesh to the root node
			scene->GetRootNode()->AddChild( mesh );
		}
	}

	// simplify
	if ( useRemeshing )
	{
		SimplygonHelpers::RemeshGeometry( sgSDK, scene, showProgress );
	}
	else
	{
		SimplygonHelpers::ReduceGeometry( sgSDK, scene, lodDefinition, showProgress );
	}

	// create chunks for the new LOD level
	Uint32 resultMeshCount = scene->GetRootNode()->GetChildCount();
	ASSERT( resultMeshCount == baseLodData.Size() );

	// create new data
	{
		if ( !AddLOD( lodDefinition.m_distance, lodIndex ) )
		{
			message = TXT("Cannot add new LOD");
			scene->Clear();
			return false;
		}

		for ( Uint32 i = 0; i < resultMeshCount; ++i )
		{
			SimplygonSDK::spSceneMesh topMesh = SimplygonSDK::Cast< SimplygonSDK::ISceneMesh >( scene->GetRootNode()->GetChild( i ) );

			SimplygonSDK::spPackedGeometryData packedGeom = topMesh->GetGeometry()->NewPackedCopy();

			if ( packedGeom->GetTriangleCount() <= lodDefinition.m_chunkFaceLimit )
			{
				// number of triangles below given limit - ignore this chunk
				++numOfRemovedChunks;
				continue;
			}

			Uint32 newChunkIdx;
			if ( SMeshChunk* newChunk = AddChunkToLOD( lodIndex, baseLodData[i].m_first, baseLodData[i].m_second, newChunkIdx ) )
			{
				SimplygonHelpers::CreateMeshChunkFromGeometry( sgSDK, packedGeom, fixWinding, *newChunk );
				newChunk->m_renderMask = baseRenderMask[ i ];
			}
		}
	}

	// delete source scene
	scene->Clear();

	// done
	return true;
#else
	return false;
#endif // defined( USE_SIMPLYGON )
}

namespace Helper
{
	typedef Uint64 TVertexHash;
	typedef THashMap< TVertexHash, Uint32 >  TVertexMap;

	// It checks if mapping given vertices will overflow the vertex pool, NOTE: it assumes there are no duplicates
	static Bool	CheckMappingOverflow( const TVertexMap& currentMapping, const TVertexHash ha, const TVertexHash hb, const TVertexHash hc, const Uint32 numStartVertices )
	{
		Uint32 numMappedVertices = numStartVertices;

		if ( !currentMapping.KeyExist(ha) )
		{
			numMappedVertices += 1;
		}

		if ( !currentMapping.KeyExist(hb) && (ha != hb))
		{
			numMappedVertices += 1;
		}

		if ( !currentMapping.KeyExist(hc) && (hc != hb) && (hc != ha))
		{
			numMappedVertices += 1;
		}

		return numMappedVertices < 65536;
	}

	// Transform float3 as vector
	static void TransformVector3( Float* x, const Matrix& matrix )
	{
		const Vector v(x[0], x[1], x[2], 0.0f);
		const Vector tv = matrix.TransformVector(v);
		x[0] = tv.X;
		x[1] = tv.Y;
		x[2] = tv.Z;
	}

	// Transform float3 as position
	static void TransformPosition3( Float* x, const Matrix& matrix )
	{
		const Vector v(x[0], x[1], x[2], 1.0f);
		const Vector tv = matrix.TransformPoint(v);
		x[0] = tv.X;
		x[1] = tv.Y;
		x[2] = tv.Z;
	}

	// Compute vertex spatial hash
	static TVertexHash GetVertexHash( const SMeshVertex& v )
	{
		return Red::CalculateHash64( &v, sizeof(v) ); // for now
	}

	// Dumpy interface
	class CDummyAppendHelper : public CMeshData::IMergeGeometryModifier
	{
	public:
		virtual Bool ProcessTriangle( const Matrix& referenceMatrix, SMeshVertex& a, SMeshVertex& b, SMeshVertex& c ) const
		{
			RED_UNUSED(a);
			RED_UNUSED(b);
			RED_UNUSED(c);
			return true;
		}
	};
}

Bool CMeshData::AppendChunk( const SMeshChunk& otherChunk, const Uint32 lodIndex, const Matrix& referenceMatrix, const IMergeGeometryModifier* geometryModifier, const Int32 renderMaskOverride /*= -1*/, const Int32 materialIndexOverride /*= -1*/, const Int32 vertexTypeOverride /*= -1*/ )
{
	// false if corrupted mesh found
	Bool appendSucceed = true;

	// determine material to use
	const Uint32 materialIndex = (materialIndexOverride != -1) ? ((Uint32) materialIndexOverride) : otherChunk.m_materialID;

	// invalid LOD
	RED_ASSERT( lodIndex < m_lodLevelInfo.Size(), TXT("Invalid LOD index %d. Mesh has only %d LODs"), lodIndex, m_lodLevelInfo.Size() );
	if ( lodIndex >= m_lodLevelInfo.Size() )
		return false;

	// make sure we always have append interface (to avoid ifs)
	static Helper::CDummyAppendHelper dummyGeometryProcessor;
	if ( !geometryModifier ) geometryModifier = &dummyGeometryProcessor;

	// find chunk with matching material from a given LOD
	SMeshChunk* thisChunk = nullptr;
	for ( const auto& lodInfo : m_lodLevelInfo )
	{
		// find the LAST chunk
		auto& chunks = GetChunks();
		for ( Int32 chunkIndex = lodInfo.m_chunks.SizeInt() - 1; chunkIndex >= 0; --chunkIndex )
		{
			RED_ASSERT( chunkIndex < chunks.SizeInt() );
			if ( chunkIndex < chunks.SizeInt() )
			{
				auto& chunk = chunks[ chunkIndex ];
				if ( chunk.m_materialID == materialIndex )
				{
					thisChunk = &chunk;
					break;
				}
			}
		}
	}

	// vertex mapping, helps to collapse (weld) similar vertices
	// this is especially effective if we removed streams from vertex (shadow meshes/distance proxy meshes)
	Helper::TVertexMap mappedVertices;
	mappedVertices.Reserve( 65536 );

	// add stuff
	Uint32 currentIndex = 0;
	const Uint32 numIndices = otherChunk.m_indices.Size();
	while ( currentIndex < numIndices )
	{
		// start new chunk
		if ( !thisChunk )
		{
			// Create new chunk
			SMeshChunk newChunk;
			newChunk.m_materialID = materialIndex;
			newChunk.m_renderMask = (renderMaskOverride != -1) ? (Uint8)renderMaskOverride : otherChunk.m_renderMask;
			newChunk.m_numBonesPerVertex = 0;
			newChunk.m_numIndices = 0;
			newChunk.m_numVertices = 0;
			newChunk.m_vertexType = (vertexTypeOverride != -1) ? (EMeshVertexType)vertexTypeOverride : otherChunk.m_vertexType;
			GetChunks().PushBack( newChunk );
			thisChunk = &GetChunks().Back();

			// Reserve some memory
			thisChunk->m_vertices.Reserve( 65536 );
			thisChunk->m_indices.Reserve( 65536*3 ); // worst case

			// Add chunk to given LOD
			m_lodLevelInfo[ lodIndex ].m_chunks.PushBack( GetChunks().Size() - 1 );

			// Reset vertex mapping
			mappedVertices.ClearFast();
		}

		// add vertices until they don't fit without breaking 64K limit
		while ( currentIndex < numIndices )
		{
			const Uint32 indexA = otherChunk.m_indices[ currentIndex + 0 ];
			const Uint32 indexB = otherChunk.m_indices[ currentIndex + 1 ];
			const Uint32 indexC = otherChunk.m_indices[ currentIndex + 2 ];
			
			// safe check if meshes is not corrupted
			if( otherChunk.m_vertices.Size() <= indexA || otherChunk.m_vertices.Size() <= indexB || otherChunk.m_vertices.Size() <= indexC )
			{				
				appendSucceed = false;
				currentIndex += 3;
				continue;
			}

			// accept/reject triangle
			SMeshVertex v[3];
			v[0] = otherChunk.m_vertices[ indexA ];
			v[1] = otherChunk.m_vertices[ indexB ];
			v[2] = otherChunk.m_vertices[ indexC ];
			if ( !geometryModifier->ProcessTriangle( referenceMatrix, v[0], v[1], v[2] ) )
			{
				// triangle was filtered out
				currentIndex += 3;
				continue;
			}

			// compute vertex hashes
			Helper::TVertexHash vh[3];
			vh[0] = Helper::GetVertexHash( v[0] );
			vh[1] = Helper::GetVertexHash( v[1] );
			vh[2] = Helper::GetVertexHash( v[2] );

			// start new chunk if we have to
			if ( !Helper::CheckMappingOverflow( mappedVertices, vh[0], vh[1], vh[2], thisChunk->m_vertices.Size() ) )
			{
				LOG_ENGINE( TXT("Data will not fit current mesh, starting new chunk, current size") );
				break;
			}

			// transform vertices and pack them
			for ( Uint32 i=0; i<3; ++i )
			{
				// map vertex
				Uint32 mappedIndex = 0;
				if ( !mappedVertices.Find( vh[i], mappedIndex  ) )
				{
					// add to mapping
					mappedIndex = thisChunk->m_vertices.Size();
					mappedVertices.Insert( vh[i], mappedIndex );

					// add to chunk
					thisChunk->m_vertices.PushBack( v[i] );
				}

				// store the triangle
				RED_FATAL_ASSERT( mappedIndex < 65536, "Vertex buffer overflow" );
				thisChunk->m_indices.PushBack( (Uint16) mappedIndex );
			}

			// Advance
			currentIndex += 3;
		}

		// finish current chunk
		if ( thisChunk )
		{
			thisChunk->m_numIndices = thisChunk->m_indices.Size();
			thisChunk->m_numVertices = thisChunk->m_vertices.Size();
			thisChunk = nullptr;
		}
	}

	return appendSucceed;
}

#endif