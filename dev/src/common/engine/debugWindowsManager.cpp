/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../core/scriptStackFrame.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

// debug windows
#include "debugWindowsManager.h"
#include "debugWindowRenderingFlags.h"
#include "debugWindowSound.h"
#include "debugWindowLoadedResources.h"
#include "debugWindowGameFrame.h"
#include "debugWindowScriptThreads.h"
#include "debugWindowPerformanceLog.h"
#include "debugWindowReviewMarkers.h"
#include "debugWindowCallstackProfiler.h"
#include "debugWindowProfilerSettings.h"
#include "debugWindowTaskManager.h"
#include "debugWindowEnvironment.h"
#include "debugWindowAnimations.h"
#include "debugWindowVegetation.h"
#include "debugWindowTickManager.h"
#include "debugWindowPhysics.h"
#include "debugWindowMemoryMetrics.h"
#include "debugWindowUmbra.h"
#include "debugWindowArrayMemory.h"
#include "debugWindowFileLoadingStats.h"
#include "debugWindowFrameStats.h"
#include "debugWindowTerrain.h"
#include "debugWindowMemoryObjects.h"
#include "debugWindowResourceMonitor.h"
#include "debugWindowJobManager.h"
#include "debugWindowWorldStreaming.h"
#include "debugWindowInGameConfig.h"
#include "debugWindowPhysicsArea.h"

// red gui
#include "redGuiMenuBar.h"
#include "redGuiMenu.h"
#include "redGuiMenuItem.h"
#include "redGuiSlider.h"
#include "redGuiLabel.h"
#include "redGuiTextBox.h"
#include "redGuiDesktop.h"
#include "redGuiButton.h"
#include "redGuiCheckBox.h"
#include "redGuiComboBox.h"
#include "redGuiGraphicContext.h"
#include "debugPageManagerBase.h"
#include "viewport.h"
#include "inputBufferedInputEvent.h"
#include "game.h"


namespace DebugWindows
{
	CDebugWindowsManager::CDebugWindowsManager()
		: m_mainMenuBar(nullptr)
		, m_hidingLock( false )
	{
		GRedGui::GetInstance().EventViewportInput.Bind( this, &CDebugWindowsManager::NotifyViewportInput );
		GRedGui::GetInstance().EventExclusiveInputChanged.Bind( this, &CDebugWindowsManager::NotifyOnExclusiveInputChanged );

		m_windows.Resize( DW_Count );
		m_mainDesktop = new RedGui::CRedGuiDesktop();
		GRedGui::GetInstance().RegisterDesktop( m_mainDesktop );
	}

	CDebugWindowsManager::~CDebugWindowsManager()
	{
		GRedGui::GetInstance().EventViewportInput.Unbind( this, &CDebugWindowsManager::NotifyViewportInput );
		GRedGui::GetInstance().EventExclusiveInputChanged.Unbind( this, &CDebugWindowsManager::NotifyOnExclusiveInputChanged );
	}

	void CDebugWindowsManager::SetupMenu()
	{
		CreateMainMenuBar();

		GRedGui::GetInstance().EventTick.Bind(this, &CDebugWindowsManager::NotifyEventTick);
	}

	void CDebugWindowsManager::DestroyDebugWindows()
	{
		GRedGui::GetInstance().UnregisterDesktop( m_mainDesktop );
	}

