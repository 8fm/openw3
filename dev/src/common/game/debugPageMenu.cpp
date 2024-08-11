/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_WINDOWS
#include "../engine/debugWindowsManager.h"
#endif
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/renderFrame.h"

#ifndef NO_DEBUG_PAGES

RED_DEFINE_STATIC_NAME( DebugMenuEntriesGet );
RED_DEFINE_STATIC_NAME( DebugMenuEntrySelect );

/// Interactive list of NPC
class CDebugPageMenu : public IDebugPage
{
protected:
	TDynArray< String > m_entries;

	Uint32 m_selectedEntry;

public:
	CDebugPageMenu()
		: IDebugPage( TXT("Menu") )
		, m_selectedEntry( 0 )
	{}

	//! This debug page was shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();

		m_selectedEntry = 0;

		m_entries.Clear();
		CallFunctionRef( NULL, CNAME( DebugMenuEntriesGet ), m_entries );

		// Add script profiling entries
		m_entries.PushBack( TXT("") ); // Add blank entry as separator
		
		// Pause game
		GGame->Pause( TXT( "CDebugPageMenu" ) );
	}

	//! This debug page was hidden
	virtual void OnPageHidden()
	{
		IDebugPage::OnPageHidden();

		// Unpause game
		GGame->Unpause( TXT( "CDebugPageMenu" ) );
	}

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible(true);
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		if ( ( key == IK_Up || key == IK_W || key == IK_Pad_DigitUp ) && action == IACT_Press )
		{
			if ( m_selectedEntry > 0 )	--m_selectedEntry;
			else						m_selectedEntry = m_entries.Size()-1;

			// If selected entry is empty, just skip it. (empty entries are used as separators)
			if( m_entries[m_selectedEntry].Empty() ) OnViewportInput( view, key, action, data );

			return true;
		}
		if ( ( key == IK_Down || key == IK_S || key == IK_Pad_DigitDown ) && action == IACT_Press )
		{
			if ( (Int32)m_selectedEntry < m_entries.SizeInt()-1 )	++m_selectedEntry;
			else										m_selectedEntry = 0;

			// If selected entry is empty, just skip it. (empty entries are used as separators)
			if( m_entries[m_selectedEntry].Empty() ) OnViewportInput( view, key, action, data );

			return true;
		}

		if ( ( key == IK_LeftMouse || key == IK_Space || key == IK_Enter || key == IK_Pad_A_CROSS ) && action == IACT_Press )
		{
			IDebugPageManagerBase::GetInstance()->SelectDebugPage( NULL );
			
			CallFunction( NULL, CNAME( DebugMenuEntrySelect ), (Int32)m_selectedEntry );
			return true;
		}

		// Not handled
		return false;
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is destined to remove. Click key 'Enter' to open new debug framework.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 70, 120, Color(255, 0, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		// Draw info background
		const Uint32 width = frame->GetFrameOverlayInfo().m_width;
		const Uint32 height = frame->GetFrameOverlayInfo().m_height;
		frame->AddDebugRect( 50, 50, width-100, height-120, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( 50, 50, width-100, height-120, Color::WHITE );

		for ( Uint32 i = 0; i < m_entries.Size(); ++i )
		{
			Color color = m_selectedEntry == i ? Color::YELLOW : Color::WHITE;
			frame->AddDebugScreenText( 55, 65 + 20*i, m_entries[i].AsChar(), color );
		}
	}
};

void CreateDebugPageMenu()
{
	IDebugPage* page = new CDebugPageMenu();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
