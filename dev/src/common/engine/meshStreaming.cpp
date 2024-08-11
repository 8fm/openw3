/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#ifdef USE_RED_RESOURCEMANAGER
#include "meshStreaming.h"

///////////////////////////////////////////////////////////////////
CStreamedMeshChunks::CStreamedMeshChunks()
{

}

///////////////////////////////////////////////////////////////////
CStreamedMeshChunks::~CStreamedMeshChunks()
{

}

///////////////////////////////////////////////////////////////////
CStreamedMeshChunks::CStreamedMeshChunks( CStreamedMeshChunks&& other )
{
	*this = std::move(other);
}

///////////////////////////////////////////////////////////////////
CStreamedMeshChunks& CStreamedMeshChunks::operator=(CStreamedMeshChunks&& other)
{
	m_chunkArray = std::move(other.m_chunkArray);
	return *this;
}

///////////////////////////////////////////////////////////////////
void CStreamedMeshChunks::AddChunk( Uint16 chunkIndex, TDynArray< Uint8, MC_BufferMesh >(& vertices)[MAX_STREAMED_MESH_VERTEX_STREAMS], TDynArray< Uint8, MC_BufferMesh >& indexData )
{
	// Do not add duplicate chunks!
	for( Uint32 i=0; i<m_chunkArray.Size(); ++i )
	{
		if( m_chunkArray[i].GetChunkIndex() == chunkIndex )
		{
			return;
		}
	}

	Uint32 chunkArrayIndex = m_chunkArray.Size();
	m_chunkArray.Grow( 1 );
	RED_ASSERT( m_chunkArray.Size() >= chunkArrayIndex, TXT( "Failed to grow chunk array" ) );
	m_chunkArray[chunkArrayIndex] = CMeshChunkPart( chunkIndex, vertices, indexData );
}

///////////////////////////////////////////////////////////////////
CMeshChunkPart& CStreamedMeshChunks::GetChunkByIndex( Uint32 index )
{
	RED_ASSERT( index < m_chunkArray.Size(), TXT( "Bad chunk index!" ) );
	return m_chunkArray[index];
}

///////////////////////////////////////////////////////////////////
void CStreamedMeshChunks::Serialize( IFile& file )
{
	Uint32 chunkCount = m_chunkArray.Size();
	file << m_ownerMesh;
	file << chunkCount;
	if( !file.IsGarbageCollector() && !file.IsMapper() )
	{
		if( file.IsReader() )
		{
			m_chunkArray.Resize( chunkCount );
		}
		
		for( Uint32 i=0; i<chunkCount; ++i )
		{
			m_chunkArray[i].Serialize( file );
		}
	}
}

////////////////////////////////////////////////////////////////////
void CMeshChunkPart::Serialize( IFile& file )
{
	file << m_chunkIndex;
	for( Int32 s = 0; s < MAX_STREAMED_MESH_VERTEX_STREAMS; ++s )
	{
		m_packedVertices[s].BulkSerialize( file );
	}
	m_packedIndices.BulkSerialize( file );
}

////////////////////////////////////////////////////////////////////
CStreamedMeshChunksFactory::CStreamedMeshChunksFactory()
{
}

////////////////////////////////////////////////////////////////////
CStreamedMeshChunksFactory::~CStreamedMeshChunksFactory()
{
}

////////////////////////////////////////////////////////////////////
// Allocate memory for a new resource buffer. Return the size allocated
MemSize CStreamedMeshChunksFactory::CreateResourceBuffer( Red::Core::ResourceManagement::CResourceDataBuffer& dstBuffer, void* userData )
{
	RED_UNUSED( dstBuffer );
	RED_UNUSED( userData );
	return 0;
}

////////////////////////////////////////////////////////////////////
// Serialize data from sourceBuffer to a new resource buffer. Returns the size of the new buffer
Bool CStreamedMeshChunksFactory::Serialize( const void* srcBuffer, Uint32 sourceSize, Red::Core::ResourceManagement::CResourceDataBuffer& dstBuffer, void* userData )
{
	PC_SCOPE( CStreamedMeshChunksFactory_Serialize );

	RED_UNUSED( userData );

	// Load the chunks from the file data
	TMemoryFileReader< MC_ResourceBuffer > memoryFileReader( reinterpret_cast< const Uint8* >( srcBuffer ), sourceSize, 0 );
	Red::Core::ResourceManagement::CResourceId meshHash;

	// Read in resource id
	memoryFileReader << meshHash;	// Kill me.

	// Now find the cpu-side mesh which we need to patch (actually we path the render mesh, but we get that from the CMesh)
	Red::Core::ResourceManagement::CResourceHandle ownerMeshHandle = m_resourceManager->AcquireResource( meshHash );
	if( ownerMeshHandle.IsValid() == false )
	{
		// Dependency hacks.
		return false;
	}

	memoryFileReader.Seek(0);		// Sweet jesus

	CStreamedMeshChunks* newChunks = new CStreamedMeshChunks();
	newChunks->Serialize( memoryFileReader );

	IRenderResource* resourceToPatch = reinterpret_cast< IRenderResource* >( ownerMeshHandle.Get< CMesh >()->GetRenderResource() );
	RED_ASSERT( resourceToPatch );

	// patch the data
	{
		PC_SCOPE( CStreamedMeshChunksFactory_PatchData );
		resourceToPatch->PatchStreamedResourceData( newChunks );
	}

	{
		PC_SCOPE( CStreamedMeshChunksFactory_Cleanup );

		// Now, throw the data away
		newChunks->ClearCpuSideData();

		// For now, we keep the empty chunks around since the system requires that this outputs a resource
		// Eventually we want the ability to throw the data away, i.e. have the concept of data-only 
		dstBuffer.SetDataBuffer( newChunks );
	}

	return true;
}

