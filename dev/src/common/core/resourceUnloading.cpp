/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "diskFile.h"
#include "resource.h"
#include "resourceUnloading.h"
#include "configFileManager.h"
#include "objectReachability.h"
#include "objectDiscardList.h"
#include "objectMap.h"
#include "configVar.h"

//----

namespace Config
{
	TConfigVar<Int32, Validation::IntRange<1,10> > cvMaxCleanupIterations( "Memory/Unloader", "MaxCleanupIterations", 10 );
	TConfigVar<Int32, Validation::IntRange<1,INT_MAX> > cvAutoPurgeDelay( "Memory/Unloader", "AutoPurgeDelay", 30 );
	TConfigVar<Int32, Validation::IntRange<0,INT_MAX> > cvGracePeriod( "Memory/Unloader", "GracePeriod", 30 );
	TConfigVar<Bool> cvEnableValidation( "Memory/Unloader", "EnableValidation", false );
	TConfigVar<Bool> cvEnableLeakCheck( "Memory/Unloader", "EnableLeakCheck", false );
	TConfigVar<Bool> cvEnableUnloading( "Memory/Unloader", "EnableUnloading", false );
}

//----

CResourceUnloader::CResourceUnloader()
	: m_reachability( new CObjectReachability() ) // helper class, contains gory details :)
	, m_unreachables( new CFastObjectList() )
{
}

CResourceUnloader::~CResourceUnloader()
{
	delete m_reachability;
	delete m_unreachables;
}

void CResourceUnloader::EnableAutomaticPurge( const Bool isEnabled )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Resource unloader functions MUST be called from main thread" );
	if ( !::SIsMainThread() ) return;

	if ( Config::cvEnableUnloading.Get() != isEnabled )
	{
		Config::cvEnableUnloading.Set( isEnabled );
		LOG_CORE( TXT("Auto resource purge: %s"), isEnabled ? TXT("ENABLED") : TXT("DISABLED") );
	}
}

void CResourceUnloader::Update()
{
	PC_SCOPE( AutoPurge );

	RED_FATAL_ASSERT( ::SIsMainThread(), "Resource unloader functions MUST be called from main thread" );
	if ( !::SIsMainThread() ) return;

	// is auto purge enabled ?
	if ( Config::cvEnableUnloading.Get() && Config::cvAutoPurgeDelay.Get() > 0 )
	{
		m_currentFrame += 1;

		// time for a cleansing ? ;]
		if ( m_currentFrame >= m_nextAutoPurge )
		{
			m_nextAutoPurge = m_currentFrame + Config::cvAutoPurgeDelay.Get();
			Purge( false );
		}
	}
}

void CResourceUnloader::ForcePurge()
{
	PC_SCOPE( ForcePurge );

	RED_FATAL_ASSERT( ::SIsMainThread(), "Resource unloader functions MUST be called from main thread" );
	if ( !::SIsMainThread() ) return;

	// time the performance of the force purge
	{
		CTimeCounter timer;
		Purge( true );

		LOG_CORE( TXT("FORCED PURGE: Took %1.2f ms"), timer.GetTimePeriodMS() );
	}
}

void CResourceUnloader::ValidatePurge( TDynArray< CDiskFile* >& purgeList ) const
{
	// prepare the unreachable list - zero it then set 1 for each VALID object
	const Uint32 maxObjectIndex = GObjectsMap->GetMaxObjectIndex(); // may be not 100% thread safe
	m_unreachables->ResetToOnes( maxObjectIndex ); // marked all entries as UNREACHABLE - we will be testing selected few anyway

	// create the initial seed list of object by copying the root set
	m_reachability->InitializeFromRootSet( maxObjectIndex );

	// propagate reachability flag, clearing the unreachable flags on the objects that were visited
	const Bool useMultiThreadedImplementation = false; // TODO
	m_reachability->CollectUnreachables( m_unreachables, useMultiThreadedImplementation );

	// ok - let's test if the resources we are about to discard are still reachable
	for ( Uint32 i=0; i<purgeList.Size(); ++i )
	{
		CDiskFile* file = purgeList[i];

		CResource* object = file->m_resource; // DIRECT ACCESS 
		RED_FATAL_ASSERT( object != nullptr, "Lies, lies, everybody lies" );

		const Uint32 objectIndex = object->GetObjectIndex();
		if ( !m_unreachables->IsSet( objectIndex ) ) // object is reachable
		{
			ERR_CORE( TXT("!!! RESOURCE PURGE VALIDATION FAILED !!!" ) );
			ERR_CORE( TXT("Resource '%ls' is still rechable"), file->GetDepotPath().AsChar() );

			// TODO: dump detailed info where this resource is used

			// prevent this resource from being purged
			purgeList[i] = nullptr;
		}
	}
}

namespace Helpers
{
	class PerClassLeakStatus
	{
	public:
		PerClassLeakStatus()
		{
			Reset();
		}

		void Reset()
		{
			const Uint32 numClasses = SRTTI::GetInstance().GetIndexedClasses().Size();
			m_histogram.Resize( numClasses );

			for ( Uint32 i=0; i<numClasses; ++i )
				m_histogram[i] = 0;

			m_numNonZeroClasses = 0;
		}

		void Count( const CClass* theClass )
		{
			Uint32& val = m_histogram[ theClass->GetClassIndex() ];
			if ( val++ == 0 )
				m_numNonZeroClasses += 1;
		}