	void CDebugWindowsManager::CreateMainMenuBar()
	{
		// add exit button
		RedGui::CRedGuiButton* exitButton = new RedGui::CRedGuiButton( 0, 0, 14, 18 );
		exitButton->SetMargin( Box2( 2, 2, 2, 2 ) );
		exitButton->SetBackgroundColor(Color::LIGHT_RED);
		exitButton->SetImage( RedGui::Resources::GExitIcon );
		exitButton->EventButtonClicked.Bind( this, &CDebugWindowsManager::NotifyOnCloseDebugWindows );
		m_mainDesktop->AddToMenuBar( exitButton );

		// add transparency slider
		m_alphaSlider = new RedGui::CRedGuiSlider( 0, 0, 100, 18 );
		m_alphaSlider->SetMargin( Box2( 0,0,10,0 ) );
		m_alphaSlider->SetBorderVisible( false );
		m_alphaSlider->SetBackgroundColor( Color::CLEAR );
		m_alphaSlider->SetMinValue( 10 );
		m_alphaSlider->SetMaxValue( 255 );
		m_alphaSlider->SetValue( 127 );
		m_alphaSlider->SetValue( (Float)GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->GetGlobalAlpha(), true );
		m_alphaSlider->EventScroll.Bind( this, &CDebugWindowsManager::NotifyEventScroll );
		m_mainDesktop->AddToMenuBar( m_alphaSlider );

		RedGui::CRedGuiButton* switchToOldDebugPage = new RedGui::CRedGuiButton(0, 0, 100, 18);
		switchToOldDebugPage->SetMargin(Box2(0, 1, 10, 1));
		switchToOldDebugPage->SetFitToText(true);
		switchToOldDebugPage->SetText(TXT("Debug pages"));
		switchToOldDebugPage->SetBorderVisible(false);
		switchToOldDebugPage->SetBackgroundColor(Color(100, 100, 100));
		switchToOldDebugPage->EventButtonClicked.Bind(this, &CDebugWindowsManager::NotifySwitchToOldDebugPage);
		m_mainDesktop->AddToMenuBar(switchToOldDebugPage);

		m_exclusiveInput = new RedGui::CRedGuiComboBox( 0, 0, 120, 18 );
		m_exclusiveInput->SetMargin( Box2(0, 1, 10, 1) );
		m_exclusiveInput->SetBorderVisible( false );
		m_exclusiveInput->SetBackgroundColor( Color::CLEAR );
		m_exclusiveInput->AddItem( TXT("RedGui ") );
		m_exclusiveInput->AddItem( TXT("Game ") );
		m_exclusiveInput->AddItem( TXT("RedGui & Game ") );
		m_exclusiveInput->SetSelectedIndex( RedGui::RGEI_Both );
		m_exclusiveInput->EventSelectedIndexChanged.Bind( this, &CDebugWindowsManager::NotifySwitchExclusiveInput );
		m_mainDesktop->AddToMenuBar( m_exclusiveInput );

		RedGui::CRedGuiLabel* inputLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 18 );
		inputLabel->SetText( TXT("Input:") );
		inputLabel->SetAutoSize( false );
		inputLabel->SetBorderVisible( false );
		inputLabel->SetBackgroundColor( Color::CLEAR );
		m_mainDesktop->AddToMenuBar( inputLabel );

		RedGui::CRedGuiCheckBox* softwareCursor = new RedGui::CRedGuiCheckBox( 0, 0, 18, 18 );
		softwareCursor->SetBorderVisible( false );
		softwareCursor->SetText( TXT("Software cursor") );
		softwareCursor->SetBackgroundColor( Color::CLEAR );
		softwareCursor->SetChecked( GRedGui::GetInstance().GetInputManager()->GetSoftwareCursorEnable() );
		softwareCursor->EventCheckedChanged.Bind( this, &CDebugWindowsManager::NotifyOnSoftwareCursoreChanged );
		m_mainDesktop->AddToMenuBar( softwareCursor );

		// create menubar
		m_mainMenuBar = new RedGui::CRedGuiMenuBar();
		m_mainDesktop->SetMenuBar( m_mainMenuBar );

		RedGui::CRedGuiMenuItem* item = nullptr;

		// create engine menu
		RedGui::CRedGuiMenu* engineMenu = m_mainMenuBar->AddNewMenu(TXT("Engine"));
		engineMenu->EventMenuItemSelected.Bind( this, &CDebugWindowsManager::NotifyMenuItemSelected );
		{
			item = engineMenu->AppendItem( TXT("Game filters")			, m_windows[DW_GameFilters] );
			item = engineMenu->AppendItem( TXT("Sound viewer")			, m_windows[DW_Sounds] );
			item = engineMenu->AppendItem( TXT("Loaded resources")		, m_windows[DW_LoadedResources] );
			item = engineMenu->AppendItem( TXT("Render resources")		, m_windows[DW_RenderResources] );
			item = engineMenu->AppendItem( TXT("Vegetation")			, m_windows[DW_Vegetation] );
			item = engineMenu->AppendItem( TXT("Tick manager")			, m_windows[DW_TickManager] );
			item = engineMenu->AppendItem( TXT("Physics")				, m_windows[DW_Physics] );
			item = engineMenu->AppendItem( TXT("Task manager")			, m_windows[DW_TaskManager] );
			item = engineMenu->AppendItem( TXT("Loading jobs")			, m_windows[DW_LoadingJobs] );

#ifdef USE_ARRAY_METRICS
			item = engineMenu->AppendItem( TXT( "Array Metrics" )		, m_windows[DW_ArrayMetrics] );
#endif
			item = engineMenu->AppendItem( TXT( "Terrain" )				, m_windows[DW_Terrain] );
			item = engineMenu->AppendItem( TXT( "World Streaming" )		, m_windows[DW_WorldStreaming] );
			item = engineMenu->AppendItem( TXT( "Object Memory" )		, m_windows[DW_ObjectsMemory] );
			item = engineMenu->AppendItem( TXT( "Resources" )			, m_windows[DW_Resources] );
			item = engineMenu->AppendItem( TXT("physics area")			, m_windows[DW_PhysicsArea] );


				//item = engineMenu->AppendSeparator();
			// here we can add new temporary windows <----------- Please don't. They're a huge pain to select properly.
// 			RedGui::CRedGuiMenu* temporaryWindowsMenu = new RedGui::CRedGuiMenu( 0, 0, 100, 0 );
// 			temporaryWindowsMenu->EventMenuItemSelected.Bind( this, &CDebugWindowsManager::NotifyMenuItemSelected );
// 			item = engineMenu->AppendSubMenuItem( TXT("Temporary windows"), temporaryWindowsMenu );
// 			{
// 			}

		}