////////////////////////////////////////////////////////////////////
// Destroy a resource
void CStreamedMeshChunksFactory::DestroyResource( Red::Core::ResourceManagement::CResourceDataBuffer& buffer, void* userData )
{
	RED_UNUSED( userData );
	CStreamedMeshChunks* theMeshChunk = (CStreamedMeshChunks*)buffer.GetDataBuffer();
	buffer.SetDataBuffer( nullptr );
	if( theMeshChunk )
	{
		delete theMeshChunk;
	}	
}

namespace MeshStreamingUtils
{
	////////////////////////////////////////////////////////////////////
	// Moves out the VB and IB from a single mesh chunk
	void StripChunkDataFromMesh( CMesh& sourceMesh, CStreamedMeshChunks& resultingChunk, Uint16 chunkIndex )
	{
		RED_ASSERT( (Uint32)chunkIndex < sourceMesh.m_chunks.GetConst().Size(), TXT( "Bad chunk index" ) );

		// We pack the index and vertex data in advance so the stream patching is faster during run-time
		TDynArray< Uint8, MC_BufferMesh > vertexStreams[MAX_STREAMED_MESH_VERTEX_STREAMS];
		TDynArray< Uint8, MC_BufferMesh > indexData;

		RenderMeshUtils::GenerateStreamingChunk( sourceMesh, chunkIndex, vertexStreams, indexData );
		resultingChunk.AddChunk( chunkIndex, vertexStreams, indexData );
	}

	// Get the number of chunk resources expected for a particular mesh
	Int32 GetChunkResourceCount( CMesh& sourceMesh, Int32 lodIndex )
	{
		RED_UNUSED( lodIndex );

		// For now, we assume the mesh only ever returns one chunk resource for the total mesh, or even for a particular lod
		return sourceMesh.m_chunks.GetAndUnlink().Size() > 0 ? 1 : 0;
	}

	////////////////////////////////////////////////////////////////////
	void GenerateChunksFromMesh( CMesh& sourceMesh, TDynArray< CStreamedMeshChunks >& resultingChunks, Int32 lodIndex )
	{
		if( lodIndex == -1 )		// Lod = -1, add all LODs to a single chunk
		{
			Int32 resultChunkIndex = resultingChunks.Size();
			resultingChunks.Grow(1);
			
			Int32 chunkCount = sourceMesh.m_chunks.GetAndUnlink().Size();
			for( Int16 i=0; i<chunkCount; ++i )
			{
				StripChunkDataFromMesh( sourceMesh, resultingChunks[resultChunkIndex], i );
			}
		}
		else
		{
			// Add a chunk entry just for this lod
			Int32 resultChunkIndex = resultingChunks.Size();
			resultingChunks.Grow(1);
			RED_ASSERT( (Uint32)lodIndex < sourceMesh.m_lodLevelInfo.Size(), TXT( "Bad lod index!" ) );
			Int32 chunkCount = sourceMesh.m_lodLevelInfo[ lodIndex ].m_chunks.Size();
			for( Int16 chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex )
			{
				StripChunkDataFromMesh( sourceMesh, resultingChunks[resultChunkIndex], chunkIndex );
			}
		}
	}

	////////////////////////////////////////////////////////////////////
	String GetMeshChunkFilename( const String& meshName, Int32 chunksIndex )
	{
		return meshName + String(TXT("_")) + String::Printf( TXT( "chunk%d" ), chunksIndex ) + String( TXT( ".chnk" ) );
	}

	////////////////////////////////////////////////////////////////////
	String SaveMeshChunksToDisk( CDirectory& directory, const String& meshName, Int32 chunksIndex, CStreamedMeshChunks& chunks )
	{
		String filename = GetMeshChunkFilename( meshName, chunksIndex );
		String fullPath = directory.GetAbsolutePath() + filename;

		IFile* fileWriter = GFileManager->CreateFileWriter( fullPath, FOF_AbsolutePath | FOF_Buffered );
		RED_ASSERT( fileWriter, TXT( "Failed to open chunk file for writing" ) );

		chunks.Serialize( *fileWriter );
		
		delete fileWriter;

		return filename;
	}

	////////////////////////////////////////////////////////////////////
	// Save a stripped mesh to disk
	String SaveStrippedMeshToDisk( CDirectory& directory, CMesh& theMesh )
	{
		String fullPath = directory.GetAbsolutePath() + theMesh.GetFile()->GetFileName();
		IFile* fileWriter = GFileManager->CreateFileWriter( fullPath, FOF_AbsolutePath | FOF_Buffered );
		RED_ASSERT( fileWriter, TXT( "Failed to open mesh file for writing" ) );

		// Save the mesh using the dependency writer to ensure any materials, collision data, etc is pulled in properly
		CDependencySaver saver( *fileWriter, nullptr );
		DependencySavingContext context( &theMesh );
		context.m_saveTransient = true;
		context.m_saveReferenced = true;
		saver.SaveObjects( context );

		delete fileWriter;

		return directory.GetDepotPath() + theMesh.GetFile()->GetFileName();
	}

}

#endif