		void Dump()
		{
			if ( !m_numNonZeroClasses )
				return;

			TDynArray< ClassEntry > entries;
			entries.Reserve( m_numNonZeroClasses );

			for ( Uint32 i=0; i<m_histogram.Size(); ++i )
			{
				if ( m_histogram[i] )
				{
					ClassEntry entry;
					entry.m_class = SRTTI::GetInstance().GetIndexedClasses()[i];
					entry.m_count = m_histogram[i];
					entries.PushBack( entry );
				}
			}

			Sort( entries.Begin(), entries.End(), [](const ClassEntry& a, const ClassEntry& b) { return a.m_count > b.m_count; } );

#ifdef RED_LOGGING_ENABLED
			ERR_CORE( TXT("%d leaking classes"), m_numNonZeroClasses );
			for ( const ClassEntry& entry : entries )
			{
				ERR_CORE( TXT("  %6i: %ls"), 
					entry.m_count,
					entry.m_class->GetName().AsChar() );
			}
#endif
		}

	private:
		struct ClassEntry
		{
			const CClass*		m_class;
			Uint32				m_count;
		};

		TDynArray< Uint32 >		m_histogram;
		Uint32					m_numNonZeroClasses;
	};
}

void CResourceUnloader::ReportLeaks() const
{
	// prepare the unreachable list - zero it then set 1 for each VALID object
	const Uint32 maxObjectIndex = GObjectsMap->GetMaxObjectIndex(); // may be not 100% thread safe
	m_unreachables->ResetToZeros( maxObjectIndex ); // no object is considered unreachable yet

	// mark only existing objects as unreachable
	GObjectsMap->VisitAllObjectsNoFilter( 
		[&](const CObject* , const Uint32 index)
		{
			m_unreachables->Set(index);
		}
	);

	// create the initial seed list of object by copying the root set
	m_reachability->InitializeFromRootSet( maxObjectIndex );

	// add all resources that are not released
	TDynArray< CObject* > resourcesWithHandles;
	GObjectsMap->VisitAllObjectsNoFilter( [&]( CObject* object, const Uint32 )
		{
			// is this a live resource that is not in the root set ?
			if ( !object->IsInRootSet() && !object->IsHandleProtected() && object->IsA< CResource >() )
			{
				CObject* ptr = object;
				m_reachability->InitializeFromObjects( &ptr, 1 );
			}
		}
	);

	// propagate reachability flag, clearing the unreachable flags on the objects that were visited
	const Bool useMultiThreadedImplementation = false; // TODO
	m_reachability->CollectUnreachables( m_unreachables, useMultiThreadedImplementation );

	// per class leak stats
	Helpers::PerClassLeakStatus perClassHistogram;

	// report leaked objects
	Uint32 numLeakedObjects = 0;
	Uint32 numLeakedMemory = 0;
	{
		CObjectsMap::ObjectIndexer indexer( GObjectsMap );
		m_unreachables->VisitSet( maxObjectIndex,
			[&](const Uint32 index)
			{
				CObject* object = indexer.GetObject(index);
				if ( object )
				{
					numLeakedObjects += 1;
					if ( numLeakedObjects < 100 )
					{
						ERR_CORE( TXT("Leak[%d] 0x%LLX, %ls, %ls"),
							numLeakedObjects, (Uint64) object,
							object->GetClass()->GetName().AsChar(),
							object->GetFriendlyName().AsChar() );
					}

					perClassHistogram.Count( object->GetClass() );
					numLeakedMemory += object->GetClass()->GetSize() + object->GetClass()->GetScriptDataSize();
				}
			}
		);
	}

	// total leak count
	if ( numLeakedObjects > 0 )
	{
		ERR_CORE( TXT("Leaked %d objects, %1.2fKB in direct data"), 
			numLeakedObjects, (Float)numLeakedMemory / 1024.0f );

		perClassHistogram.Dump();
	}
}

void CResourceUnloader::Purge( const Bool full )
{
	// settings
	const Bool leakCheck = Config::cvEnableLeakCheck.Get();
	const Bool validate = Config::cvEnableValidation.Get();

	// iterative cleanup
	const Uint32 numIterations = Config::cvMaxCleanupIterations.Get();
	for ( Uint32 i=0; i<numIterations; ++i )
	{
		// get objects to clear - the quarantine 
		const Uint32 frameCutoff = CDiskFile::st_frameIndex - Config::cvGracePeriod.Get(); // allow objects to live for some time
		CDiskFile::st_quarantine.LockQuarantine( frameCutoff, m_purgatory );

		// nothing new in the quarantine
		if ( m_purgatory.Empty() )
			break;

		// validation: make sure that objects to discard are not referenced
		if ( validate )
			ValidatePurge( m_purgatory );

		// discard objects from purgatory
		for ( CDiskFile* file : m_purgatory )
		{
			if ( file ) // may be removed by validation
				file->Purge();		
		}

		// full purge - make sure the objects are discarded
		// TODO: once we get rid of the Discard() and discard list bullshit this will not be needed
		if ( full )
			GObjectsDiscardList->ProcessList( true );
	}

	// unlock the quarantine data
	CDiskFile::st_quarantine.UnlockQuarantine();

	// check the leaks - report any objects that could be freed but were not
	if ( leakCheck )
		ReportLeaks();
}