		// create graphic menu
		RedGui::CRedGuiMenu* renderMenu = m_mainMenuBar->AddNewMenu(TXT("Graphic"));
		renderMenu->EventMenuItemSelected.Bind( this, &CDebugWindowsManager::NotifyMenuItemSelected );
		{
			item = renderMenu->AppendItem( TXT("Dynamic textures")		, m_windows[DW_DynamicTextures] );
			item = renderMenu->AppendItem( TXT("Umbra")					, m_windows[DW_Umbra] );
			item = renderMenu->AppendItem( TXT("Gpu resource use")		, m_windows[DW_GpuResourceUse] );
			item = renderMenu->AppendItem( TXT("Texture Streaming")		, m_windows[DW_TextureStreaming] );
		}

		// create gameplay menu
		RedGui::CRedGuiMenu* gameplayMenu = m_mainMenuBar->AddNewMenu(TXT("Gameplay"));
		gameplayMenu->EventMenuItemSelected.Bind( this, &CDebugWindowsManager::NotifyMenuItemSelected );
		{
			item = gameplayMenu->AppendItem( TXT("NPC viewer")			, m_windows[DW_NPCViewer] );
			item = gameplayMenu->AppendItem( TXT("Vehicle viewer")		, m_windows[DW_VehicleViewer] );
#ifndef FINAL
            item = gameplayMenu->AppendItem( TXT("Boat settings")       , m_windows[DW_BoatSettings] );
#endif
			item = gameplayMenu->AppendItem( TXT("Environment settings"), m_windows[DW_Environment] );
			item = gameplayMenu->AppendItem( TXT("InGame Configuration"), m_windows[DW_InGameConfig] );
		}

		// create performance menu
		RedGui::CRedGuiMenu* performanceMenu = m_mainMenuBar->AddNewMenu( TXT("Performance") );
		performanceMenu->EventMenuItemSelected.Bind( this, &CDebugWindowsManager::NotifyMenuItemSelected );
		{
			item = performanceMenu->AppendItem( TXT("Animations")			, m_windows[DW_Animations] );
			item = performanceMenu->AppendItem( TXT("Memory metrics")		, m_windows[DW_MemoryMetrics] );
			item = performanceMenu->AppendItem( TXT("File loading stats")	, m_windows[DW_FileLoadingStats] );
		}

		// create profiler menu
		RedGui::CRedGuiMenu* profilerMenu = m_mainMenuBar->AddNewMenu(TXT("Profilers"));
		profilerMenu->EventMenuItemSelected.Bind( this, &CDebugWindowsManager::NotifyMenuItemSelected );
		{
			item = profilerMenu->AppendItem( TXT("Counters")			, m_windows[DW_Counters] );
			item = profilerMenu->AppendItem( TXT("Settings")			, m_windows[DW_Settings] );
			item = profilerMenu->AppendItem( TXT("Performance log")		, m_windows[DW_PerformanceLog] );
			item = profilerMenu->AppendItem( TXT("Scripts threads")		, m_windows[DW_ScriptThreads] );
			item = profilerMenu->AppendItem( TXT("Game frame")			, m_windows[DW_GameFrame] );
			item = profilerMenu->AppendItem( TXT("Game world")			, m_windows[DW_GameWorld] );
		}

		// misc menu
		RedGui::CRedGuiMenu* miscMenu = m_mainMenuBar->AddNewMenu(TXT("Misc"));
		miscMenu->EventMenuItemSelected.Bind( this, &CDebugWindowsManager::NotifyMenuItemSelected );
		{
#ifndef NO_MARKER_SYSTEMS
			item = miscMenu->AppendItem( TXT("Review markers")			, m_windows[DW_ReviewMarkers] );
#endif // NO_MARKER_SYSTEMS
		}

