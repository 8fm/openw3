/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/depot.h"
#include "collisionCache.h"
#include "collisionCacheBuilder.h"
#include "collisionCacheReadOnly.h"
#include "../core/contentManifest.h"

//----

/// Null collision cache implementation
class CCollisionCacheNull : public ICollisionCache
{
public:
	virtual ~CCollisionCacheNull() {};

	virtual Bool Initialize( const String& ) 
	{
		// always successful
		return true;
	}

	virtual void InvalidateCollision( const String& )
	{
		// not used
	}

	virtual EResult FindCompiled( CompiledCollisionPtr&, const String&, Red::System::DateTime, Box2* bounds )
	{
		// always fail
		return ICollisionCache::eResult_Invalid;
	}

	virtual EResult HasCollision( const String&, Red::System::DateTime ) const
	{
		// always fail
		return ICollisionCache::eResult_Invalid;
	}

	virtual EResult Compile( CompiledCollisionPtr& result, const ICollisionContent* context, const String&, Red::System::DateTime, CObject* sourceObject ) 
	{
		// compile on the spot
		if ( context )
		{
			result = context->CompileCollision( nullptr );
			if ( result )
			{
				return ICollisionCache::eResult_Valid;
			}
		}

		// always fail
		return ICollisionCache::eResult_Invalid;
	}

	virtual void Flush()
	{
		// not used
	}

	virtual class C2dArray* DumpStatistics( Bool createIfDoesntExist ) override
	{
		// not used
		return nullptr;
	}
};

//----

// the default implementation of collision cache is NULL collision cache
CCollisionCacheNull GNullCollisionCache;

//----

const Bool ICollisionCache::FindCompiled_Sync( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time )
{
	CTimeCounter timer;
	Bool stalled = false;
	for ( ;; )
	{
		const EResult callResult = FindCompiled( result, name, time );
		if ( callResult != eResult_NotReady )
		{
			if ( stalled )
			{
				ERR_ENGINE( TXT("Stall due to collision not being ready: %1.2fms, result: %d"), timer.GetTimePeriodMS(), callResult );
			}

			return (callResult == eResult_Valid);
		}

		Red::Threads::YieldCurrentThread();
		stalled = true;
	}
}

const Bool ICollisionCache::HasCollision_Sync( const String& name, Red::System::DateTime time ) const
{
	CTimeCounter timer;
	Bool stalled = false;
	for ( ;; )
	{
		const EResult callResult = HasCollision( name, time );
		if ( callResult != eResult_NotReady )
		{
			if ( stalled )
			{
				ERR_ENGINE( TXT("Stall due to collision not being ready: %1.2fms, result: %d"), timer.GetTimePeriodMS(), callResult );
			}

			return (callResult == eResult_Valid);
		}

		Red::Threads::YieldCurrentThread();
		stalled = true;
	}
}

const Bool ICollisionCache::Compile_Sync( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time, CObject* sourceObject )
{
	CTimeCounter timer;
	Bool stalled = false;
	for ( ;; )
	{
		const EResult callResult = Compile( result, content, name, time, sourceObject );
		if ( callResult != eResult_NotReady )
		{
			if ( stalled )
			{
				ERR_ENGINE( TXT("Stall due to collision not being ready: %1.2fms, result: %d"), timer.GetTimePeriodMS(), callResult );
			}

			return (callResult == eResult_Valid);
		}

		Red::Threads::YieldCurrentThread();
		stalled = true;
	}
}

ICollisionCache* ICollisionCache::CreateReadOnlyCache( const String& absolutePath )
{
	ICollisionCache* cache = new CCollisionCacheReadOnly();
	if ( cache->Initialize( absolutePath ) )
	{
		return cache;
	}

	// failed to open the cache
	delete cache;
	return nullptr;
}

ICollisionCache* ICollisionCache::CreateReadWriteCache( const String& absolutePath )
{
	ICollisionCache* cache = new CCollisionCacheBuilder();
	if ( cache->Initialize( absolutePath ) )
	{
		return cache;
	}

	// failed to open the cache
	delete cache;
	return nullptr;
}

ICollisionCache* ICollisionCache::CreateNullCache()
{
	return new CCollisionCacheNull();
}

CCollisionCacheResolver::CCollisionCacheResolver()
{
}

CCollisionCacheResolver::~CCollisionCacheResolver()
{
}

void CCollisionCacheResolver::Shutdown()
{
	for ( ICollisionCache* cache : m_collisionCacheChain )
	{
		delete cache;
	}
}

void CCollisionCacheResolver::InvalidateCollision( const String& name )
{
	for ( ICollisionCache* cache : m_collisionCacheChain )
	{
		cache->InvalidateCollision( name );
	}
}

CCollisionCacheResolver::EResult CCollisionCacheResolver::FindCompiled( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time, Box2* bounds )
{
	EResult opResult = EResult::eResult_Invalid;
	for ( ICollisionCache* cache : m_collisionCacheChain )
	{
		opResult = cache->FindCompiled( result, name, time, bounds );
		if ( opResult == EResult::eResult_Valid )
		{
			break;
		}
		else if ( opResult == EResult::eResult_NotReady )
		{
			break;
		}
	}

	return opResult;
}

CCollisionCacheResolver::EResult CCollisionCacheResolver::HasCollision( const String& name, Red::System::DateTime time ) const
{
	EResult opResult = EResult::eResult_Invalid;
	for ( const ICollisionCache* cache : m_collisionCacheChain )
	{
		opResult = cache->HasCollision( name, time );
		if ( opResult == EResult::eResult_Valid )
		{
			break;
		}
		else if ( opResult == EResult::eResult_NotReady )
		{
			break;
		}
	}

	return opResult;
}

