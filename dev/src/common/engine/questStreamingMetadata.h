/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_QUEST_STREAMING_METADATA_H_
#define _RED_QUEST_STREAMING_METADATA_H_

#ifdef USE_RED_RESOURCEMANAGER

#include "../core/bundledefinition.h"
#include "../core/resourcedatabase.h"


//////////////////////////////////////////////////////////////////////////
// helper struct for a layer with its associated resource nodes
struct SLayerNode
{
	// Container type typedef's
	typedef TDynArray< Red::Core::BundleDefinition::SBundleFileDesc* >	BundleFileDescriptions;
	
	// Data.
	CGUID																m_layerGuid;
	String																m_LayerShortName;
	String																m_nodeName;
	TDynArray< CComponentResourceCollection* >							m_componentCollections;
	BundleFileDescriptions												m_bundleDescriptions;
};

//////////////////////////////////////////////////////////////////////////
class CQuestMetadataNodeInfo
{
public:

	//////////////////////////////////////////////////////////////////////////
	// Constructors
	//////////////////////////////////////////////////////////////////////////
	CQuestMetadataNodeInfo();

	CQuestMetadataNodeInfo( const StringAnsi& depotFile, const Uint32 itemCount, const CGUID& layerGuid, const Red::System::GUID& bundleGuid );

	CQuestMetadataNodeInfo( const CQuestMetadataNodeInfo& other );

	//////////////////////////////////////////////////////////////////////////
	// Destructor.
	//////////////////////////////////////////////////////////////////////////
	~CQuestMetadataNodeInfo();
	//////////////////////////////////////////////////////////////////////////
	// Public Data
	//////////////////////////////////////////////////////////////////////////
	StringAnsi									m_depotFile;	// Bundle name.
	Uint32										m_itemCount;
	Uint32										m_headerSize;	// The size of the depot File.
	CGUID										m_layerGUID;
	CGUID										m_bundleGUID;
};

//////////////////////////////////////////////////////////////////////////
class CQuestStreamingMetadata
{
	public:
		CQuestStreamingMetadata();
		~CQuestStreamingMetadata();
		
		// Write the metadata file.
		void Serialize( IFile& file );
		
		// Adds the node info at the correct depth.
		void AddNodeInfo( CQuestMetadataNodeInfo& nodeInfo );

		// Returns the metadata node
		CQuestMetadataNodeInfo* GetNodeInfo( const CGUID& guid );

	private:

		TDynArray< CQuestMetadataNodeInfo, MC_ResourceBuffer >		m_metadataNodes;

		friend class RedGui::CRedGuiResourceSystemHeatMapControl;
};


//////////////////////////////////////////////////////////////////////////
class CQuestStreamingMetadataCreator : Red::System::NonCopyable
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Constructors
	//////////////////////////////////////////////////////////////////////////
	CQuestStreamingMetadataCreator( Red::Core::ResourceManagement::CResourceManager& resourceManager );

	//////////////////////////////////////////////////////////////////////////
	// Destructor
	//////////////////////////////////////////////////////////////////////////
	~CQuestStreamingMetadataCreator();

	//////////////////////////////////////////////////////////////////////////
	// Public Methods
	//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR
	// Initialized the metadata, and prepares a file for the results.
	void Initialize( const String& worldFilename, CDirectory& directory );

	// This function emulates the behavior that should occur in the build 
	// pipeline.
	// It generates all the data related to the metadata as well as the 
	// metadata itself.
	Red::Core::ResourceManagement::CResourceHandle& Generate( const CWorld* world, Bool loadGlobalLayers = true );
#endif

	//////////////////////////////////////////////////////////////////////////
	// Public Data
	//////////////////////////////////////////////////////////////////////////

private:

	//////////////////////////////////////////////////////////////////////////
	// Private Methods
	//////////////////////////////////////////////////////////////////////////
#ifndef NO_EDITOR

	// Collect all the layer info's from the world
	void CollectWorldLayerInfos( const CWorld* world, TDynArray< CLayerInfo* >& worldLayerInfos );
	
	// Count the number of layers in a layer group
	Uint32 CountLayers( const CLayerGroup* layerGroup, Bool countSystemGroupLayer ) const;

	// Filter the layerinfo's based upon layerinfo tag.
	void FilterLayers( TDynArray< CLayerInfo* >& srcLayers, TDynArray< CLayerInfo* >& dstLayers, ELayerBuildTag layerInfoTag );

	// Gather all the entities from a layer
	void GatherEntities( CLayerInfo* srcLayer, TDynArray< CEntity* >& entities );

	// Create working data directory
	CDirectory* CreateWorkingDataDirectory( const CDirectory& worldDirectory );
	
	// Builds the metadata based off what's contained in the quad-tree.
	void CreateMetadata();

	// Creates and serialized the generated metadata to disk.
	Bool CreateMetadataFile();

	// Write out node files
	void SaveNodeFiles();

	// Process' each layer to extract the entities and component data.
	void ProcessLayer( CLayerInfo* layerInfo, const Uint32 layerIdx );
	
	// Insert the entity in the final bundle from the layer.
	void InsertEntity( CEntity* entity, SLayerNode& layerNode, const Uint32 entityIdx );
	
	// Insert and prepare the components from an entity for bundling.
	void InsertComponent( CComponent* component, SLayerNode& layerNode, const Uint32 entityIdx );

#endif // ! NO_EDITOR
	//////////////////////////////////////////////////////////////////////////
	// Private Data
	//////////////////////////////////////////////////////////////////////////
	String																m_metadataFilename;
	CDirectory*															m_workingDataDirectory;
	StringAnsi															m_bundleDefinitionFilename;
	Red::Core::BundleDefinition::CBundleDefinitionWriter*				m_bundleDefinitionWriter;

	Red::Core::ResourceManagement::CResourceManager&					m_resourceManager;			// Handle to the resource manager.
	Red::Core::ResourceManagement::CResourceHandle						m_questStreamingMetadataResourceHandle;	// Resource handle to the quest streaming metadata resource.

	Red::Core::ResourceManagement::CResourceBuildDatabase*										m_buildDb;				// Database of all the assets we touch during generation

	TDynArray< SLayerNode >												m_layerNodes;
};
#endif
#endif // _RED_QUEST_STREAMING_METADATA_H_