		// create frame menu
		RedGui::CRedGuiMenu* frameStatsMenu = m_mainMenuBar->AddNewMenu(TXT("FrameStats"));
		frameStatsMenu->EventMenuItemSelected.Bind( this, &CDebugWindowsManager::NotifyMenuItemSelected );
		{
			item = frameStatsMenu->AppendItem( TXT("Scene Stats")			, m_windows[DW_SceneStats] );
		}
	}

	void CDebugWindowsManager::NotifyMenuItemSelected( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiMenuItem* menuItem )
	{
		RedGui::CRedGuiWindow* window = menuItem->GetUserData< RedGui::CRedGuiWindow >();
		if( window != nullptr )
		{
			window->SetVisible( true );
		}
	}

	void CDebugWindowsManager::NotifyEventScroll( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		RED_UNUSED( eventPackage );

		GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->SetGlobalAlpha( (Uint32)value );
	}

	void CDebugWindowsManager::NotifySwitchToOldDebugPage( RedGui::CRedGuiEventPackage& eventPackage )
	{
		SetVisible(false);

#ifndef NO_DEBUG_PAGES
		// select first debug page on the list of all
		IDebugPageManagerBase::GetInstance()->OnViewportInput( nullptr, IK_F4, IACT_Press, 0.0f);
#endif	// NO_DEBUG_PAGES
	}

	Bool CDebugWindowsManager::GetVisible() const
	{
		return GRedGui::GetInstance().GetEnabled() || GRedGui::GetInstance().IsBudgetModeOn();
	}

	void CDebugWindowsManager::SetVisible( Bool value, Bool keepOpenedWindows /*= false*/ )
	{
		if(value == true)
		{
#ifndef NO_DEBUG_PAGES
			IDebugPageManagerBase::GetInstance()->OnViewportInput( nullptr, IK_Escape, IACT_Press, 0.0f);
#endif	// NO_DEBUG_PAGES
		}

		if( keepOpenedWindows == false )
		{
			HideAllVisibleWindows();
		}

		GRedGui::GetInstance().SetEnabled(value);
	}

	Bool CDebugWindowsManager::IsDebugWindowVisible( EDebugWindow type )
	{
		return m_windows[type]->GetVisible();
	}

	void CDebugWindowsManager::ShowDebugWindow( EDebugWindow type )
	{
		m_windows[type]->SetVisible( true );
	}

	void CDebugWindowsManager::HideDebugWindow( EDebugWindow type )
	{
		m_windows[type]->SetVisible( false );
	}

	void CDebugWindowsManager::NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
#ifndef NO_DEBUG_PAGES
		if(IDebugPageManagerBase::GetInstance()->IsDebugPageActive() == true)
		{
			SetVisible(false);
		}
#endif	// NO_DEBUG_PAGES
	}

	void CDebugWindowsManager::RegisterWindow( RedGui::CRedGuiWindow* window, EDebugWindow windowId )
	{
		m_windows[ windowId ] = window;
		m_mainDesktop->AddWindow( window );
	}

	void CDebugWindowsManager::HideAllVisibleWindows()
	{
		if( m_hidingLock == 0 )
		{
			const Uint32 windowCount = m_windows.Size();
			for( Uint32 i=0; i<windowCount; ++i )
			{
				if( m_windows[i] && m_windows[i]->GetVisible() == true )
				{
					m_windows[i]->SetVisible( false );
				}
			}
		}
	}

	void CDebugWindowsManager::NotifyViewportInput( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		RED_UNUSED( eventPackage );

		if( action == IACT_Press )
		{
			if( key == IK_NumMinus )
			{
				Uint32 transparencyValue = GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->GetGlobalAlpha();
				transparencyValue = ( transparencyValue > 30 ) ? transparencyValue - 30 : 0;
				GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->SetGlobalAlpha( transparencyValue );
				m_alphaSlider->SetValue( (Float)GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->GetGlobalAlpha() );
			}
			else if( key == IK_NumPlus )
			{
				Uint32 transparencyValue = GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->GetGlobalAlpha() + 30;
				GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->SetGlobalAlpha( transparencyValue );
				m_alphaSlider->SetValue( (Float)GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->GetGlobalAlpha() );
			}
			else if( key == IK_Escape )
			{
				HideAllVisibleWindows();
			}
		}
	}

	void CDebugWindowsManager::NotifyOnCloseDebugWindows( RedGui::CRedGuiEventPackage& eventPackage )
	{
		SetVisible( false );
	}

	void CDebugWindowsManager::NotifySwitchExclusiveInput( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		Uint32 inputState = (Uint32)Clamp( value, 0, (Int32)RedGui::RGEI_Count );
		GRedGui::GetInstance().SetExclusiveInput( (RedGui::ERedGuiExclusiveInput) inputState );
	}

	void CDebugWindowsManager::NotifyOnExclusiveInputChanged( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value )
	{
		m_exclusiveInput->SetSelectedIndex( value );
	}

	RedGui::CRedGuiMenu* CDebugWindowsManager::GetMenu( const String& name )
	{
		if( m_mainMenuBar )
		{
			return m_mainMenuBar->GetMenu( name );
		}
		else 
		{
			return NULL;
		}		
	}

	void CDebugWindowsManager::NotifyOnSoftwareCursoreChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		GRedGui::GetInstance().GetInputManager()->SetSoftwareCursorEnable( value );
	}

	void CDebugWindowsManager::LockHiding()
	{
		++m_hidingLock;
	}

	void CDebugWindowsManager::UnlockHiding()
	{
		if( m_hidingLock > 0 )
		{
			--m_hidingLock;
		}
	}

}	// namespace DebugWindows

