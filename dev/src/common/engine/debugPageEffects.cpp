/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "fxState.h"
#include "fxTrackItem.h"

#ifndef NO_DEBUG_PAGES

#include "debugPageManagerBase.h"
#include "tickManager.h"
#include "inputBufferedInputEvent.h"
#include "renderFrame.h"
#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#include "game.h"
#include "world.h"
#include "fxDefinition.h"
#include "entity.h"
#endif

/// Interactive list of NPC
class CDebugPageEffects : public IDebugPage
{
private:
	CFXState*				m_selectedState;
	TDynArray< CFXState* >	m_lastFetchedStates;
	
	static const Uint32 LINE_HEIGHT;

	Int32	m_firstLineVisible;
	Int32	m_numLinesVisible;

public:
	CDebugPageEffects()
		: IDebugPage( TXT("Effects") )
		, m_selectedState( NULL )
		, m_firstLineVisible( 0 )
		, m_numLinesVisible( 0 )
	{
	}

	virtual ~CDebugPageEffects()
	{
	}

	//! This debug page was shown
	//virtual void OnPageShown()
	//{
	//	IDebugPage::OnPageShown();
	//}

	////! This debug page was hidden
	//virtual void OnPageHidden()
	//{
	//	IDebugPage::OnPageHidden();
	//}

