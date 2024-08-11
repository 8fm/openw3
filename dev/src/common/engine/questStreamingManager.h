/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_QUEST_STREAMING_MANAGER_H_
#define _RED_QUEST_STREAMING_MANAGER_H_
#ifdef USE_RED_RESOURCEMANAGER

class Red::Core::ResourceManagement::IFileLoadedCallback;
#include "../redThreads/redThreadsAtomic.h"

//////////////////////////////////////////////////////////////////////////
class CManagedLayer : public Red::Core::ResourceManagement::IFileLoadedCallback, Red::System::NonCopyable
{
public:
	CManagedLayer( CLayerInfo& layerInfo ) 
		: m_needsLayerLoading( false )
		, m_bundleLoadRequested( false )
		, m_layerInfo( layerInfo )
	{
	}

	RED_INLINE Bool operator==( const CLayerInfo* layerInfo )
	{
		return ( &m_layerInfo == layerInfo );
	}

	RED_INLINE void SetBundleLoadRequested( Bool loaded )
	{
		m_bundleLoadRequested = loaded;
	}

	RED_INLINE Bool BundleLoadRequested() const
	{
		return m_bundleLoadRequested;
	}


	RED_INLINE Bool HasLayer()
	{
		return ( m_layerInfo.IsLoaded() ) ? true : false;
	}

	RED_INLINE const Red::System::GUID& GetLayerGUID()
	{
		return m_layerInfo.GetGUID();
	}

	void LoadLayer();

	void UnloadLayer();

	virtual void Callback( Red::Core::ResourceManagement::CResourceHandleCollection& resourceHandleCollection );

private:

	RED_INLINE void ReadyForLayer()
	{
		m_needsLayerLoading.SetValue( true );
	}

	Red::Threads::CAtomic< Bool >		m_needsLayerLoading;
	Bool								m_bundleLoadRequested;
	CLayerInfo&							m_layerInfo;
};

//////////////////////////////////////////////////////////////////////////
class CQuestStreamingManager : public Red::System::NonCopyable
{
public:
	CQuestStreamingManager( Red::Core::ResourceManagement::CResourceManager& resourceManager, const CDiskFile& world );
	~CQuestStreamingManager();

	Bool Initialize();

	void AddLayers( TDynArray< CLayerInfo* >& layerInfos );
	void RemoveLayers( TDynArray< CLayerInfo* >& layerInfos );

	void Update( const Vector& position, const TDynArray< Float >& streamingDistances );

#ifndef NO_EDITOR
	void Generate( const CWorld* world );
#endif
private:
	// Flush the layers that need to be loaded.
	void FlushLayers();

	// Interface for loading a bundle.
	void LoadBundle();

	// Interface for unloading a bundle.
	void UnloadBundle();

	typedef THashMap< Red::System::GUID, Red::Core::ResourceManagement::CResourceHandleCollection*, MC_ResourceBuffer > TResourceHandleMap;

	TDynArray< CManagedLayer* >															m_managedLayers;

	TDynArray< CQuestMetadataNodeInfo*, MC_ResourceBuffer >								m_bundlesToLoad;
	TDynArray< CQuestMetadataNodeInfo*, MC_ResourceBuffer >								m_bundlesToUnload;

	TResourceHandleMap																	m_activeResourceHandles;

	//Layer GUID to Layer look up. 
	Red::Core::ResourceManagement::CResourceManager&									m_resourceManager;
	
	String																				m_worldFilename;
	CDirectory*																			m_directory;

	Red::Core::ResourceManagement::CResourceHandle										m_metadataResourceHandle;

	friend class RedGui::CRedGuiResourceSystemHeatMapControl;
};

#endif // USE_RED_RESOURCEMANAGER
#endif // _RED_QUEST_STREAMING_MANAGER_H_

