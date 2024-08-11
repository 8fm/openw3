/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderLodBudgetSystem.h"

#ifndef NO_DEBUG_WINDOWS
#include "../engine/debugWindowsManager.h"
#endif
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/inputBufferedInputEvent.h"
#include "../engine/renderFrame.h"

#ifndef NO_DEBUG_PAGES

/// Debug page with GUI texture stats
class CDebugPageCharacterLODs : public IDebugPage
{
public:
	CDebugPageCharacterLODs( CRenderLodBudgetSystem* lodBudgetSystem )
		: IDebugPage( TXT("Character LODs budget system") )
		, m_lodBudgetSystem( lodBudgetSystem )
		, m_selectedLodManagementPolicy( (Uint32)lodBudgetSystem->GetLodManagementPolicy()  )
	{}

	//! Page shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();
	}

	//! Tick the crap
	virtual void OnTick( Float timeDelta )
	{
		IDebugPage::OnTick( timeDelta );
	}

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

		// Send the event
		if ( action == IACT_Press )
		{
			switch ( key )
			{
			case IK_Home:
			case IK_Pad_X_SQUARE:
				m_lodBudgetSystem->ToggleSwitch();
				break;

			case IK_Pad_DigitDown:
			case IK_Down:
				++m_selectedLodManagementPolicy;
				if ( m_selectedLodManagementPolicy > LDP_MAX - 1 )
				{
					m_selectedLodManagementPolicy = LDP_None;
				}
				break;

			case IK_Pad_DigitUp:
			case IK_Up:
				--m_selectedLodManagementPolicy;
				if ( m_selectedLodManagementPolicy < LDP_None )
				{
					m_selectedLodManagementPolicy = LDP_MAX - 1;
				}
				break;

			case IK_Pad_A_CROSS:
			case IK_Enter:
				if ( m_selectedLodManagementPolicy != m_lodBudgetSystem->GetLodManagementPolicy() && m_selectedLodManagementPolicy != LDP_MAX )
				{
					m_lodBudgetSystem->SetLodManagementPolicy( (ELodManagementPolicy)m_selectedLodManagementPolicy );
				}
				break;
			}
		}

		// Not processed
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

		const Uint32 screenWidth = frame->GetFrameOverlayInfo().m_width;
		const Uint32 screenHeight = frame->GetFrameOverlayInfo().m_height;

		Uint32 ySpacing = 15;
		Uint32 listXSpacing = 75;

		Uint32 pageTopLeftX = 50;
		Uint32 pageTopLeftY = 50;
		Uint32 pageWidth = screenWidth - 2 * pageTopLeftX;
		Uint32 pageHeight = screenHeight - pageTopLeftY - 80;

		Uint32 commandsX = pageTopLeftX + 20;
		Uint32 commandsY = pageTopLeftY;

		Uint32 headerX = pageTopLeftX + 400;
		Uint32 headerY = pageTopLeftY + ySpacing;

		frame->AddDebugRect( pageTopLeftX, pageTopLeftY, pageWidth, pageHeight, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( pageTopLeftX, pageTopLeftY, pageWidth, pageHeight, Color::WHITE );

		frame->AddDebugScreenFormatedText( commandsX, commandsY + 1 * ySpacing, Color::WHITE, TXT( "Press Home button to toggle system ON/OFF" ) );

		frame->AddDebugScreenFormatedText( commandsX, commandsY + 2 * ySpacing, m_lodBudgetSystem->IsOn() ? Color::GREEN : Color::RED, TXT( "Bugeting system %s" ), m_lodBudgetSystem->IsOn() ? TXT( "ON" ) : TXT( "OFF" ) );
		frame->AddDebugScreenFormatedText( commandsX, commandsY + 3 * ySpacing, m_lodBudgetSystem->IsInitialized() ? Color::GREEN : Color::RED, TXT( "Bugeting initialized: [%s]" ), m_lodBudgetSystem->IsInitialized() ? TXT( "TRUE" ) : TXT( "FALSE" ) );
		
		for ( Int32 i = 0; i < LDP_MAX; ++i )
		{
			Color c;
			
			if ( i == m_selectedLodManagementPolicy )
			{
				c = Color::YELLOW;
			}
			else if ( i == m_lodBudgetSystem->GetLodManagementPolicy() )
			{
				c = Color::GREEN;
			}			
			else
			{
				c = Color::WHITE;
			}
			
			String lmpName;
			#define LMP_NAME( x ) case x: lmpName = TXT( #x ); break;
			switch ( i )
			{
				LMP_NAME( LDP_None );
				LMP_NAME( LDP_AgressiveDrop );
				LMP_NAME( LDP_GentleDrop );
				LMP_NAME( LDP_AdaptiveDrop );
				LMP_NAME( LDP_AgressiveEnhance );
				LMP_NAME( LDP_MAX );
			}
			#undef LMP_NAME

			frame->AddDebugScreenFormatedText( commandsX, commandsY + ( 5 + i ) * ySpacing, c, TXT( "%s" ), lmpName.AsChar() );
		}

		frame->AddDebugScreenFormatedText( headerX + 1 * listXSpacing, headerY + 0 * ySpacing, Color::YELLOW, TXT( "Previous frame" ) );
		frame->AddDebugScreenFormatedText( headerX + 2 * listXSpacing, headerY + 0 * ySpacing, Color::YELLOW, TXT( "Current frame" ) );
		
		frame->AddDebugScreenFormatedText( headerX + 0 * listXSpacing, headerY + 1 * ySpacing, Color::YELLOW, TXT( "Proxies count" ) );
		frame->AddDebugScreenFormatedText( headerX + 0 * listXSpacing, headerY + 2 * ySpacing, Color::YELLOW, TXT( "Chunks count" ) );
		frame->AddDebugScreenFormatedText( headerX + 0 * listXSpacing, headerY + 3 * ySpacing, Color::YELLOW, TXT( "Triangles count" ) );

		if ( m_lodBudgetSystem->IsOn() )
		{
			// write stats
			frame->AddDebugScreenFormatedText( headerX + 1 * listXSpacing, headerY + 1 * ySpacing, Color::WHITE, TXT( "%d" ), m_lodBudgetSystem->GetCurrentCharacterProxiesCount() );
			frame->AddDebugScreenFormatedText( headerX + 1 * listXSpacing, headerY + 2 * ySpacing, Color::WHITE, TXT( "%d" ), m_lodBudgetSystem->GetCurrentCharacterChunksCount() );
			frame->AddDebugScreenFormatedText( headerX + 1 * listXSpacing, headerY + 3 * ySpacing, Color::WHITE, TXT( "%d" ), m_lodBudgetSystem->GetCurrentCharacterTrianglesCount() );
			frame->AddDebugScreenFormatedText( headerX + 2 * listXSpacing, headerY + 1 * ySpacing, Color::WHITE, TXT( "%d" ), m_lodBudgetSystem->GetPreviousCharacterProxiesCount() );
			frame->AddDebugScreenFormatedText( headerX + 2 * listXSpacing, headerY + 2 * ySpacing, Color::WHITE, TXT( "%d" ), m_lodBudgetSystem->GetPreviousCharacterChunksCount() );
			frame->AddDebugScreenFormatedText( headerX + 2 * listXSpacing, headerY + 3 * ySpacing, Color::WHITE, TXT( "%d" ), m_lodBudgetSystem->GetPreviousCharacterTrianglesCount() );
		}
		else
		{
			frame->AddDebugScreenFormatedText( (Int32)( headerX + 1.5f * listXSpacing ), (Int32)( headerY + 1.5f * ySpacing ), Color::WHITE, TXT( "Turn on the system to see the stats." ) );
		}
	}

private:
	CRenderLodBudgetSystem*	m_lodBudgetSystem;
	Int32						m_selectedLodManagementPolicy;
};

void CreateDebugPageCharacterLods()
{
	/*IDebugPage* page = new CDebugPageCharacterLODs( GetRenderer()->GetLodBudgetSystem() );
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );*/
}

#endif