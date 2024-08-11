/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "string.h"
#include "hashMap.h"
#include "resourceid.h"
#include "bitset.h"

class CDependencyCollector;
class CDependencyCacheMapping;
class CDependencyCache;

/// Dependency cache list
class CDependencyCache
{
public:
	CDependencyCache();
	~CDependencyCache();

	// Magic number & file version
	static const Uint32 FILE_MAGIC = 'DEPS';
	static const Uint32 FILE_VERSION = 2;

	// Resource ID
	typedef Red::Core::ResourceManagement::CResourceId	ResourceID;

	//---

	// Load data from file - the whole cache is read only
	Bool Load( const String& absoluteFilePath );

	//---

	// Allocate dependency collector to use with this cache
	CDependencyCollector* AllocateCollector() const;

	// Collect the dependencies of given file index (thread safe method)
	// New dependencies + the file to load are added to the dependency collector
	void CollectDependencies( class CDependencyCollector& outCollector, const Uint32 rootFileIndex, const Bool isRoot = true ) const;

	//---

	/// Get number of resources in the dependency cache
	const Uint32 GetNumResources() const;

	/// Lookup internal index for given resource ID - will return 0 if not found
	const Uint32 MapResourceToIndex( const ResourceID& id ) const;

	/// Lookup resource ID for given internal index
	const ResourceID MapIndexToResource( const Uint32 index ) const;

	//---

	/// Append dynamic content dependency cache (EP1, EP2, etc - big DLCs)
	void AppendDynamicCache( const CDependencyCache& cache );

private:
	typedef TDynArray< Uint32, MC_Depot >				TIndices;
	typedef TDynArray< ResourceID, MC_Depot >			TResources;
	typedef THashMap< ResourceID, Uint32, DefaultHashFunc< ResourceID >, DefaultEqualFunc< ResourceID >, MC_Depot >		TResourceMap;

	// dependency data
	TIndices		m_dependencyIndices; // index into the dependency buffer for normal dependencies
	TIndices		m_dependencyExcluded; // index into the dependency buffer for excluded dependencies
	TIndices		m_dependencyBuffer; // list of file dependencies

	// resource mapping 
	TResources		m_resources;
	TResourceMap	m_resourceMap;

	// static list of free internal state structures
	mutable CDependencyCollector*	m_freeList;
	mutable Red::Threads::CMutex	m_freeListLock;

	// lock used when dependency cache is resized
	mutable Red::Threads::CRWLock	m_resizeLock;

	// Collect the dependencies of given file index (thread safe method)
	// New dependencies + the file to load are added to the dependency collector
	void CollectDependencies_NoLock( class CDependencyCollector& outCollector, const Uint32 rootFileIndex, const Bool isRoot = true ) const;
	

	friend class CDependencyCollector;
};

///----

/// Dependency collector - sorted list of file indices
class CDependencyCollector
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_Depot );

public:
	// Release the collector
	void Release();
	
	// exclude file
	RED_FORCE_INLINE void Exclude( const Uint32 index )
	{
		m_excludedFiles.Set( index );
	}

	// unexclude file
	RED_FORCE_INLINE void Unexclude( const Uint32 index )
	{
		m_excludedFiles.Clear( index );
	}

	// visit the file - returns true if file is visited for the first time
	RED_FORCE_INLINE void Mark( const Uint32 index )
	{
		m_visitedFiles.Set( index );
	}

	// remove the bit for given file entry
	RED_FORCE_INLINE void Unmark( const Uint32 index )
	{
		m_visitedFiles.Clear( index );
	}

	// is the file marked ?
	RED_FORCE_INLINE bool IsMarked( const Uint32 index ) const
	{
		return m_visitedFiles.Get( index );
	}

	// is the file excluded ?
	RED_FORCE_INLINE bool IsExcluded( const Uint32 index ) const
	{
		return m_excludedFiles.Get( index );
	}

	// add file to internal list
	RED_FORCE_INLINE void Add( const Uint32 index )
	{
		m_fileList.PushBack( index );
	}

	// get number of files in the list
	RED_FORCE_INLINE const Uint32 Size() const
	{
		return m_fileList.Size();
	}

	// get n-th file to load
	RED_FORCE_INLINE const Uint32 GetFileIndex( const Uint32 index ) const
	{
		return m_fileList[ index ];
	}

private:
	CDependencyCollector( CDependencyCache& cache );
	~CDependencyCollector();

	// files visited and collected during dependency walking
	BitSet64Dynamic		m_visitedFiles;

	// files excluded from being the dependencies (for example: inline files)
	BitSet64Dynamic		m_excludedFiles;

	// file list - sorted load list
	typedef TDynArray< Uint32, MC_Depot >		TFileList;
	TFileList			m_fileList;

	// free list pointer
	CDependencyCollector*	m_nextFree;
	CDependencyCache*		m_cache;

	friend class CDependencyCache;
};

///----
