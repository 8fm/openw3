/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __RED_BUNDLE_BUILDER_CONSUMER_H__
#define __RED_BUNDLE_BUILDER_CONSUMER_H__

#include "../../common/redSystem/cpuid.h"

#include "../../common/core/core.h"
#include "../../common/core/bundledefinition.h"

#include "feedback.h"
#include "options.h"

namespace Bundler
{
	//////////////////////////////////////////////////////////////////////////
	// CConsumer
	//////////////////////////////////////////////////////////////////////////
	template< typename TPayload >
	class CConsumer : public Red::Threads::CThread
	{
	public:
		CConsumer( const AnsiChar* threadName, Red::Threads::CSemaphore* lock );
		virtual ~CConsumer();

		void Activate( TPayload& payload );
		void ActivateSingleThreaded( TPayload& payload );

		RED_INLINE Bool IsReady() const { return m_ready.GetValue(); }
		RED_INLINE void QueueShutdown() { m_shutdown.SetValue( true ); }

		RED_INLINE Bool HasData() const { return m_hasData.GetValue(); }
		RED_INLINE void ClearData() { m_hasData.SetValue( false ); }

	protected:
		virtual void Do() = 0;
		virtual void ThreadFunc() override final;

	private:
		Red::Threads::CSemaphore* m_semaphore;
		Red::Threads::CAtomic< Bool > m_ready;
		Red::Threads::CAtomic< Bool > m_hasData;
		Red::Threads::CAtomic< Bool > m_shutdown;

	protected:
		TPayload m_payload;
		Feedback m_feedback;
	};

	template< typename TPayload >
	CConsumer< TPayload >::CConsumer( const AnsiChar* threadName, Red::Threads::CSemaphore* lock )
		:	Red::Threads::CThread( threadName )
		,	m_semaphore( lock )
		,	m_ready( true )
		,	m_hasData( false )
		,	m_shutdown( false )
	{

	}

	template< typename TPayload >
	CConsumer< TPayload >::~CConsumer()
	{

	}

	template< typename TPayload >
	void CConsumer< TPayload >::ThreadFunc()  
	{
		while( !m_shutdown.GetValue() || !m_ready.GetValue() )
		{
			if( !m_ready.GetValue() )
			{
				Do();

				m_hasData.SetValue( true );

				m_feedback.SetProgress( 1.0f );
				m_ready.SetValue( true );
				m_semaphore->Release();
			}

			Red::Threads::SleepOnCurrentThread( 1 );
		}
	}

	template< typename TPayload >
	void CConsumer< TPayload >::Activate( TPayload& payload )
	{
		// Pre conditions
		RED_FATAL_ASSERT( m_semaphore != nullptr, "Semaphore required for multithreaded processing" );
		RED_FATAL_ASSERT( m_ready.GetValue(), "Cannot use this thread, it's still busy" );
		RED_FATAL_ASSERT( !m_shutdown.GetValue(), "Cannot use this thread, Thread has been scheduled to shutdown" );

		// Store the data this worker will process
		m_payload = payload;

		RED_LOG( bug, TXT( "Ready state before: %hs" ), ( m_ready.GetValue() == true )? "true" : "false" );

		// Switch thread to "in-use"
		RED_VERIFY( m_ready.Exchange( false ) == true, TXT( "There was a problem trying to return this thread to it's ready state" ) );

		RED_LOG( bug, TXT( "Ready state after: %hs" ), ( m_ready.GetValue() == true )? "true" : "false" );

		// 
		m_semaphore->Acquire();
	}

	template< typename TPayload >
	void CConsumer< TPayload >::ActivateSingleThreaded( TPayload& payload )
	{
		m_payload = payload;
		Do();
	}
}

#endif // __RED_BUNDLE_BUILDER_CONSUMER_H__
