/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "debugWindowsManager.h"
#include "../core/scriptStackFrame.h"
#include "../core/scriptableState.h"
#include "../core/scriptingSystem.h"
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "inputBufferedInputEvent.h"
#include "inputKeys.h"
#include "renderFrame.h"
#include "node.h"

/// Debug page with script threads
class CDebugPageScriptThreads : public IDebugPage
{
protected:
	enum ESortType
	{
		SortById,
		SortByOwner,
		SortByFunction,
		SortByTicks,
		SortByTime,
	};

	struct ThreadInfo
	{
		Uint32		m_id;
		Bool		m_isLost;
		Bool		m_tempFlag;
		Uint32		m_lastTotalTicks;
		Float		m_lastTotalTime;
		Float		m_highlightTime;
		Color		m_highlightColor;
		String		m_function;
		String		m_owner;

		ThreadInfo( CScriptThread* thread )
			: m_id( thread->GetID() )
			, m_isLost( false )
			, m_tempFlag( false )
			, m_highlightTime( 1.0f )
			, m_highlightColor( Color::GREEN )
			, m_lastTotalTicks( thread->GetTotalTicks() )
			, m_lastTotalTime( thread->GetTotalTime() )
			, m_function( TXT("Unknown") )
			, m_owner( TXT("World") )
		{
			// Get owner
			IScriptable* owner = thread->GetContext().Get();
			if ( owner )
			{
				if ( owner->IsA< CScriptableState >() )
				{
					// Get name of the state
					CScriptableState* theState = Cast< CScriptableState >( owner );
					m_owner = theState->GetStateName().AsString();

					// Get the state machine
					ISerializable* sm = theState->GetStateMachine();
					if ( sm && sm->IsA< CNode >() )
					{
						CNode* node = Cast< CNode >( sm );
						m_owner += TXT(" in ");
						m_owner += node->GetName();
					}
				}
				else
				{
					// Use simple name
					m_owner = owner->GetFriendlyName().AsChar();
				}
			}

			// Get top level function
			if ( thread->GetFrames().Size() )
			{
				CScriptStackFrame* frame = thread->GetFrames()[0];
				m_function = frame->m_function->GetName().AsString();
			}
		}
		
		void Update( CScriptThread* thread )
		{
			// Update time
			m_lastTotalTicks = thread->GetTotalTicks();
			m_lastTotalTime = thread->GetTotalTime();

			// Get top level function
			if ( thread->GetFrames().Size() )
			{
				CScriptStackFrame* frame = thread->GetFrames()[0];
				m_function = frame->m_function->GetName().AsString();
			}
			else
			{
				m_function = TXT("Unknown");
			}
		}

		static ESortType s_sortType;

		template < typename T >
		RED_INLINE static Int32 GetCompareValue( const T& a, const T& b )
		{
			if ( a < b ) return 1;
			if ( a > b ) return -1;
			return 0;
		}

		static int ThreadInfoCompare( const void *arg1, const void *arg2 )
		{
			ThreadInfo* a = *( ThreadInfo** ) arg1;
			ThreadInfo* b = *( ThreadInfo** ) arg2;

			switch ( s_sortType )
			{
				case SortById: return GetCompareValue( a->m_id, b->m_id );
				case SortByOwner: return -GetCompareValue( a->m_owner, b->m_owner );
				case SortByFunction: return -GetCompareValue( a->m_function, b->m_function );
				case SortByTicks: return GetCompareValue( a->m_lastTotalTicks, b->m_lastTotalTicks );
				case SortByTime: return GetCompareValue( a->m_lastTotalTime, b->m_lastTotalTime );
			}

			return 0;
		}
	};

protected:
	THashMap< Uint32, ThreadInfo* >		m_threads;
	ESortType						m_sortType;
	Int32								m_listOffset;

public:
	CDebugPageScriptThreads()
		: IDebugPage( TXT("Script Threads") )
		, m_sortType( SortById )
		, m_listOffset( 0 )
	{};

