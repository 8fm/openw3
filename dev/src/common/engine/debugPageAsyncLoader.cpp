/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#if 0

/// Page for showing async loader tokens
class CDebugPageAsyncLoader : public IDebugPage, public IAsyncLoaderListener
{
protected:
	struct AsyncToken
	{
		Uint32					m_id;				//!< ID of token
		String					m_name;				//!< Loaded file
		EAsyncLoadingStatus		m_status;			//!< Internal status
		Float					m_time;				//!< Loading start time
		Uint32					m_bytesLoaded;		//!< Bytes loaded for this file
		Uint32					m_filesLoaded;		//!< Number of loaded files
		Float					m_keepAlive;		//!< Keep alive timer for fading away

		RED_INLINE AsyncToken( Uint32 id, const String& name )
			: m_id( id )
			, m_status( ALS_Pending )
			, m_keepAlive( 4.0f )
			, m_filesLoaded( 0 )
			, m_bytesLoaded( 0 )
		{
			CFilePath filePath( name );
			m_name = filePath.GetFileName();
		};
	};

protected:
	TDynArray< AsyncToken* >	m_tokens;		//!< Internal token list
	Red::Threads::CMutex		m_tokenMutex;	//!< List access mutex

public:
	CDebugPageAsyncLoader()
		: IDebugPage( TXT("Async Loader") )
	{
		// Install as listener of the async loader
		GAsyncLoader->SetListener( this );
	}

	//! External viewport tick
	virtual void OnTick( Float timeDelta )
	{
		Red::Threads::CMutex::CScopedLock lock( &m_tokenMutex );

		// Update despawn timers :)
		TDynArray< AsyncToken* > tokensToRemove;
		for ( TDynArray< AsyncToken* >::iterator it=m_tokens.Begin(); it!=m_tokens.End(); ++it )
		{
			AsyncToken* token = *it;
			if ( token->m_status == ALS_Finished || token->m_status == ALS_Canceled || token->m_status == ALS_Finished )
			{
				token->m_keepAlive -= timeDelta;
				if ( token->m_keepAlive < 0.0f )
				{
					token->m_keepAlive = 0.0f;
					tokensToRemove.PushBack( token );
				}
			}
		}

		// Remove expired tokens
		for ( TDynArray< AsyncToken* >::iterator it=tokensToRemove.Begin(); it!=tokensToRemove.End(); ++it )
		{
			AsyncToken* token = *it;
			m_tokens.Remove( token );
			delete token;
		}		
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_tokenMutex );

		// Header
		Uint32 y = 65;
		frame->AddDebugScreenText( 50, y, TXT("Loader stats"), Color::YELLOW ); y += 15;
		frame->AddDebugScreenText( 50, y, String::Printf( TXT("Total data loaded: %1.2f MB"), ( Float ) ( ( Double ) CDependencyLoader::GetNumBytesLoadedSoFar() / ( 1024.0 * 1024.0 ) ) ), Color::WHITE ); y += 15;
		frame->AddDebugScreenText( 50, y, String::Printf( TXT("Total files loaded: %i"), CDependencyLoader::GetNumFilesLoadedSoFar() ), Color::WHITE ); y += 15;
		y += 10;

		// Async list header
		frame->AddDebugScreenText( 50, y, TXT("Async Loading Queue"), Color::YELLOW );
		y += 15;

