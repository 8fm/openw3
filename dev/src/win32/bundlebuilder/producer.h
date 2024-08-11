/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __RED_BUNDLE_BUILDER_PRODUCER_H__
#define __RED_BUNDLE_BUILDER_PRODUCER_H__

#include "../../common/redSystem/cpuid.h"

#include "../../common/core/core.h"
#include "../../common/core/bundledefinition.h"

#include "options.h"

namespace Bundler
{
	//////////////////////////////////////////////////////////////////////////
	// CProducer
	//////////////////////////////////////////////////////////////////////////
	template< typename TWorker, typename TPayload >
	class CProducer
	{
		typedef Red::Core::BundleDefinition::IBundleDefinition IBundleDefinition;

	public:
		CProducer( IBundleDefinition& bundleDefinition );
		virtual ~CProducer();

		RED_INLINE void Run( const COptions& options );

		void RunMultithreaded( const COptions& options );
		void RunSingleThreaded( const COptions& options );

	protected:
		virtual void Initialize( const COptions& options ) = 0;
		virtual void FillPayload( TPayload& payload, const StringAnsi& bundleName, Red::Core::BundleDefinition::CBundleDataContainer* bundleData ) = 0;
		virtual void Shutdown( const COptions& options ) = 0;

		virtual void OnWorkFinished( TWorker* ) {}

		// Undefined
		void operator=( const CProducer< TWorker, TPayload >& );

		IBundleDefinition& m_bundleDefinition;
	};

	template< typename TWorker, typename TPayload >
	CProducer< TWorker, TPayload >::CProducer( IBundleDefinition& bundleDefinition )
		:	m_bundleDefinition( bundleDefinition )
	{

	}

	template< typename TWorker, typename TPayload >
	CProducer< TWorker, TPayload >::~CProducer()
	{

	}

	template< typename TWorker, typename TPayload >
	RED_INLINE void Bundler::CProducer< TWorker, TPayload >::Run( const COptions& options )
	{
		if( options.IsSinglethreaded() )
		{
			RunSingleThreaded( options );
		}
		else
		{
			RunMultithreaded( options );
		}
	}

	template< typename TWorker, typename TPayload >
	void Bundler::CProducer< TWorker, TPayload >::RunSingleThreaded( const COptions& options )
	{
		using Red::Core::BundleDefinition::TBundleDataContainers;

		const TBundleDataContainers& bundleDataContainer( m_bundleDefinition.GetBundles() );

		Feedback::Initialize( bundleDataContainer.Size() );

		Initialize( options );

		// Create a worker, but don't activate the thread, just call the functions directly from the main thread
		TWorker worker( nullptr, options );

		Uint32 bundleIndex = 0;
		for( TBundleDataContainers::const_iterator iBundle = bundleDataContainer.Begin(); iBundle != bundleDataContainer.End(); ++iBundle )
		{
			TPayload payload;

			FillPayload( payload, iBundle->m_first, iBundle->m_second );

			worker.ActivateSingleThreaded( payload );

			OnWorkFinished( &worker );
			worker.ClearData();

			// Used for debugging, so it's easy to identify how far through processing the bundles we are
			++bundleIndex;
		}
	}

	template< typename TWorker, typename TPayload >
	void CProducer< TWorker, TPayload >::RunMultithreaded( const COptions& options )
	{
		using Red::Core::BundleDefinition::TBundleDataContainers;

		const TBundleDataContainers& bundleDataContainer( m_bundleDefinition.GetBundles() );

		Feedback::Initialize( bundleDataContainer.Size() );

		Uint32 numThreads = options.GetNumThreads();

		if( numThreads == 0 )
		{
			numThreads = Red::System::CpuId::GetInstance().GetNumberOfLogicalCores();
		}

		Initialize( options );

		TDynArray< TWorker* > workers;
		workers.Resize( numThreads );

		Red::Threads::CSemaphore semaphore( numThreads, numThreads );

		for( Uint32 i = 0; i < workers.Size(); ++i )
		{
			workers[ i ] = new TWorker( &semaphore, options );

			// Thread Spin up
			workers[ i ]->InitThread();
		}

		Uint32 bundleIndex = 0;
		for( TBundleDataContainers::const_iterator iBundle = bundleDataContainer.Begin(); iBundle != bundleDataContainer.End(); ++iBundle )
		{
			semaphore.Acquire();

			//Find free worker
			for( Uint32 iWorker = 0; iWorker < workers.Size(); ++iWorker )
			{
				if( workers[ iWorker ]->HasData() )
				{
					OnWorkFinished( workers[ iWorker ] );
					workers[ iWorker ]->ClearData();
				}

				if( workers[ iWorker ]->IsReady() )
				{
					TPayload payload;

					FillPayload( payload, iBundle->m_first, iBundle->m_second );

					RED_LOG( bug, TXT( "Activating worker %u" ), iWorker );

					workers[ iWorker ]->Activate( payload );
					break;
				}

				// If this assert triggers, then all worker threads were busy but the main thread (this one)
				// didn't lock (when it should have) when we acquired the semaphore
				RED_ASSERT( iWorker != workers.Size() - 1, TXT( "Couldn't find free thread" ) );
			}

			semaphore.Release();

			// Used for debugging, so it's easy to identify how far through processing the bundles we are
			++bundleIndex;
		}

		for( Uint32 i = 0; i < workers.Size(); ++i )
		{
			// Command Thread to exit
			workers[ i ]->QueueShutdown();
		}

		for( Uint32 i = 0; i < workers.Size(); ++i )
		{
			// Wait for thread to exit
			workers[ i ]->JoinThread();

			if( workers[ i ]->HasData() )
			{
				OnWorkFinished( workers[ i ] );
				workers[ i ]->ClearData();
			}

			// Cleanup
			delete workers[ i ];
			workers[ i ] = nullptr;
		}

		Shutdown( options );

		Feedback::Shutdown();
	}
}

#endif // __RED_BUNDLE_BUILDER_WORKER_MANAGER_H__