void InitializeAndRegisterDebugWindows()
{
	DebugWindows::CDebugWindowsManager& debugManager = GDebugWin::GetInstance();

	debugManager.RegisterWindow( new DebugWindows::CDebugWindowRenderingFlags()		, DebugWindows::DW_GameFilters );
#ifdef USE_UMBRA
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowUmbra()				, DebugWindows::DW_Umbra );
#endif
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowLoadedResources()	, DebugWindows::DW_LoadedResources );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowVegetation()			, DebugWindows::DW_Vegetation );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowTickManager()		, DebugWindows::DW_TickManager );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowPhysics()			, DebugWindows::DW_Physics );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowTaskManager()		, DebugWindows::DW_TaskManager );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowLoadingJobs()		, DebugWindows::DW_LoadingJobs );
	
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowEnvironment()		, DebugWindows::DW_Environment );
#ifdef SOUND_DEBUG
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowSound()				, DebugWindows::DW_Sounds );
#endif
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowGameFrame()			, DebugWindows::DW_GameFrame );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowScriptThreads()		, DebugWindows::DW_ScriptThreads );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowPerformanceLog()		, DebugWindows::DW_PerformanceLog );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowCallstackProfiler()	, DebugWindows::DW_Counters );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowProfilerSettings()	, DebugWindows::DW_Settings );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowInGameConfig()		, DebugWindows::DW_InGameConfig );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowAnimations()			, DebugWindows::DW_Animations );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowMemoryMetrics()		, DebugWindows::DW_MemoryMetrics );
#endif

	debugManager.RegisterWindow( new DebugWindows::CDebugWindowFileLoadingStats		, DebugWindows::DW_FileLoadingStats );

	debugManager.RegisterWindow( new DebugWindows::CDebugWindowMemoryObjects		, DebugWindows::DW_ObjectsMemory );

#ifdef ENABLE_RESOURCE_MONITORING
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowResourceMonitor		, DebugWindows::DW_Resources );
#endif	

	debugManager.RegisterWindow( new DebugWindows::CDebugWindowSceneStats			, DebugWindows::DW_SceneStats );

#ifndef NO_MARKER_SYSTEMS
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowReviewMarkers()		, DebugWindows::DW_ReviewMarkers );
#endif // NO_MARKER_SYSTEMS

#ifdef USE_ARRAY_METRICS
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowArrayMemoryMetrics()	, DebugWindows::DW_ArrayMetrics );
#endif

	debugManager.RegisterWindow( new DebugWindows::CDebugWindowTerrain()			, DebugWindows::DW_Terrain );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowWorldStreaming()		, DebugWindows::DW_WorldStreaming );
	debugManager.RegisterWindow( new CDebugWindowPhysicsArea()						, DebugWindows::DW_PhysicsArea );

	// HACK "remove" dependency from game to engine 
	extern void RegisterGameDebugWindows();
	RegisterGameDebugWindows();

	// register debug windows related only with renderer stuff
	extern void RegisterRendererDebugWindows();
	RegisterRendererDebugWindows();

	debugManager.SetupMenu();
}

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

void funcOpenDebugWindows( CObject*, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
	if( GGame != nullptr )
	{
		IViewport* viewport = GGame->GetViewport();
		if( viewport != nullptr )
		{
			GRedGui::GetInstance().OnViewportSetDimensions( viewport, viewport->GetWidth(), viewport->GetHeight() );
		}
	}
	GDebugWin::GetInstance().SetVisible( true );
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

	RETURN_VOID();
}

void ExportDebugWindowsNatives()
{
	NATIVE_GLOBAL_FUNCTION( "OpenDebugWindows", funcOpenDebugWindows );
}
