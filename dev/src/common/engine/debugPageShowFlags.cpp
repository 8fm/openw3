/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "debugWindowsManager.h"
#include "debugCheckBox.h"
#include "debugPageManagerBase.h"
#include "viewport.h"
#include "game.h"
#include "renderFrame.h"
#include "inputKeys.h"
#include "inputBufferedInputEvent.h"
#include "../core/scriptStackFrame.h"
#include "../core/scriptable.h"

/// Special check box option for toggling show flag on and off
class CDebugCheckBoxShowFlag : public IDebugCheckBox
{
protected:
	EShowFlags		m_flag;

public:
	CDebugCheckBoxShowFlag( IDebugCheckBox* parent, EShowFlags flag )
		: IDebugCheckBox( parent, CEnum::ToString< EShowFlags >( flag ), false, true )
		, m_flag( flag )
	{};

	//! Is this item checked ?
	virtual Bool IsChecked() const
	{
		IViewport* gameViewport = GGame->GetViewport();
		return gameViewport->GetRenderingMask()[ m_flag ];
	}

	//! Toggle state of this item, only for check items
	virtual void OnToggle()
	{
		IViewport* gameViewport = GGame->GetViewport();
		if ( gameViewport->GetRenderingMask()[ m_flag ] )
		{
			gameViewport->ClearRenderingMask( m_flag );
		}
		else
		{
			gameViewport->SetRenderingMask( m_flag );
		}
	}
};

extern EShowFlags GShowGameFilter[];
extern EShowFlags GShowRenderFilter[];
extern EShowFlags GShowPostProcessFilter[];

/// Debug page that can toggle show flags on/off
class CDebugPageShowFlags : public IDebugPage
{
protected:
	CDebugOptionsTree*	m_tree;

public:
	CDebugPageShowFlags()
		: IDebugPage( TXT("Show flags") )
	{
		// Create debug tree
		m_tree = new CDebugOptionsTree( 55, 65, 330, 500, this );

		// Collect options
		TDynArray< EShowFlags > showFlags;
		for ( Uint32 i=0; GShowGameFilter[i] != SHOW_MAX_INDEX; i++ )
		{
			EShowFlags flag = GShowGameFilter[i];
			showFlags.PushBack( flag );
		}

		// Don't let them screw with the debug shit :)
		showFlags.Remove( SHOW_VisualDebug );
		showFlags.Remove( SHOW_Profilers );

		// Create rendering group
		IDebugCheckBox* rendering = new IDebugCheckBox( NULL, TXT("Rendering"), true, false );
		m_tree->AddRoot( rendering );

		// Fill with rendering related options
		for ( Uint32 i=0; GShowRenderFilter[i] != SHOW_MAX_INDEX; i++ )
		{
			EShowFlags flag = GShowRenderFilter[i];
			new CDebugCheckBoxShowFlag( rendering, flag );
			showFlags.Remove( flag );
		}

		// Create rendering group
		IDebugCheckBox* postProcess = new IDebugCheckBox( NULL, TXT("Postprocess"), true, false );
		m_tree->AddRoot( postProcess );

		// Fill with rendering related options
		for ( Uint32 i=0; GShowPostProcessFilter[i] != SHOW_MAX_INDEX; i++ )
		{
			EShowFlags flag = GShowPostProcessFilter[i];
			new CDebugCheckBoxShowFlag( postProcess, flag );
			showFlags.Remove( flag );
		}

		// Extra shit
		IDebugCheckBox* debug = new IDebugCheckBox( NULL, TXT("Debug"), true, false );
		m_tree->AddRoot( debug );

		// Fill with rest of the options
		for ( Uint32 i=0; i<showFlags.Size(); i++ )
		{
			new CDebugCheckBoxShowFlag( debug, showFlags[i] );
		}

		// Select the rendering group
		m_tree->SelectItem( rendering );
	};

	~CDebugPageShowFlags()
	{
		delete m_tree;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		m_tree->OnRender( frame );		
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_GameFilters );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// Send the event
		if ( m_tree->OnInput( key, action, data ) )
		{
			return true;
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		m_tree->OnTick( timeDelta );
	}

};

void CreateDebugPageShowFlags()
{
	IDebugPage* page = new CDebugPageShowFlags();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif

#ifndef RED_FINAL_BUILD
static void funcDebugSetEShowFlag( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Uint32, sflag	,1);
	GET_PARAMETER(bool, activate, false);
	FINISH_PARAMETERS;

	if(activate)
	{
		GGame->GetViewport()->SetRenderingMask((EShowFlags)sflag);
	}
	else
	{
		IViewport* gameViewport = GGame->GetViewport();
		if ( gameViewport->GetRenderingMask()[ sflag ] )
		{
			gameViewport->ClearRenderingMask( (EShowFlags)sflag );
		}
	}
}

void ExportShowFlagsNatives()
{	
	NATIVE_GLOBAL_FUNCTION( "DebugSetEShowFlag",				funcDebugSetEShowFlag );
}
#endif

