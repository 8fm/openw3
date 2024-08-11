/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "updateTransformManager.h"

namespace DebugWindows
{
	class CDebugWindowTickManager : public RedGui::CRedGuiWindow
	{
		struct SInternalCounter
		{
			DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, MC_Debug );

			String			m_name;
			ETickGroup		m_tickGroup;
			Float			m_currentTime;
			Uint32			m_currentCount;
			Float			m_averageTime;
			Float			m_maxTime;

			SInternalCounter( const String& name,  ETickGroup group );
			virtual void Update( CWorld* world );
			void Reset();
		};

		struct SInternalComponentCounter : public SInternalCounter
		{
			SInternalComponentCounter( const String& name,  ETickGroup group );
			virtual void Update( CWorld* world );
		};

		struct SInternalTimerCounter : public SInternalCounter
		{
			SInternalTimerCounter( const String& name,  ETickGroup group );
			virtual void Update( CWorld* world );
		};

		struct SInternalTotalCounter : public SInternalCounter
		{
			SInternalTotalCounter( const String& name );
			virtual void Update( CWorld* world );
		};

		struct SInternalEntitiesCounter : public SInternalCounter
		{
			SInternalEntitiesCounter( const String& name );
			virtual void Update( CWorld* world );
		};

		struct SInternalEffectsCounter : public SInternalCounter
		{
			SInternalEffectsCounter( const String& name );
			virtual void Update( CWorld* world );
		};

	public:
		CDebugWindowTickManager();
		~CDebugWindowTickManager();

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifyOnResetClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyChangeTab( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* tab );

		void CreateCounters();
		void UpdateCounters();

		void CreateControls();
		void CreateDefaultRows();

		static Float TicksToTime( Uint64 ticks );

	private:
		Uint32 GetTimersGroupIndex() const { return ( Uint32 ) TICK_Max; }
		Uint32 GetAnimEventsGroupIndex() const { return ( Uint32 ) TICK_Max + 1; }

		// general
		RedGui::CRedGuiButton*	m_resetCouters;
		RedGui::CRedGuiList*	m_generalTicks;

		//groups
		RedGui::CRedGuiTab*		m_groups;
		RedGui::CRedGuiList*	m_groupsLists[TICK_Max];
		RedGui::CRedGuiList*	m_timersGroup;
		RedGui::CRedGuiList*	m_animEventsGroup;

		//logic
		TDynArray< SInternalCounter*, MC_Debug >	m_counters;

	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