	////! External viewport tick
	//virtual void OnTick( Float timeDelta )
	//{
	//	IDebugPage::OnTick( timeDelta );
	//}

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible( true );
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_LoadedResources );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		if ( ( key == IK_Up || key == IK_W || key == IK_Pad_DigitUp ) && action == IACT_Press )
		{
			Int32 currentIndex = static_cast< Int32 >( m_lastFetchedStates.GetIndex( m_selectedState ) );
			if ( currentIndex == -1 )
			{
				// None selected
				if ( !m_lastFetchedStates.Empty() )
				{
					// Select first
					m_selectedState = m_lastFetchedStates[0];
				}
				else
				{
					// Selectin nothing
					m_selectedState = NULL;
				}
			}
			else if ( --currentIndex >= 0 )
			{
				// Select previous
				m_selectedState = m_lastFetchedStates[ currentIndex ];
			}

			// Processed
			return true;
		}
		if ( ( key == IK_Down || key == IK_S || key == IK_Pad_DigitDown ) && action == IACT_Press )
		{
			Int32 currentIndex = static_cast< Int32 >( m_lastFetchedStates.GetIndex( m_selectedState ) );
			if ( currentIndex == -1 )
			{
				// None selected
				if ( !m_lastFetchedStates.Empty() )
				{
					// Select first
					m_selectedState = m_lastFetchedStates[0];
				}
				else
				{
					// Selectin nothing
					m_selectedState = NULL;
				}
			}
			else if ( ++currentIndex < (Int32)m_lastFetchedStates.Size() )
			{
				// Select next
				m_selectedState = m_lastFetchedStates[ currentIndex ];
			}

			// Processed
			return true;
		}

		// Not handled
		return false;
	};

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

		IDebugPage::OnViewportGenerateFragments( view, frame );

		// Collect currently ticked effects
		if ( !GGame->GetActiveWorld() )
		{
			return;
		}
		CTickManager* tickMgr = GGame->GetActiveWorld()->GetTickManager();

		struct GrabEffectsVisitor
		{
			GrabEffectsVisitor( TDynArray< CFXState* >& effects ) : m_effects( effects ) { m_effects.ClearFast(); }
			RED_FORCE_INLINE void Process( CFXState* state ) { m_effects.PushBack( state ); }
			TDynArray< CFXState* >& m_effects;
		} grabEffectsVisitor( m_lastFetchedStates );

		tickMgr->GetEffects().ProcessAll( grabEffectsVisitor );

		// ================= DRAW EFFECTS LIST PANEL ======================
		// Draw effects list background
		const Uint32 width = frame->GetFrameOverlayInfo().m_width;
		const Uint32 height = frame->GetFrameOverlayInfo().m_height;
		frame->AddDebugRect( 50, 50, 150, height-120, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( 50, 50, 150, height-120, Color::WHITE );

		// Caption
		Uint32 y = 65;
		Uint32 x = 70;
		String caption = String::Printf( TXT("Active effects: %i"), m_lastFetchedStates.Size() );
		frame->AddDebugScreenText( x, y, caption, Color::LIGHT_GREEN );

		// Compute scrolling stuff
		m_numLinesVisible = ( height-140 ) / LINE_HEIGHT;
		{
			Int32 indexSelected = static_cast< Int32 >( m_lastFetchedStates.GetIndex( m_selectedState ) );
			if ( indexSelected >= 0 )
			{
				// Proper effect is selected, so make sure it is contained in the view of the list
				if ( indexSelected < m_firstLineVisible )
				{
					// Have to scroll up
					m_firstLineVisible -= ( m_firstLineVisible - indexSelected );
				}
				else if ( indexSelected > m_firstLineVisible + m_numLinesVisible - 1 )
				{
					// Have to scroll down
					m_firstLineVisible += ( indexSelected - ( m_firstLineVisible + m_numLinesVisible - 1 ) );
				}
			}
		}

		// Draw scrolling bar
		Float barHeightPercentage = 1.0f;
		const Int32 maxBarHeight = height-120;
		Int32 barHeight = (Int32)( barHeightPercentage * (Float) maxBarHeight );
		Float barPositionPercentage = 0.0f;
		Int32 barPosition = 52;
		if ( !m_lastFetchedStates.Empty() )
		{
			barHeightPercentage = Min( (Float)m_numLinesVisible / (Float)m_lastFetchedStates.Size(), 1.0f );
			barHeight = (Int32)( ( (Float)( maxBarHeight ) ) * barHeightPercentage );
			barPositionPercentage = (Float)m_firstLineVisible / (Float)m_lastFetchedStates.Size();
			barPosition += (Int32) ( barPositionPercentage * (Float)maxBarHeight );
		}
		
		frame->AddDebugRect( 52, barPosition, 8, barHeight, Color::LIGHT_BLUE );
		frame->AddDebugFrame( 52, barPosition, 8, barHeight, Color::BLUE );

		// List of effects
		y += 20;
		Int32 numShownEffects = Min( m_numLinesVisible, Max( (Int32)m_lastFetchedStates.Size() - m_firstLineVisible, 0 ) );
		for ( Int32 i=m_firstLineVisible; i<m_firstLineVisible + numShownEffects; ++i )
		{
			const String effectName = m_lastFetchedStates[i]->GetDefinition()->GetName().AsString();
			Color textColor = Color::WHITE;

			if ( !ValidateEffectOptmialization( m_lastFetchedStates[i], NULL ) )
			{
				// Indicate that this effect is not optimally made
				textColor = Color::YELLOW;
			}

			if ( m_lastFetchedStates[i] == m_selectedState )
			{
				textColor = Color::MAGENTA;
			}

			frame->AddDebugScreenText( x, y, effectName, textColor );
			y += LINE_HEIGHT;
		}

		// ================== DRAW DEBUG BOUNDING BOX OF THE SELECTED EFFECT =================
		// Show BB of the selected effect
		{
			Box bbox;
			if ( m_selectedState && m_lastFetchedStates.Exist( m_selectedState ) )
			{
				bbox = m_selectedState->GetDebugBoundingBox();
			}
			frame->AddDebugBox( bbox, Matrix::IDENTITY, Color::LIGHT_RED );
		}

		// ================== WRITE DESCRIPTION OF SELECTED EFFECT ==========================
		if ( m_selectedState && m_lastFetchedStates.Exist( m_selectedState ) )
		{
			// Draw selected effect description background
			frame->AddDebugRect( 210, 50, width - 310, height-120, Color( 0, 0, 0, 128 ) );
			frame->AddDebugFrame( 210, 50, width - 310, height-120, Color::WHITE );

			y = 65;
			x = 225;

			// Print path of the parent entity
			String entityPath = m_selectedState->GetEntity()->GetFriendlyName();
			frame->AddDebugScreenText( x, y, entityPath, Color::WHITE );
			y += 15;

			// Print the general effect state info
			String currentTimeString = String::Printf( TXT("Current time: %.2f"), m_selectedState->GetCurrentTime() );
			frame->AddDebugScreenText( x, y, currentTimeString, Color::YELLOW );
			if ( m_selectedState->IsStopping() )
			{
				frame->AddDebugScreenText( x + 150, y, TXT("STOPPING"), Color::RED );
			}
			if ( m_selectedState->IsPaused() )
			{
				frame->AddDebugScreenText( x + 200, y, TXT("PAUSED"), Color::MAGENTA );
			}
			TDynArray< String > optimizationComments;
			if ( !ValidateEffectOptmialization( m_selectedState, &optimizationComments ) )
			{
				for ( Uint32 i=0; i<optimizationComments.Size(); ++i )
				{
					y += 15;
					frame->AddDebugScreenText( x, y, optimizationComments[i], Color::YELLOW );
				}
			}
			
			y += 15;
			
			// List all active track items play data (the implemented ones :))
			TDynArray< String > description;
			TDynArray< IFXTrackItemPlayData* > playDatas;
			m_selectedState->GetPlayData( playDatas );
			for ( Uint32 i=0; i<playDatas.Size(); ++i )
			{
				description.Clear();
				playDatas[i]->GetDescription( description );
				if ( !description.Empty() )
				{
					frame->AddDebugScreenText( x, y, description[0], Color::GREEN );
					y += 15;
				}
				for ( Uint32 j=1; j<description.Size(); ++j )
				{
					frame->AddDebugScreenText( x, y, description[j], Color::WHITE );
					y += 15;
				}
			}
			
		}
	}

	Bool ValidateEffectOptmialization( CFXState* fxState, TDynArray< String >* comments )
	{
		Bool result = true;
		TDynArray< IFXTrackItemPlayData* > playDatas;
		fxState->GetPlayData( playDatas );

		for ( Uint32 i=0; i<playDatas.Size(); ++i )
		{
			if ( !playDatas[i]->ValidateOptimization( comments ) )
			{
				// Not optimally made
				result = false;
				if ( !comments )
				{
					// No need to test further
					break;
				}
			}
		}

		return result;
	}
};

const Uint32 CDebugPageEffects::LINE_HEIGHT = 15;

void CreateDebugPageEffects()
{
	IDebugPage* page = new CDebugPageEffects();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
