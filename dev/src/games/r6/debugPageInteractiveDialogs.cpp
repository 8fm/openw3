/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "idSystem.h"
#include "idInstance.h"
#include "idTopic.h"
#include "idThread.h"
#include "idInterlocutor.h"
#include "r6DialogDisplayManager.h"
#include "idHud.h"
#include "idGraphBlockChoice.h"
#include "../../common/engine/debugPage.h"
#include "../../common/engine/debugPageManagerBase.h"
#include "../../common/engine/renderFrame.h"

#ifndef NO_DEBUG_PAGES

/// Interactive dialogs debug
class CDebugPageInteractiveDialogs : public IDebugPage
{
protected:
	TQueue< const CIDGraphBlock* >		m_lastFewBlocks;
	TDynArray< Debug_SIDThreadInfo >	m_entries;
	Uint32								m_selectedEntry;
	Bool								m_paused;


public:
	CDebugPageInteractiveDialogs()
		: IDebugPage( TXT("Interactive dialogs") )
		, m_selectedEntry( 0 )
		, m_paused( false )
	{}

	//! This debug page was shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();

		m_selectedEntry = 0;

		m_entries.Clear();
	}

	//! This debug page was hidden
	virtual void OnPageHidden()
	{
		IDebugPage::OnPageHidden();

		if ( m_paused )
		{
			// Unpause game
			GGame->Unpause( TXT( "CDebugPageInteractiveDialogs" ) );
			m_paused = false;
		}
	}

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		if ( ( key == IK_Up || key == IK_W /*|| key == IK_Pad_DigitUp*/ ) && action == IACT_Press )
		{
			if ( m_selectedEntry > 0 )
			{
				--m_selectedEntry;
			}
			else
			{
				m_selectedEntry = m_entries.Empty() ? 0 : m_entries.Size() - 1;
			}

			return false; 
		}
		if ( ( key == IK_Down || key == IK_S /*|| key == IK_Pad_DigitDown*/ ) && action == IACT_Press )
		{
			if ( m_selectedEntry < m_entries.Size() - 1 && !m_entries.Empty() )
			{
				++m_selectedEntry;
			}
			else
			{
				m_selectedEntry = 0;
			}

			return true;
		}

		if ( ( key == IK_Space || key == IK_Enter || key == IK_P ) && action == IACT_Press )
		{
			if ( m_paused )
			{
				GGame->Unpause( TXT( "CDebugPageInteractiveDialogs" ) );
				m_paused = false;

			}
			else
			{
				GGame->Pause( TXT( "CDebugPageInteractiveDialogs" ) );
				m_paused = true;
			}

			if ( m_selectedEntry < m_entries.Size() )
			{
			}

			return true;
		}

		// Not handled
		return false;
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		// Draw info background
		const Uint32 width = frame->GetFrameInfo().GetWidth();
		const Uint32 height = frame->GetFrameInfo().GetHeight();
		frame->AddDebugRect( 50, 50, width-100, height-120, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( 50, 50, width-100, height-120, Color::WHITE );

		// Collect threads
		m_entries.ClearFast();
		GCommonGame->GetSystem< CInteractiveDialogSystem > ()->Debug_GatherThreadInfos( m_entries );

		// Display headers
		frame->AddDebugScreenText( 55, 65, TXT("Dialog:"), Color::GREEN );
		frame->AddDebugScreenText( 155, 65, TXT("Topic:"), Color::GREEN );
		frame->AddDebugScreenText( 255, 65, TXT("Thread:"), Color::GREEN );

		// Iterate over threads to display them in the list
		for ( Uint32 i = 0; i < m_entries.Size(); ++i )
		{
			Color color = m_selectedEntry == i ? Color::YELLOW : Color::WHITE;
			Debug_SIDThreadInfo& nfo = m_entries[ i ];
			frame->AddDebugScreenText( 55, 80 + 15 * i,		nfo.m_instance->GetResourceHandle().GetPath().StringAfter( TXT("\\"), true ).StringBefore( TXT("."), true ), color );
			frame->AddDebugScreenText( 155, 80 + 15 * i, 	nfo.m_topic->GetTopic()->GetName().AsString(), color );
			frame->AddDebugScreenText( 255, 80 + 15 * i,	nfo.m_thread->GetInput()->GetName().AsString(), color );
		}

		// If any thread is selected
		if ( m_selectedEntry < m_entries.Size() )
		{
			Uint32 x = 400;
			Uint32 y = 65;

			// Pick the info
			Debug_SIDThreadInfo& nfo = m_entries[ m_selectedEntry ];
			const CIDThreadInstance* thread = nfo.m_thread;

			// Display current state 
			frame->AddDebugScreenText( x, y, TXT("Current state:"), Color::GREEN );
			y += 15;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Dialog: %s, Topic: %s, Thread: %s"), 
				CEnum::ToString( nfo.m_instance->GetPlayState() ).AsChar(), CEnum::ToString( nfo.m_topic->GetPlayState() ).AsChar(), CEnum::ToString( thread->GetPlayState() ).AsChar() ), Color::WHITE );
			
			// Display interlocutors map
			y += 20;
			frame->AddDebugScreenText( x, y, TXT("Interlocutors:"), Color::GREEN );
			for ( THashMap< CName, CIDInterlocutorComponent* >::const_iterator it = nfo.m_instance->m_knownInterlocutors.Begin(); it != nfo.m_instance->m_knownInterlocutors.End(); ++it )
			{
				y += 15;
				frame->AddDebugScreenText( x, y, it->m_first.AsString() + TXT(":"), Color::LIGHT_BLUE );
				String talkingToString;
				CIDInterlocutorComponent*	talkingTo	= it->m_second->GetToWhomAmITalking();
				if( talkingTo )
				{
					talkingToString	= talkingTo->GetLocalizedName();
				}
				frame->AddDebugScreenText( x + 100, y, String::Printf( TXT("%s (voicetag: %s, tag: %s, talking to: %s)"), 
					it->m_second->GetFriendlyName().AsChar(),
					it->m_second->GetVoiceTag().AsString().AsChar(),
					it->m_second->GetTags().ToString().AsChar(),
					talkingTo ),
					Color::WHITE );
			}

			// Display current block
			y += 20;
			frame->AddDebugScreenText( x, y, TXT("Current block:"), Color::GREEN );
			y += 15;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("%s of class %s"), 
				thread->m_currentBlock ? thread->m_currentBlock->GetName().AsString().AsChar() : TXT("NULL"),
				thread->m_currentBlock ? thread->m_currentBlock->GetClass()->GetName().AsString().AsChar() : TXT("void") ),
				Color::WHITE );

			// Display last few blocks
			y += 20;
			const Int32 numLastBlocks = 5;
			frame->AddDebugScreenText( x, y, TXT("Last few blocks:"), Color::GREEN );
			for ( Int32 i = thread->m_lastBlocks.SizeInt() - 2; i >= 0 && i >= thread->m_lastBlocks.SizeInt() - numLastBlocks - 2; --i )
			{
				y += 15;
				const CIDGraphBlock* block = thread->m_lastBlocks[ i ];
				frame->AddDebugScreenText( x, y, String::Printf( TXT("%s of class %s"), block->GetName().AsString().AsChar(),	block->GetClass()->GetName().AsString().AsChar() ),	Color::WHITE ); 
			}

			// Display current choices
			y += 20;
			frame->AddDebugScreenText( x, y, TXT("Current choices:"), Color::GREEN );
			if ( thread->m_choiceOptions[ CHOICE_Left ].IsVisibleNow() )
			{
				y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Left:\"%s\"")
					, thread->m_choiceOptions[ CHOICE_Left ].m_option->m_text.GetString().AsChar() )
					, Color::WHITE );
			}
			if ( thread->m_choiceOptions[ CHOICE_Right ].IsVisibleNow() )
			{
				y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Right: \"%s\"")
					, thread->m_choiceOptions[ CHOICE_Right ].m_option->m_text.GetString().AsChar() )
					, Color::WHITE );
			}
			if ( thread->m_choiceOptions[ CHOICE_Up ].IsVisibleNow() )
			{
				y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Up: \"%s\"") 
					, thread->m_choiceOptions[ CHOICE_Up ].m_option->m_text.GetString().AsChar() )
					, Color::WHITE );
			}
			if ( thread->m_choiceOptions[ CHOICE_Down ].IsVisibleNow() )
			{
				y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Down: \"%s\"")
					, thread->m_choiceOptions[ CHOICE_Down ].m_option->m_text.GetString().AsChar() )
					, Color::WHITE );
			}

			// Display hud lines
			TDynArray< CR6DialogDisplay::SPlayingLine > &lines = GCommonGame->GetSystem< CR6DialogDisplay > ()->m_playingLines;
			if ( !lines.Empty() )
			{				
				y += 20;
				frame->AddDebugScreenText( x, y, TXT("Current hud lines:"), Color::GREEN );

				for ( Uint32 i = 0; i < lines.Size(); ++i )
				{
					y += 15;
					CIDInterlocutorComponent* interlocutor = lines[ i ].m_speaker;
					frame->AddDebugScreenText( x, y, interlocutor->GetVoiceTag().AsString() + TXT(":"), Color::LIGHT_BLUE );
					frame->AddDebugScreenText( x + 100, y, lines[ i ].m_line->m_text.GetString(), 
						( interlocutor && interlocutor->GetLineStatus( *lines[ i ].m_line, nfo.m_instance->GetInstanceID() ) == DILS_Playing ) ? Color::WHITE : Color::GRAY );
				}
				
			}
		}
	}
};

IDebugPage* CreateDebugPageInteractiveDialogs()
{
	IDebugPage* page = new CDebugPageInteractiveDialogs();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
	return page;
}

#endif
