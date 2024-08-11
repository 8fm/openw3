/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiManager.h"
#include "redGuiMenuBar.h"
#include "redGuiMessageBox.h"

namespace DebugWindows
{
	enum EDebugWindow
	{
		DW_GameFilters,
		DW_RenderResources,
		DW_DynamicTextures,
		DW_LoadedResources,
		DW_NPCViewer,
		DW_VehicleViewer,
#ifndef FINAL
        DW_BoatSettings,
#endif
		DW_Environment, 
		DW_Sounds,
		DW_GameFrame,
		DW_ScriptThreads,
		DW_PerformanceLog,
		DW_Counters,
		DW_Settings,
		DW_InGameConfig,
		DW_Animations,
		DW_Vegetation,
		DW_GameWorld,
		DW_TickManager,
		DW_Physics,
		DW_MemoryMetrics,
		DW_TaskManager,
		DW_LoadingJobs,
		DW_Umbra,

#ifndef NO_TELEMETRY
		DW_TelemetryTags,
#endif //NO_TELEMETRY

#ifndef NO_MARKER_SYSTEMS
		DW_ReviewMarkers,
#endif	// NO_MARKER_SYSTEMS

#ifdef USE_ARRAY_METRICS
		DW_ArrayMetrics,
#endif

		DW_FileLoadingStats,
		DW_SceneStats,
		DW_Terrain,
		DW_ObjectsMemory,
		DW_Resources,
		DW_GpuResourceUse,
		DW_WorldStreaming,
		DW_TextureStreaming,

		DW_PhysicsArea,
		DW_Count,
	};

	class CDebugWindowsManager
	{
	public:
		CDebugWindowsManager();
		~CDebugWindowsManager();

		void RegisterWindow( RedGui::CRedGuiWindow* window, EDebugWindow windowId );
		void SetupMenu();

		void DestroyDebugWindows();

		Bool GetVisible() const;
		void SetVisible( Bool value, Bool keepOpenedWindows = false );

		Bool IsDebugWindowVisible( EDebugWindow type );
		void ShowDebugWindow( EDebugWindow type );
		void HideDebugWindow( EDebugWindow type );
		void HideAllVisibleWindows();

		void LockHiding();
		void UnlockHiding();

		template< typename CastClass >
		RED_INLINE CastClass* GetDebugWindow( EDebugWindow type )
		{
			return static_cast< CastClass* >(m_windows[type]);
		}

		RedGui::CRedGuiMenu* GetMenu( const String& name );

	private:
		void NotifyViewportInput( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
		void NotifyMenuItemSelected( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiMenuItem* menuItem );
		void NotifyEventScroll( RedGui::CRedGuiEventPackage& eventPackage, Float value );
		void NotifySwitchToOldDebugPage( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnCloseDebugWindows( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );
		void NotifySwitchExclusiveInput( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyOnExclusiveInputChanged( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value );
		void NotifyOnSoftwareCursoreChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void CreateMainMenuBar();

	private:
		RedGui::CRedGuiMenuBar*				m_mainMenuBar;
		RedGui::CRedGuiSlider*				m_alphaSlider;
		RedGui::CRedGuiComboBox*			m_exclusiveInput;

		RedGui::CRedGuiDesktop*				m_mainDesktop;
		TDynArray< RedGui::CRedGuiWindow* >	m_windows;

		Uint32								m_hidingLock;
	};
}	// DebugWindows

//////////////////////////////////////////////////////////////////////////
typedef TSingleton<DebugWindows::CDebugWindowsManager> GDebugWin;
//////////////////////////////////////////////////////////////////////////
extern void InitializeAndRegisterDebugWindows();

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

//////////////////////////////////////////////////////////////////////////
// Export native funtions to scripts
void ExportDebugWindowsNatives();