CCollisionCacheResolver::EResult CCollisionCacheResolver::Compile( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time )
{
	EResult opResult = EResult::eResult_Invalid;
	for ( ICollisionCache* cache : m_collisionCacheChain )
	{
		opResult = cache->Compile( result, content, name, time );
		if ( opResult == EResult::eResult_Valid )
		{
			break;
		}
		else if ( opResult == EResult::eResult_NotReady )
		{
			break;
		}
	}

	return opResult;
}

void CCollisionCacheResolver::Flush()
{
	for ( ICollisionCache* cache : m_collisionCacheChain )
	{
		cache->Flush();
	}
}

class C2dArray* CCollisionCacheResolver::DumpStatistics( Bool createIfDoesntExist )
{
	return nullptr;
}

const Bool CCollisionCacheResolver::FindCompiled_Sync( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time )
{
	Bool opResult = false;
	for ( ICollisionCache* cache : m_collisionCacheChain )
	{
		opResult = cache->FindCompiled_Sync( result, name, time );
		if ( opResult )
		{
			break;
		}
	}

	return opResult;
}

const Bool CCollisionCacheResolver::HasCollision_Sync( const String& name, Red::System::DateTime time ) const
{
	Bool opResult = false;
	for ( const ICollisionCache* cache : m_collisionCacheChain )
	{
		opResult = cache->HasCollision_Sync( name, time );
		if ( opResult )
		{
			break;
		}
	}

	return opResult;
}

const Bool CCollisionCacheResolver::Compile_Sync( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time, CObject* sourceObject )
{
	Bool opResult = false;
	for ( ICollisionCache* cache : m_collisionCacheChain )
	{
		opResult = cache->Compile_Sync( result, content, name, time, sourceObject );
		if ( opResult )
		{
			break;
		}
	}

	return opResult;
}

void CCollisionCacheResolver::OnContentAvailable( const SContentInfo& contentInfo )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Mounting off the main thread" );

	typedef ICollisionCache* (*CreateFunc)(const String&);
	const CreateFunc CreateCollisionCache = GFileManager->IsReadOnly() ? &ICollisionCache::CreateReadOnlyCache : &ICollisionCache::CreateReadWriteCache;

	typedef Bool (CacheChain::* PushFunc)(ICollisionCache*);

	for ( const SContentFile* contentFile : contentInfo.m_contentFiles )
	{
		const PushFunc Push = contentFile->m_isPatch ? &CacheChain::PushFront : &CacheChain::PushBack;

		const String collisionCacheFile = String::Printf( TXT("%ls%hs"), contentInfo.m_mountPath.AsChar(), contentFile->m_path.AsChar() );
		LOG_ENGINE(TXT("CCollisionCacheResolver: attaching '%ls'"), collisionCacheFile.AsChar() );

		ICollisionCache* cache = CreateCollisionCache( collisionCacheFile );
		if ( cache )
		{
			if ( ! (m_collisionCacheChain.*Push)( cache ) )
			{
				delete cache;
				ERR_ENGINE(TXT("Failed to create collision cache '%ls'. Reached chain length limit %u"), collisionCacheFile.AsChar(), MAX_CACHE_CHAIN_LENGTH );
				return;
			}
		}
		else
		{
			ERR_ENGINE( TXT("Failed to create collision cache '%ls'"), collisionCacheFile.AsChar() );
		}
	}
}

CCollisionCacheResolver::EResult CCollisionCacheResolver::FindCompiled_Async( ICollisionCacheCallback * callback, const String& name, Red::System::DateTime time, Box2* bounds )
{
	RED_FATAL_ASSERT( callback, "Null callback are not supported." );

	CompiledCollisionPtr compiledCollisionPtr;
	EResult result = FindCompiled( compiledCollisionPtr, name, time, bounds );

	if( result == ICollisionCache::eResult_Valid )
	{
		callback->OnCompiledCollisionFound( compiledCollisionPtr );
	}
	else
	{
		CompiledCollisionQuery query = 
		{
			callback,
			name,
			time,
			bounds
		};

		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
		m_queryContainer.PushBack( query );
	}

	return result;
}


void CCollisionCacheResolver::CancelFindCompiled_Async( ICollisionCacheCallback * callback )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	
	m_queryContainer.Erase( 
		RemoveIf( m_queryContainer.Begin(), m_queryContainer.End(), [=]( const CompiledCollisionQuery & query ){ return query.callback == callback; } ), 
		m_queryContainer.End() );
}

void CCollisionCacheResolver::Tick() 
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	
	struct Functor
	{
		Functor( CCollisionCacheResolver * resolver )
			: m_resolver( resolver )
		{}

		Bool operator()( const CompiledCollisionQuery & query )
		{
			CompiledCollisionPtr compiledCollision;
			EResult result = m_resolver->FindCompiled( compiledCollision, query.name, query.time, query.bounds );
			if( result == ICollisionCache::eResult_Valid )
			{
				query.callback->OnCompiledCollisionFound( compiledCollision );
			}
			else if( result == ICollisionCache::eResult_Invalid )
			{
				query.callback->OnCompiledCollisionInvalid();
			}

			return !( result == ICollisionCache::eResult_NotReady );
		}

		CCollisionCacheResolver * m_resolver;
	};

	m_queryContainer.Erase( 
		RemoveIf( m_queryContainer.Begin(), m_queryContainer.End(), Functor(this) ), 
		m_queryContainer.End() );

}

static CCollisionCacheResolver GCollisionCacheResolver;
CCollisionCacheResolver* GCollisionCache = &GCollisionCacheResolver;
