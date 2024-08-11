/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/collisionCache.h"
#include "../../common/engine/collisionCacheReadOnly.h"

/// Async collision cache validation tool - checks data format integrity
class CTestCollisionCacheCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CTestCollisionCacheCommandlet, ICommandlet, 0 );

public:
	CTestCollisionCacheCommandlet();
	~CTestCollisionCacheCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Test asynchrnous collision cache access"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;

private:
	String				m_path;
	ICollisionCache*	m_cache;
};

BEGIN_CLASS_RTTI( CTestCollisionCacheCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CTestCollisionCacheCommandlet );

CTestCollisionCacheCommandlet::CTestCollisionCacheCommandlet()
{
	m_commandletName = CName( TXT("testcollisioncache") );
}

CTestCollisionCacheCommandlet::~CTestCollisionCacheCommandlet()
{
}

bool CTestCollisionCacheCommandlet::Execute( const CommandletOptions& options )
{
	// get the path to collision cache
	if ( !options.GetSingleOptionValue( TXT("file"), m_path ) )
	{
		ERR_WCC( TXT("Expecting path to collision cache") );
		return false;
	}

	// should we scramble the order
	Bool scrambleReadOrder = false;
	if ( options.HasOption( TXT("scramble") ) )
	{
		LOG_WCC( TXT("Loading order will be scrambled") );
		scrambleReadOrder = true;
	}

	// setup inter-frame delay
	Uint32 frameDelay = 33;
	if ( options.HasOption( TXT("delay") ) )
	{
		String value;
		options.GetSingleOptionValue( TXT("delay"), value );
		FromString< Uint32 >( value, frameDelay );
	}

	// internal throttle - schedule no more than X per frame
	Uint32 throttle = 100;
	if ( options.HasOption( TXT("throttle") ) )
	{
		String value;
		options.GetSingleOptionValue( TXT("throttle"), value );
		FromString< Uint32 >( value, throttle );
	}

	// get the cache
	CCollisionCacheReadOnly* cache = static_cast< CCollisionCacheReadOnly* >( ICollisionCache::CreateReadOnlyCache( m_path ) );
	if ( !cache )
	{
		ERR_WCC( TXT("Failed to load collision cache from '%ls'"), m_path.AsChar() );
		return false;
	}

	// wait for cache to become alive
	{
		CTimeCounter waitTimer;

		for ( ;; )
		{
			if ( cache->GetNumEntries_Debug() > 0 )
				break;

			Red::Threads::YieldCurrentThread();
		}

		LOG_WCC( TXT("Waiting for cache to initialize: %1.2fms"), waitTimer.GetTimePeriodMS() );
	}

	// create the tempshit data holders
	TDynArray< CompiledCollisionPtr > loadedCollisions;
	TDynArray< Bool > invalidCollisions;
	loadedCollisions.Resize( cache->GetNumEntries_Debug() );
	invalidCollisions.Resize( cache->GetNumEntries_Debug() );

	// randomize loading order
	TDynArray< Uint32 > loadingOrder;
	loadingOrder.Resize( cache->GetNumEntries_Debug() );
	for ( Uint32 i=0; i<loadingOrder.Size(); ++i )
	{
		loadingOrder[i] = i;
	}

	// scramble the list really well
	if ( scrambleReadOrder )
	{
		for ( Uint32 i=0; i<(loadingOrder.Size()*2); ++i )
		{
			const Uint32 a = (rand() * loadingOrder.Size()) / RAND_MAX;
			const Uint32 b = (rand() * loadingOrder.Size()) / RAND_MAX;
			if ( a != b )
			{
				Swap( loadingOrder[a], loadingOrder[b] ); 
			}
		}
	}

	// test loading all collisions
	{
		CTimeCounter loadingTimer;
		CTimeCounter frameTimer;
		Uint32 numFrames = 0;
		Uint32 numLoadedTotal = 0;
		for ( ;; )
		{
			Uint32 numLoaded = 0;
			Uint32 numStarted = 0;
			Bool everythingLoaded = true;
			for ( Uint32 i=0; i<loadedCollisions.Size(); ++i )
			{
				// invalid
				if ( invalidCollisions[i] )
					continue;

				// data not loaded, try to get it
				if ( !loadedCollisions[i] )
				{
					everythingLoaded = false;

					const String path = cache->GetEntryPath_Debug( loadingOrder[i] );

					CompiledCollisionPtr mesh;
					ICollisionCache::EResult result = cache->FindCompiled( mesh, path, Red::System::DateTime() );
					if ( result == ICollisionCache::eResult_Valid )
					{
						loadedCollisions[i] = mesh;
						numLoaded += 1;
					}
					else if ( result == ICollisionCache::eResult_Invalid )
					{
						invalidCollisions[i] = true;
						ERR_WCC( TXT("Collision data for '%ls' failed to load"), path.AsChar() );
					}
					else
					{
					}

					// throttling
					numStarted += 1;
					if ( throttle && numStarted >= throttle )
					{
						everythingLoaded = false;
						break;
					}
				}
			}

			// track loading
			if ( numLoaded )
			{
				LOG_WCC( TXT("Load count (%d): %d->%d in %1.3fms"), numLoaded, numLoadedTotal, numLoadedTotal+numLoaded, frameTimer.GetTimePeriodMS() );
				numLoadedTotal += numLoaded;
			}

			if ( everythingLoaded )
				break;

			frameTimer.ResetTimer();

			// wait
			if ( frameDelay )
				Red::Threads::SleepOnCurrentThread( frameDelay );
			else
				Red::Threads::YieldCurrentThread();

			++numFrames;
		}

		LOG_WCC( TXT("All collision from cache loaded in %1.2fms (%d frames)"), loadingTimer.GetTimePeriodMS(), numFrames );
	}

	// release memory
	loadedCollisions.Clear();

	// close cache
	delete cache;

	// done
	return true;
}

void CTestCollisionCacheCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  testcollisioncache -file=<file.cache>") );
}
