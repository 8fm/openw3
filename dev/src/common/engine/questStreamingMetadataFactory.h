/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef QUEST_STREAMING_METADATA_FACTORY_H_
#define QUEST_STREAMING_METADATA_FACTORY_H_
#ifdef USE_RED_RESOURCEMANAGER

class CQuestStreamingMetadataFactory : public Red::Core::ResourceManagement::IResourceFactoryBase
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ResourceFactory );
public:
	//////////////////////////////////////////////////////////////////////////
	// Constructor
	//////////////////////////////////////////////////////////////////////////
	CQuestStreamingMetadataFactory();

	//////////////////////////////////////////////////////////////////////////
	// Destructor
	//////////////////////////////////////////////////////////////////////////
	~CQuestStreamingMetadataFactory();
	//////////////////////////////////////////////////////////////////////////
	// Public Methods
	//////////////////////////////////////////////////////////////////////////
	// Create a memory buffer for a resource.
	virtual MemSize CreateResourceBuffer( Red::Core::ResourceManagement::CResourceDataBuffer& dstBuffer, void* userData ) final;

	// Serialize the data
	virtual Bool Serialize( const void* sourceBuffer, Uint32 sourceSize, Red::Core::ResourceManagement::CResourceDataBuffer& dstBuffer, void* userData ) final;			

	// Destroy the resource.
	virtual void DestroyResource( Red::Core::ResourceManagement::CResourceDataBuffer& buffer, void* userData ) final;

private:
};

#endif // USE_RED_RESOURCEMANAGER
#endif // QUEST_STREAMING_METADATA_FACTORY_H_