		// Show list
		for ( TDynArray< AsyncToken* >::iterator it=m_tokens.Begin(); it!=m_tokens.End(); ++it )
		{
			AsyncToken* token = *it;

			// Do not draw past the screen end
			if ( y > frame->GetFrameInfo().m_height )
			{
				continue;
			}

			// Draw the line
			if ( token->m_status == ALS_Pending )
			{
				frame->AddDebugScreenText( 50, y, token->m_name, Color::WHITE );
			}
			else if ( token->m_status == ALS_Loading )
			{
				const Float loadingTimeSoFar = (Float)Red::System::Clock::GetInstance().GetTimer().GetSeconds() - token->m_time;
				frame->AddDebugScreenText( 50, y, token->m_name, Color::YELLOW );				
				frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.1fs"), loadingTimeSoFar ), Color::YELLOW );				
			}
			else if ( token->m_status == ALS_Finished )
			{
				frame->AddDebugScreenText( 50, y, token->m_name, Color::GREEN );				

				if ( token->m_time < 0.5f )
				{
					frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.1fms"), token->m_time ), Color::GREEN );				
				}
				else
				{
					frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.1fs"), token->m_time ), Color::GREEN );				
				}

				if ( token->m_bytesLoaded < 500000 )
				{
					frame->AddDebugScreenText( 300, y, String::Printf( TXT("%1.2fKB"), token->m_bytesLoaded / ( 1024.0f ) ), Color::GREEN );
				}
				else
				{
					frame->AddDebugScreenText( 300, y, String::Printf( TXT("%1.2fMB"), token->m_bytesLoaded / ( 1024.0f * 1024.0f ) ), Color::GREEN );
				}

				frame->AddDebugScreenText( 350, y, String::Printf( TXT("%i files"), token->m_filesLoaded ), Color::GREEN );				
			}
			else if ( token->m_status == ALS_Canceled )
			{
				frame->AddDebugScreenText( 50, y, token->m_name, Color::GRAY );
			}
			else if ( token->m_status == ALS_Error )
			{
				frame->AddDebugScreenText( 50, y, token->m_name, Color::RED );
			}

			// Next line
			y += 15;
		}
	}

protected:
	//! Loading task was scheduled
	virtual void OnAsyncTaskScheduled( Uint32 taskID, const String& depotPath )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_tokenMutex );

		// Create new info
		AsyncToken* token = new AsyncToken( taskID, depotPath );
		m_tokens.PushBack( token );
	}

	//! Loading task started loading
	virtual void OnAsyncTaskStartedLoading( Uint32 taskID )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_tokenMutex );

		// Find by id
		for ( TDynArray< AsyncToken* >::iterator it=m_tokens.Begin(); it!=m_tokens.End(); ++it )
		{
			AsyncToken* token = *it;
			if ( token->m_id == taskID )
			{
				token->m_status = ALS_Loading;
				token->m_time = (Float) Red::System::Clock::GetInstance().GetTimer().GetSeconds();
				break;
			}
		}
	}

	//! Loading task ended loading
	virtual void OnAsyncTaskEndedLoading( Uint32 taskID, Float totalLoadingTime, Uint32 bytesRead, Uint32 filesRead, Bool loadingError )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_tokenMutex );

		// Find by id
		for ( TDynArray< AsyncToken* >::iterator it=m_tokens.Begin(); it!=m_tokens.End(); ++it )
		{
			AsyncToken* token = *it;
			if ( token->m_id == taskID )
			{
				// Update status
				token->m_status = loadingError ? ALS_Error : ALS_Finished;
				token->m_time = totalLoadingTime;
				token->m_bytesLoaded = bytesRead;				
				token->m_filesLoaded = filesRead;
				token->m_keepAlive = 8.0f;
				break;
			}
		}
	}

	//! Loading task was canceled
	virtual void OnAsyncTaskCanceled( Uint32 taskID )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_tokenMutex );

		// Find by id
		for ( TDynArray< AsyncToken* >::iterator it=m_tokens.Begin(); it!=m_tokens.End(); ++it )
		{
			AsyncToken* token = *it;
			if ( token->m_id == taskID )
			{
				// Update status
				token->m_status = ALS_Canceled;
				token->m_keepAlive = 8.0f;
				break;
			}
		}
	}
};

void CreateDebugPageAsyncLoader()
{
	IDebugPage* page = new CDebugPageAsyncLoader();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif

#endif