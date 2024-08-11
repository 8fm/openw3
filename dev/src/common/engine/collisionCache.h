/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/contentListener.h"
#include "../core/lazyCacheChain.h"

#include "collisionContent.h"
#include "../physics/compiledCollision.h"

/// Collision cache interface
class ICollisionCache
{
public:
	virtual ~ICollisionCache() {};

	// Operation result
	enum EResult
	{
		eResult_NotReady=-1,		//!< Operation is still being processed, you must wait
		eResult_Invalid=0,			//!< Operation failed (and/or) result is invalid
		eResult_Valid=1,			//!< Operation succeeded (and/or) result is valid
	};

	//! Invalidate collision for given resource in the cache
	virtual void InvalidateCollision( const String& name ) = 0;

	//! Find collision data in cache and load collision data for it
	//! This is an asynchronous operation - it can return eResult_NotReady if the data is not yet available
	virtual EResult FindCompiled( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time, Box2* bounds = nullptr ) = 0;

	//! Check if we have collision for given mesh
	//! This is an asynchronous operation - it can return eResult_NotReady if the data is not yet available
	virtual EResult HasCollision( const String& name, Red::System::DateTime time ) const = 0;

	//! Compile new collision mesh for given ICollisionContent
	//! This is an asynchronous operation - it can return eResult_NotReady if the data is not yet available
	virtual EResult Compile( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time, CObject* sourceObject = nullptr ) = 0;

	//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
	virtual void Flush() = 0;

	//! Generate runtime statistics
	virtual class C2dArray* DumpStatistics( Bool createIfDoesntExist = true ) = 0;

public:
	//! Deprecated SYNC API, only returns true (eResult_Valid) or false (eResult_Invalid)
	//! NOTE: this can cause MAJOR stalls on whatever thread it's being called from
	const Bool FindCompiled_Sync( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time );
	const Bool HasCollision_Sync( const String& name, Red::System::DateTime time ) const;
	const Bool Compile_Sync( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time, CObject* sourceObject = nullptr );

public:
	// Create read only collision cache
	static ICollisionCache* CreateReadOnlyCache( const String& absolutePath );

	// Create runtime read/write collision cache
	static ICollisionCache* CreateReadWriteCache( const String& absolutePath );

	// Create NULL collision cache - will silently fail all requests - saves us from having an if ( GCoOllisionCache ) in few places
	static ICollisionCache* CreateNullCache();

protected:
	//! Initialize collision cache at given file, can return false if specific file cannot be opened
	virtual Bool Initialize( const String& absoluteFilePath ) = 0;
};

class ICollisionCacheCallback
{
public:

	virtual ~ICollisionCacheCallback(){}
	virtual void OnCompiledCollisionFound( CompiledCollisionPtr collision ) = 0;
	virtual void OnCompiledCollisionInvalid() = 0;
};

//////////////////////////////////////////////////////////////////////////
/// Resolver for collision caches
class CCollisionCacheResolver : public IContentListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	typedef ICollisionCache::EResult EResult;

public:
	static const Uint32 MAX_CACHE_CHAIN_LENGTH = 64;
	
public:
	CCollisionCacheResolver();
	virtual ~CCollisionCacheResolver();
	void Shutdown();

	//! Invalidate collision for given resource in the cache
	void InvalidateCollision( const String& name );

	//! Find collision data in cache and load collision data for it
	//! This is an asynchronous operation - it can return eResult_NotReady if the data is not yet available
	EResult FindCompiled( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time, Box2* bounds = nullptr );

	//! Check if we have collision for given mesh
	//! This is an asynchronous operation - it can return eResult_NotReady if the data is not yet available
	EResult HasCollision( const String& name, Red::System::DateTime time ) const;

	//! Compile new collision mesh for given ICollisionContent
	//! This is an asynchronous operation - it can return eResult_NotReady if the data is not yet available
	EResult Compile( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time );

	//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
	void Flush();

	//! Generate runtime statistics
	class C2dArray* DumpStatistics( Bool createIfDoesntExist = true );

	//! 
	EResult FindCompiled_Async( ICollisionCacheCallback * callback, const String& name, Red::System::DateTime time, Box2* bounds = nullptr );
	void CancelFindCompiled_Async( ICollisionCacheCallback * callback );
	void Tick();

public:
	//! Deprecated SYNC API, only returns true (eResult_Valid) or false (eResult_Invalid)
	//! NOTE: this can cause MAJOR stalls on whatever thread it's being called from
	const Bool FindCompiled_Sync( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time );
	const Bool HasCollision_Sync( const String& name, Red::System::DateTime time ) const;
	const Bool Compile_Sync( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time, CObject* sourceObject = nullptr );

private:
	virtual const Char* GetName() const override { return TXT("CCollisionCacheResolver"); }
	virtual void OnContentAvailable( const SContentInfo& contentInfo ) override;

private:
	typedef Helper::CLazyCacheChain< ICollisionCache, MAX_CACHE_CHAIN_LENGTH > CacheChain;
	
	struct CompiledCollisionQuery
	{
		ICollisionCacheCallback * callback;
		String name;
		Red::System::DateTime time;
		Box2 * bounds;
	};

	typedef TDynArray< CompiledCollisionQuery > QueryContainer;
	
	CacheChain m_collisionCacheChain;
	QueryContainer m_queryContainer;
	Red::Threads::CMutex m_mutex;
};

//-------------------------------------------------

extern CCollisionCacheResolver* GCollisionCache;

//-------------------------------------------------