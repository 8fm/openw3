/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "directory.h"
#include "dynarray.h"
#include "hashmap.h"
#include "handleMap.h"
#include "sortedMap.h"
#include "resource.h"
#include "dependencyCache.h"
#include "contentListener.h"
#include "commandLineParams.h"
#include "hashset.h"

#undef FindResource

class CJobLoadResource;
class CDepotBundles;
class CDiskBundle;
class CDiskFile;
class CXMLReader;

// internal hash32->disk file map
typedef Uint64 TDepotFileHash;
typedef TSortedMap< TDepotFileHash, CDiskFile*, DefaultCompareFunc< TDepotFileHash >, MC_Depot >		TDepotFileMap;

///////////////////////////////////////////////////////////////////////////////////////////

// The depot
class CDepot : public CDirectory, public IContentListener
{
public:
	enum AsyncResourceState
	{
		ARS_FAILED,
		ARS_LOADING,
		ARS_LOADED
	};

	CDepot( const String &rootDataPath );
	RED_MOCKABLE ~CDepot();

	// Toggle big fat error messages on missing assets (good for cook debugging)
	void ToggleMissingFileMessages( Bool flag );

	// Allow/Disallow fallback resources to be created
	void EnableFallbackResources( const Bool useFallbackResources );

	// Initialize the depot
	Bool Initialize( const Core::CommandLineArguments& coreArguments );

	// Add empty proxy file to the depot
	// Always creates a loose file representation
	// Will create needed directories
	CDiskFile* CreateNewFile( const Char* fileDepotPath );

	// Find file in depot, does not use links
	RED_MOCKABLE CDiskFile* FindFile( const String& path ) const;
	CDiskFile* FindFile( const Char* path ) const;

	// Find file in depot using precomputed path hash - use only in read ony mode
	CDiskFile* FindFile( const TDepotFileHash hash ) const;

	// Find file in depot, uses links
	CDiskFile* FindFileUseLinks( const String& path, Uint32 recurencyLevel );

	// Find already loaded resource
	THandle< CResource > FindResource( const String& depotFileName );

	// Load resource from serialized data
	THandle< CResource > LoadResource( const String& depotFileName, Bool useLinks = true );

	// Load resource from serialized data
	THandle< CResource > LoadResource( const String& depotFileName, ResourceLoadingContext& context, Bool useLinks = true );

	// Load resource asynchronously from serialized data
	THandle< CResource > LoadResourceAsync( const String& depotFileName );

	// Load depot file into XML
	CXMLReader* LoadXML( const String& depotFileName );

	// Get the state of async resource loading and return the resource if loaded
	AsyncResourceState GetAsyncResource( const String& depotFileName, THandle< CResource >& resource );

	// Get mapped disk file for given dependency cache index
	CDiskFile* GetMappedDiskFile( const Uint32 dependencyCacheIndex ) const;

	// Map disk file to internal dependency cache index
	const Uint32 MapFileToDependencyCache( CDiskFile* file );

	// Remove disk file from dependency mapping
	void UnmapFileFromDependencyCache( CDiskFile* file );

	// Check if a file exists in depot
	RED_MOCKABLE bool FileExist( const String& path );

	//! Get depot root data path
	RED_INLINE const String& GetRootDataPath() const { return m_rootDataPath; }

	//! Should we log resource errors ?
	RED_INLINE const Bool IsReportingResourceErrors() const { return m_strictErrorReporting; }

	//! Is running from bundles ?
	RED_INLINE const Bool IsUsingBundles() const { return (m_bundles != nullptr); }

	//! Get the bundle related interface
	RED_INLINE CDepotBundles* GetBundles() const { return m_bundles; }

	//! Get dependency cache
	RED_INLINE const CDependencyCache& GetDependencyCache() const { return m_dependencyCache; }

public:
	//! Remap to different path
	virtual void Remap( const String& newAbsolutePath, const Bool forceRescan = true ) override final;

	//! Get absolute directory path
	virtual void GetAbsolutePath( String& str ) const;

	//! Get directory depot path
	virtual void GetDepotPath( String& str ) const;

protected:
	typedef TDynArray< CDiskFile*, MC_Depot >			TIndexedDiskFiles;

	// Resolve alias path
	static const Char* ResolvePath( const Char* path );

	// Content listener pipeline
	virtual const Char* GetName() const override;
	virtual void OnContentAvailable( const SContentInfo& contentInfo ) override;

	// Create fallback resource
	CResource* GetFallbackResource( const String& path );

	// Create fallback file
	CDiskFile* GetFallbackFile( const String& path );

	String									m_rootDataPath;
	THashMap< String, CJobLoadResource* >	m_loadResourceJobs;

	TDynArray< CResource* >					m_fallbackResources;

	TIndexedDiskFiles						m_dependencyDiskFileMapping;
	CDependencyCache						m_dependencyCache;

	mutable TDepotFileMap					m_fileMap;
	mutable Bool							m_depotHashed;

	Bool									m_enableFallbackResources;
	Bool									m_strictErrorReporting;

	// single entry point integration between bundles and depot
	CDepotBundles*							m_bundles;

	THashSet<String>						m_patchedDepCashes;
};

///////////////////////////////////////////////////////////////////////////////////////////

// The depot instance
extern CDepot *GDepot;

///////////////////////////////////////////////////////////////////////////////////////////

// Load resource
template< class T >
THandle< T > LoadResource( const String& depotFileName )
{
	THandle< CResource > obj = GDepot->LoadResource( depotFileName );
	return Cast< T >( obj );
}

// Load resource
template< class T >
THandle< T > LoadResource( const String& depotFileName, ResourceLoadingContext& context )
{ 
	THandle< CResource > obj = GDepot->LoadResource( depotFileName, context );
	return Cast< T >( obj );
}

///////////////////////////////////////////////////////////////////////////////////////////

class IDeferredInitDelegate
{
public:
	virtual ~IDeferredInitDelegate() {}
	virtual void OnPostContentManagerInit() {}
	virtual void OnBaseEngineInit() {}
};

class CDeferredInit
{
public:

	CDeferredInit(const String& dataPath)
		: mAbsolutePath(dataPath)
	{}

	// deletes all the delegates
	~CDeferredInit();

	// register a callback delegate for after ContentManager is initialised
	void AddDelegate(IDeferredInitDelegate*);

	// Trigger delegate callbacks
	void OnBaseEngineInit();
	void OnPostContentManagerInit();

	// cache this for handy access while GDepot is not created...
	const String& DataPath() const { return mAbsolutePath; }

private:
	TDynArray<IDeferredInitDelegate*> mContentManagerInitDelegates;
	String mAbsolutePath;
};

extern CDeferredInit* GDeferredInit;

