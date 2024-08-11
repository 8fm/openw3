#include "build.h"
#ifdef USE_RED_RESOURCEMANAGER

//////////////////////////////////////////////////////////////////////////
CQuestStreamingMetadataFactory::CQuestStreamingMetadataFactory()
{
}

//////////////////////////////////////////////////////////////////////////
CQuestStreamingMetadataFactory::~CQuestStreamingMetadataFactory()
{

}

//////////////////////////////////////////////////////////////////////////
MemSize CQuestStreamingMetadataFactory::CreateResourceBuffer( Red::Core::ResourceManagement::CResourceDataBuffer& dstBuffer, void* userData )
{
	// Allocate some memory.
	void* allocAddr = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceBuffer, sizeof( CQuestStreamingMetadata ) );

	// Construct a CWorldStreaming metadata object at that address.
	::new (allocAddr)CQuestStreamingMetadata();

	// Pass it to the destination handle.
	dstBuffer.SetDataBuffer( allocAddr );
	return sizeof( CQuestStreamingMetadata );
}

//////////////////////////////////////////////////////////////////////////
Bool CQuestStreamingMetadataFactory::Serialize( const void* sourceBuffer, Uint32 sourceSize, Red::Core::ResourceManagement::CResourceDataBuffer& dstBuffer, void* userData )
{
	void* allocAddr = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceBuffer, sizeof( CQuestStreamingMetadata ) );

	CQuestStreamingMetadata* qsmObj = new (allocAddr)CQuestStreamingMetadata();
	
	CMemoryFileReader memReader( static_cast< const Uint8* >( sourceBuffer ), sourceSize, 0 );
	qsmObj->Serialize( memReader );

	dstBuffer.SetDataBuffer( allocAddr );

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataFactory::DestroyResource( Red::Core::ResourceManagement::CResourceDataBuffer& buffer, void* userData )
{
	// Grab the metadata object back
	CQuestStreamingMetadata* metadata = static_cast< CQuestStreamingMetadata* >( buffer.GetDataBuffer() );
	
	// Free if the metadata object is valid.
	if( metadata != nullptr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceBuffer, metadata );
	}
	
	buffer.SetDataBuffer( nullptr );
}
#endif // USE_RED_RESOURCEMANAGER
