/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_STREAMING_RESOURCE_HELPER_H_
#define _RED_STREAMING_RESOURCE_HELPER_H_

#ifdef USE_RED_RESOURCEMANAGER
#include "../core/resourcedatabase.h"

//////////////////////////////////////////////////////////////////////////
struct SResourceDatabaseItemCreationData
{
	String m_typeName;
	String m_resourceName;
	String m_resourceIDName;
	String m_sourcePath;
	Red::System::GUID m_guid;
	TDynArray< Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex > m_dependencies;
};

//////////////////////////////////////////////////////////////////////////
class CStreamingResourceHelper 
{				
public:	
	typedef TDynArray< Red::Core::BundleDefinition::SBundleFileDesc* > BundleFileDescriptions;
	//////////////////////////////////////////////////////////////////////////
	// Constructors
	//////////////////////////////////////////////////////////////////////////
	CStreamingResourceHelper()
	{
	}
	
	//////////////////////////////////////////////////////////////////////////
	// Destructor
	//////////////////////////////////////////////////////////////////////////
	~CStreamingResourceHelper();
	
	//////////////////////////////////////////////////////////////////////////
	// Static Methods
	//////////////////////////////////////////////////////////////////////////

	// Creates and adds a bundle file description to the bundle file description list.
	static void CreateBundleFileDescription( CDiskFile* diskFile, const Uint32 fourCC, const Red::Core::ResourceManagement::ECompressionType compressionType, Uint32 thisResource, Red::Core::ResourceManagement::CResourceBuildDatabase& resourceDatabase, BundleFileDescriptions& bundleDescriptions );

	// Serialise a component to a resource collection
	static void SaveComponentToCollection( CMeshComponent* component, CComponentResourceCollection* collection, Uint8 streamingTreeDepth = 0xFF );

	// Extracts the CMesh import for a component, strips it, re-saves it as a temporary resource, then adds it to the bundle
	// Returns the resource index for the mesh component.
	static Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex ProcessMeshComponent( CMeshComponent* component, Red::Core::ResourceManagement::CResourceBuildDatabase& buildDb, CDirectory& workingDirectory, BundleFileDescriptions& bundleDescriptions );

	// Prepares the entities for re-saving.
	static void PrepareEntityForResaved( CEntity* entity );

	// Re-saves the entity group collection as a temp file ready for bundling.
	static Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex ResaveEntityGroupResources( CLayerInfo* layerInfo, Red::Core::ResourceManagement::CResourceBuildDatabase& database, Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex entityGroupResourceIndex, CDirectory& workingDirectory, BundleFileDescriptions& bundleDescriptions );

	// Function helper for adding a new resource to the database to ensure consistency.
	static Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex AddNewResourceToDatabase( Red::Core::ResourceManagement::CResourceBuildDatabase& resourceDatabase, const SResourceDatabaseItemCreationData& resourceCreationData );

	// Builds a string which includes all the parent layerGroups.
	static String BuildLayerGroupString( CLayerGroup* layerGroup );
};

#endif // USE_RED_RESOURCEMANAGER
#endif //_RED_STREAMING_RESOURCE_HELPER_H_