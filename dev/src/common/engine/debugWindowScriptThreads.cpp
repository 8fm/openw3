/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../core/scriptThread.h"
#include "redGuiLabel.h"
#include "redGuiManager.h"
#include "redGuiPanel.h"
#include "redGuiList.h"
#include "redGuiComboBox.h"
#include "debugWindowScriptThreads.h"
#include "../core/scriptStackFrame.h"
#include "../core/scriptableState.h"
#include "../core/scriptingSystem.h"
#include "node.h"

namespace DebugWindows
{	
	// FIX THIS!!!
	CDebugWindowScriptThreads::EScriptThreadSortType CDebugWindowScriptThreads::SScriptThreadsSorter::s_sortType = CDebugWindowScriptThreads::STST_Id;

	CDebugWindowScriptThreads::SThreadInfo::SThreadInfo( const CScriptThread& thread )
		: m_id( thread.GetID() )
		, m_isLost( false )
		, m_isDead( false )
		, m_highlightTime( 1.0f )
		, m_highlightColor( Color::WHITE )
		, m_lastTotalTicks( thread.GetTotalTicks() )
		, m_lastTotalTime( thread.GetTotalTime() )
		, m_function( TXT("Unknown") )
		, m_owner( TXT("World") )
	{
		// Get owner
		IScriptable* owner = thread.GetContext().Get();
		if ( owner != nullptr )
		{
			if ( owner->IsA< CScriptableState >() == true )
			{
				// Get name of the state
				CScriptableState* theState = Cast< CScriptableState >( owner );
				m_owner = theState->GetStateName().AsString();

				// Get the state machine
				ISerializable* sm = theState->GetStateMachine();
				if ( sm != nullptr && sm->IsA< CNode >() == true)
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
		if ( thread.GetFrames().Size() )
		{
			CScriptStackFrame* frame = thread.GetFrames()[0];
			m_function = frame->m_function->GetName().AsString();
		}
	}

	void CDebugWindowScriptThreads::SThreadInfo::Update( const CScriptThread& thread )
	{
		// Update time
		m_lastTotalTicks = thread.GetTotalTicks();
		m_lastTotalTime = thread.GetTotalTime();

		// Get top level function
		if ( thread.GetFrames().Size() )
		{
			CScriptStackFrame* frame = thread.GetFrames()[0];
			m_function = frame->m_function->GetName().AsString();
		}
		else
		{
			m_function = TXT("Unknown");
		}
	}

	CDebugWindowScriptThreads::CDebugWindowScriptThreads() 
		: RedGui::CRedGuiWindow(200,200,750,400)
		, m_sortType(nullptr)
		, m_threadsList(nullptr)
	{
		SetCaption(TXT("ScriptThreads"));

		// connect to events
		GRedGui::GetInstance().EventTick.Bind(this, &CDebugWindowScriptThreads::NotifyEventTick);

		RedGui::CRedGuiPanel* sortPanel = new RedGui::CRedGuiPanel(0, 0, 700, 20);
		sortPanel->SetMargin(Box2(10, 10, 5, 5));
		sortPanel->SetBorderVisible(false);
		sortPanel->SetBackgroundColor(Color::CLEAR);
		AddChild(sortPanel);
		sortPanel->SetDock(RedGui::DOCK_Top);

		RedGui::CRedGuiLabel* sortLabel = new RedGui::CRedGuiLabel(0,0, 50, 20);
		sortLabel->SetText(TXT("Sort by: "));
		sortPanel->AddChild(sortLabel);
		sortLabel->SetDock(RedGui::DOCK_Left);

		m_sortType = new RedGui::CRedGuiComboBox(0, 0, 150, 20);
		m_sortType->EventSelectedIndexChanged.Bind(this, &CDebugWindowScriptThreads::NotifyEventSelectedIndexChanged);
		sortPanel->AddChild(m_sortType);
		m_sortType->SetDock(RedGui::DOCK_Left);
		m_sortType->AddItem(TXT("ID"));
		m_sortType->AddItem(TXT("Function"));
		m_sortType->AddItem(TXT("Owner"));
		m_sortType->AddItem(TXT("Ticks"));
		m_sortType->AddItem(TXT("Time"));
		m_sortType->SetSelectedIndex(0);

		m_threadsList = new RedGui::CRedGuiList(0, 0, 700, 300);
		m_threadsList->SetMargin(Box2(5, 5, 5, 5));
		m_threadsList->SetDock(RedGui::DOCK_Fill);
		m_threadsList->AppendColumn( TXT("ID"), 100 );
		m_threadsList->AppendColumn( TXT("OWNER"), 300 );
		m_threadsList->AppendColumn( TXT("FUNCTIONS"), 200 );
		m_threadsList->AppendColumn( TXT("LAST TICK"), 100 );
		m_threadsList->AppendColumn( TXT("LAST TIME"), 100 );
		AddChild(m_threadsList);
	}

	CDebugWindowScriptThreads::~CDebugWindowScriptThreads()
	{
	}

	void CDebugWindowScriptThreads::OnPendingDestruction()
	{
		m_sortType->EventSelectedIndexChanged.Unbind(this, &CDebugWindowScriptThreads::NotifyEventSelectedIndexChanged);
	}

	void CDebugWindowScriptThreads::NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime)
	{
		if( GetVisible() == true )
		{
			// Mark all threads as lost for now
			TDynArray< SThreadInfo* > threadIdsToRemove;
			for(Uint32 i=0; i< m_threads.Size(); ++i)
			{
				SThreadInfo* info = m_threads[i];
				info->m_isDead = true;

				// Dump highlight
				info->m_highlightTime -= deltaTime;
				if ( info->m_highlightTime < 0.0f )
				{
					// Clamp fade out
					info->m_highlightTime = 0.0f;
					info->m_highlightColor = Color(255,255,255,255-(Uint8)(255*info->m_highlightTime));	// 'linear' fade out

					// Killed, remove from list
					if ( info->m_isLost == true )
					{
						threadIdsToRemove.PushBack( info );
					}
				}
			}

			// Remove threads
			for(Uint32 i=0; i< threadIdsToRemove.Size(); ++i)
			{
				m_threads.Remove( threadIdsToRemove[i] );
				delete threadIdsToRemove[i];
			}

			// Get all threads, spawn new thread infos, update existing
			const CScriptingSystem::TScriptThreadArray& threads = GScriptingSystem->GetThreads();
			for ( CScriptingSystem::TScriptThreadArray::const_iterator it=threads.Begin(); it!=threads.End(); ++it )
			{
				CScriptThread* thread = *it;
				if ( thread != nullptr && thread->IsKilled() == false )
				{
					// Update current info
					SThreadInfo* info = nullptr;
					for(Uint32 i=0; i< m_threads.Size(); ++i)
					{
						if(thread->GetID() == m_threads[i]->m_id)
						{
							info = m_threads[i];
							break;
						}
					}

					// No ID created, create one
					if ( info == nullptr )
					{
						info = new SThreadInfo( *thread );
						m_threads.Insert( info );
					}

					// Update
					info->Update( *thread );

					// Mark as alive
					info->m_isDead = false;
				}
			}

			// Start fading killed threads
			for(Uint32 i=0; i< m_threads.Size(); ++i)
			{
				SThreadInfo* info = m_threads[i];

				// Start fade out
				if ( info->m_isDead == true && info->m_isLost == false )
				{
					info->m_isLost = true;
					info->m_highlightColor = Color::GRAY;
					info->m_highlightTime = 1.0f;
				}
			}

			// update gui
			UpdateGuiInformation();
		}
	}

	void CDebugWindowScriptThreads::UpdateGuiInformation()
	{
		m_threadsList->RemoveAllItems();

		for(Uint32 i=0; i< m_threads.Size(); ++i)
		{
			SThreadInfo* info = m_threads[i];
			Color color = info->m_highlightColor;

			m_threadsList->AddItem( TXT("") );

			m_threadsList->SetItemColor( i, color );
			m_threadsList->SetItemText( ToString( info->m_id ), i, 0 );
			m_threadsList->SetItemText( info->m_owner, i, 1 );
			m_threadsList->SetItemText( info->m_function, i, 2 );
			m_threadsList->SetItemText( ToString( info->m_lastTotalTicks ), i, 3 );
			m_threadsList->SetItemText( String::Printf( TXT("%1.3f ms"), info->m_lastTotalTime * 1000.0f ), i, 4 );
		}
	}

	void CDebugWindowScriptThreads::NotifyEventSelectedIndexChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
	{
		RED_UNUSED( eventPackage );

		switch ( selectedIndex )
		{
		case STST_Id: 
			SScriptThreadsSorter::s_sortType = STST_Id;
			break;
		case STST_Owner: 
			SScriptThreadsSorter::s_sortType = STST_Owner;
			break;
		case STST_Function: 
			SScriptThreadsSorter::s_sortType = STST_Function;
			break;
		case STST_Ticks: 
			SScriptThreadsSorter::s_sortType = STST_Ticks;
			break;
		case STST_Time: 
			SScriptThreadsSorter::s_sortType = STST_Time;
			break;
		}

		m_threads.Sort();
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
