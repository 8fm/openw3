/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef __RESOURCE_LOADING_H__
#define __RESOURCE_LOADING_H__

class CDiskFile;
class CResource;

/// Loading task context for resource loading thread
/// This structure is fire and forget, never cache a reference/pointer to it
class ResourceLoadingTask
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

public:
	CDiskFile*			m_diskFile;					// Disk file to feed with the loaded file
	
public:
	ResourceLoadingTask( CDiskFile* diskFile );
	~ResourceLoadingTask();
};

/// Resource loading dispatcher
/// Main objective: feed the depot with loaded files 
/// Secondary objective: optimize the file loading times by using the depenency cache
class CResourceLoader
{
public:
	CResourceLoader();
	RED_MOCKABLE ~CResourceLoader();

	// Enable/Disable "loading mode" - more aggressive usage of resources
	void ToggleLoadingMode( const Bool isInLoadingMode );

	// Schedule resources to be loaded and wait for the loading to complete
	// Files will be opened, deserialized and if everything is OK, added to the depot via the CDiskFile
	// All files in the queue will be flushed
	// If we have the dependency map supplied it will be used to extract file dependencies and load them
	RED_MOCKABLE void Load( CDiskFile* rootFile, CDiskFile** files, Uint32 numFiles, EResourceLoadingPriority priority, class IDependencyImportLoader* dependencyLoader );

private:
	// Resolve file dependencies and load them
	void PrepareDependencies( CDiskFile** rootFiles, const Uint32 numRootFiles, TDynArray< CDiskFile* >& outLoadingFiles ) const;

	// Two version of the loading code - sync and async, choose wisely
	void LoadManyAsync( CDiskFile* rootFile, const EResourceLoadingPriority priority, CDiskFile** loadingList, const Uint32 numLoadingListFiles, class IDependencyImportLoader* dependencyLoader );
	
	// Are we in the "loading mode"
	// This usually means that we can go more aggressive with resources used by the loading system
	Bool	m_isLoadingMode;

	// Helper token
	struct AsyncFileInfo
	{
		IAsyncFile*		m_asyncFile;
		CDiskFile*		m_depotFile;
	};

	// Resolve loading priority
	static Uint8 ResolveIOTag( const EResourceLoadingPriority priority );
};

//---

typedef TSingleton< CResourceLoader > SResourceLoader;

//---

#endif // __RESOURCE_LOADING_H__