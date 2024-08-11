/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#endif	// NO_DEBUG_WINDOWS

#ifndef NO_DEBUG_PAGES

#include "debugCommandBox.h"
#include "debugCheckBox.h"
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "renderFrame.h"

#ifndef NO_DEBUG_WINDOWS
class CDebugWindowOpenCommand : public IDebugCommandBox
{
public:
	CDebugWindowOpenCommand( IDebugCheckBox* parent, const String& name, DebugWindows::EDebugWindow type )
		: IDebugCommandBox( parent, name )
		, m_type( type )
	{
		/* intentionally empty */
	}

	virtual void Process()
	{
		GDebugWin::GetInstance().SetVisible( true );
		GDebugWin::GetInstance().ShowDebugWindow( m_type );
	}

private:
	DebugWindows::EDebugWindow m_type;
};

#endif

class CDebugPageStartPage : public IDebugPage
{	
public:
	CDebugPageStartPage()
		: IDebugPage( TXT( "StartPage" ) )
		, m_availablePagesTree( nullptr )
		, m_removedPagesTree( nullptr )
	{
	}

	~CDebugPageStartPage()
	{
		if( m_availablePagesTree != nullptr )
		{
			delete m_availablePagesTree;
			m_availablePagesTree = nullptr;
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) 
	{
		if( m_availablePagesTree != nullptr )
		{
			if ( m_availablePagesTree->OnInput( key, action, data ) )
			{
				return true;
			}
		}
		return false;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) 
	{
		if( m_availablePagesTree == nullptr )
		{
			m_availablePagesTree = new CDebugOptionsTree( 50, 120, 300, 350, this, true );

#ifndef NO_DEBUG_WINDOWS
			// create debug window commands
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Rendering flags"), DebugWindows::DW_GameFilters ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Dynamic Textures"), DebugWindows::DW_DynamicTextures ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Loaded resources"), DebugWindows::DW_LoadedResources ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Render resources"), DebugWindows::DW_RenderResources ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("NPC viewer"), DebugWindows::DW_NPCViewer ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Vehicle viewer"), DebugWindows::DW_VehicleViewer ) );
#ifndef FINAL
            m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Boat settings"), DebugWindows::DW_BoatSettings ) );
#endif
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Environment"), DebugWindows::DW_Environment ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Sounds"), DebugWindows::DW_Sounds ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Game frame"), DebugWindows::DW_GameFrame ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Script threads"), DebugWindows::DW_ScriptThreads ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Performance log"), DebugWindows::DW_PerformanceLog ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Counters"), DebugWindows::DW_Counters ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Animations"), DebugWindows::DW_Animations ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Vegetation"), DebugWindows::DW_Vegetation ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Game world"), DebugWindows::DW_GameWorld ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Tick manager"), DebugWindows::DW_TickManager ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Physics"), DebugWindows::DW_Physics ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Memory metrics"), DebugWindows::DW_MemoryMetrics ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Task Manager"), DebugWindows::DW_TaskManager ) );
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Umbra"), DebugWindows::DW_Umbra ) );
#endif
#ifndef NO_MARKER_SYSTEMS								  
			m_availablePagesTree->AddRoot( new CDebugWindowOpenCommand( nullptr, TXT("Review markers"), DebugWindows::DW_ReviewMarkers ) );
#endif	// NO_MARKER_SYSTEMS
		}

		if( m_removedPagesTree == nullptr )
		{
			m_removedPagesTree = new CDebugOptionsTree( 400, 120, 300, 350, this, true );

			m_removedPagesTree->AddRoot( new IDebugCheckBox( nullptr, TXT("Dynamic Attributes"), false, false ) );
			m_removedPagesTree->AddRoot( new IDebugCheckBox( nullptr, TXT("Gameplay Config"), false, false ) );
			m_removedPagesTree->AddRoot( new IDebugCheckBox( nullptr, TXT("Menu"), false, false ) );
			m_removedPagesTree->AddRoot( new IDebugCheckBox( nullptr, TXT("Memory"), false, false ) );
			m_removedPagesTree->AddRoot( new IDebugCheckBox( nullptr, TXT("Bg"), false, false ) );
			m_removedPagesTree->AddRoot( new IDebugCheckBox( nullptr, TXT("BehTree"), false, false ) );
			m_removedPagesTree->AddRoot( new IDebugCheckBox( nullptr, TXT("Budget counters"), false, false ) );
			m_removedPagesTree->AddRoot( new IDebugCheckBox( nullptr, TXT("Character LODs budgets"), false, false ) );
		}


		String message = TXT("Debug pages below are converted to new framework.");
		frame->AddDebugRect( 50, 90, 300, 20, Color( 0, 0, 0, 128 ) );
		frame->AddDebugScreenFormatedText( 55, 105, Color::LIGHT_GREEN, message.AsChar() );
		m_availablePagesTree->OnRender( frame );

		String message2 = TXT("Debug pages below were removed from new framework.");
		frame->AddDebugRect( 400, 90, 300, 20, Color( 0, 0, 0, 128 ) );
		frame->AddDebugScreenFormatedText( 405, 105, Color::LIGHT_RED, message2.AsChar() );
		m_removedPagesTree->OnRender( frame );
	}

	virtual void OnTick( Float timeDelta )
	{
		if ( m_availablePagesTree != nullptr )
		{
			m_availablePagesTree->OnTick( timeDelta );
		}
	}

private:
	CDebugOptionsTree*		m_availablePagesTree;
	CDebugOptionsTree*		m_removedPagesTree;
};

void CreateDebugPageStartPage()
{
	IDebugPage* page = new CDebugPageStartPage();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}
#endif	// NO_DEBUG_PAGES