	//! Update list
	virtual void OnTick( Float timeDelta )
	{
		// Mark all threads as lost for now :)
		TDynArray< Uint32 > threadIdsToRemove;
		for ( THashMap< Uint32, ThreadInfo* >::iterator it=m_threads.Begin(); it!=m_threads.End(); ++it )
		{
			// Reset
			ThreadInfo* info = it->m_second;
			info->m_tempFlag = true;

			// Dump highlight
			info->m_highlightTime -= timeDelta;
			if ( info->m_highlightTime < 0.0f )
			{
				// Clamp fade out
				info->m_highlightTime = 0.0f;
				info->m_highlightColor = Color::WHITE;

				// Killed, remove from list
				if ( info->m_isLost )
				{
					threadIdsToRemove.PushBack( info->m_id );
				}
			}
		}

		// Remove threads
		for ( TDynArray< Uint32 >::const_iterator it=threadIdsToRemove.Begin(); it!=threadIdsToRemove.End(); ++it )
		{
			THashMap< Uint32, ThreadInfo* >::iterator toRemove = m_threads.Find( *it );
			if ( toRemove != m_threads.End() )
			{
				delete toRemove->m_second;
				m_threads.Erase( toRemove );
			}
		}

		// Get all threads, spawn new thread infos, update existing
		const CScriptingSystem::TScriptThreadArray& threads = GScriptingSystem->GetThreads();
		for ( CScriptingSystem::TScriptThreadArray::const_iterator it=threads.Begin(); it!=threads.End(); ++it )
		{
			CScriptThread* thread = *it;
			if ( thread && !thread->IsKilled() )
			{
				// Update current info
				ThreadInfo* info = NULL;
				m_threads.Find( thread->GetID(), info );

				// No ID created, create one
				if ( !info )
				{
					info = new ThreadInfo( thread );
					m_threads.Insert( thread->GetID(), info );
				}
				
				// Update
				info->Update( thread );

				// Mark as alive
				info->m_tempFlag = false;
			}
		}

		// Start fading killed threads
		for ( THashMap< Uint32, ThreadInfo* >::iterator it=m_threads.Begin(); it!=m_threads.End(); ++it )
		{
			ThreadInfo* info = it->m_second;

			// Start fade out
			if ( info->m_tempFlag && !info->m_isLost )
			{
				info->m_isLost = true;
				info->m_highlightColor = Color::GRAY;
				info->m_highlightTime = 1.0f;
			}
		}
	}
	
	//! Handle key
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_ScriptThreads );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// Change sort mode
		if ( key == IK_S && action == IACT_Press )
		{
			m_sortType = ( ESortType )( (m_sortType + 1) % 5 );
			return true;
		}

		return false;
	}

	//! Sort threads
	void SortThreads( TDynArray< ThreadInfo* >& allThreads )
	{
		// Get all threads
		allThreads.Reserve( m_threads.Size() );
		for ( THashMap< Uint32, ThreadInfo* >::iterator it=m_threads.Begin(); it!=m_threads.End(); ++it )
		{
			ThreadInfo* info = it->m_second;
			allThreads.PushBack( info );
		}

		// Sort the crap
		ThreadInfo::s_sortType = m_sortType;
		qsort( allThreads.TypedData(), allThreads.Size(), sizeof( ThreadInfo* ), &ThreadInfo::ThreadInfoCompare );
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		// Sort for displaying
		TDynArray< ThreadInfo* > allThreads;
		SortThreads( allThreads );

		// Header
		Uint32 y = 65;
		frame->AddDebugScreenText( 65, y, TXT("Threads"), Color::YELLOW );
		frame->AddDebugScreenText( 125, y, String::Printf( TXT("%i threads"), allThreads.Size() ), Color::WHITE );

		// Sorting
		if ( m_sortType == SortById ) frame->AddDebugScreenText( 215, y, TXT("Sort by ID"), Color::WHITE );
		if ( m_sortType == SortByFunction ) frame->AddDebugScreenText( 215, y, TXT("Sort by Function"), Color::WHITE );
		if ( m_sortType == SortByOwner ) frame->AddDebugScreenText( 215, y, TXT("Sort by Owner"), Color::WHITE );
		if ( m_sortType == SortByTicks ) frame->AddDebugScreenText( 215, y, TXT("Sort by Ticks"), Color::WHITE );
		if ( m_sortType == SortByTime ) frame->AddDebugScreenText( 215, y, TXT("Sort by Time"), Color::WHITE );

		// Move down
		y += 20;

		// Dump the list
		Uint32 startIndex = Max< Int32 >( 0, m_listOffset );
		const Uint32 height = frame->GetFrameOverlayInfo().m_height - 30;
		for ( Uint32 i=startIndex; i<allThreads.Size() && y < height; ++i )
		{
			// Determine color
			ThreadInfo* info = allThreads[i];
			const Color& color = info->m_highlightColor;

			// Draw thread info
			frame->AddDebugScreenText( 55, y, String::Printf( TXT("%i"), info->m_id ), color );
			frame->AddDebugScreenText( 95, y, info->m_owner, color );
			frame->AddDebugScreenText( 345, y, info->m_function, color );
			frame->AddDebugScreenText( 495, y, String::Printf( TXT("%i"), info->m_lastTotalTicks ), color );
			frame->AddDebugScreenText( 530, y, String::Printf( TXT("%1.3fms"), info->m_lastTotalTime * 1000.0f ), color );

			// Move down
			y += 15;
		}
	}
};

CDebugPageScriptThreads::ESortType CDebugPageScriptThreads::ThreadInfo::s_sortType = CDebugPageScriptThreads::SortById;

void CreateDebugPageScriptThreads()
{
	IDebugPage* page = new CDebugPageScriptThreads();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif