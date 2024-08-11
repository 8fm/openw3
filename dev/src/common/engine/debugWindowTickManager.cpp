/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiManager.h"
#include "redGuiGridLayout.h"
#include "redGuiList.h"
#include "redGuiTab.h"
#include "redGuiButton.h"
#include "redGuiScrollPanel.h"
#include "debugWindowTickManager.h"
#include "game.h"
#include "updateTransformManager.h"
#include "tickManager.h"
#include "world.h"
#include "extAnimEvent.h"

namespace DebugWindows
{
	//////////////////////////////////////////////////////////////////////////
	//
	CDebugWindowTickManager::SInternalCounter::SInternalCounter( const String& name,  ETickGroup group )
		: m_name( name )
		, m_tickGroup( group )
		, m_currentTime( 0.0f )
		, m_currentCount( 0 )
		, m_averageTime( 0.0f )
		, m_maxTime( 0.0f )
	{
		/* intentionally empty */
	}

	void CDebugWindowTickManager::SInternalCounter::Update( CWorld* world )
	{
		m_maxTime = Max( m_maxTime, m_currentTime );
		m_averageTime = ( m_averageTime + m_currentTime ) / 2.0f;
	}

	void CDebugWindowTickManager::SInternalCounter::Reset()
	{
		m_currentCount = 0;
		m_currentTime = 0.0f;
		m_averageTime = 0.0f;
		m_maxTime = 0.0f;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	CDebugWindowTickManager::SInternalComponentCounter::SInternalComponentCounter( const String& name,  ETickGroup group )
		: SInternalCounter( name, group )
	{
		/* intentionally empty */
	}

	void CDebugWindowTickManager::SInternalComponentCounter::Update( CWorld* world )
	{
		const CComponentTickManager::TickGroupStats& stats = world->GetTickManager()->GetGroupComponentStats( m_tickGroup );

		//
		m_currentTime = TicksToTime( stats.m_statsTime );
		m_currentCount = stats.m_statsCount;

		SInternalCounter::Update( world );
	}


	//////////////////////////////////////////////////////////////////////////
	//
	CDebugWindowTickManager::SInternalTimerCounter::SInternalTimerCounter( const String& name,  ETickGroup group )
		: SInternalCounter( name, group )
	{
		/* intentionally empty */
	}

	void CDebugWindowTickManager::SInternalTimerCounter::Update( CWorld* world )
	{
		const STickGenericStats& stats = world->GetTickManager()->GetGroupTimersStats( m_tickGroup );

		//
		m_currentTime = TicksToTime( stats.m_statsTime );
		m_currentCount = stats.m_statsCount;

		SInternalCounter::Update( world );
	}


	//////////////////////////////////////////////////////////////////////////
	//
	CDebugWindowTickManager::SInternalTotalCounter::SInternalTotalCounter( const String& name )
		: SInternalCounter( name, TICK_Max )
	{
		/* intentionally empty */
	}

	void CDebugWindowTickManager::SInternalTotalCounter::Update( CWorld* world )
	{
		Uint64 totalTime = 0;
		totalTime += world->GetTickManager()->GetGroupComponentStats( TICK_PrePhysics ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupComponentStats( TICK_PrePhysicsPost ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupComponentStats( TICK_Main ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupComponentStats( TICK_PostPhysics ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupComponentStats( TICK_PostPhysicsPost ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupComponentStats( TICK_PostUpdateTransform ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupTimersStats( TICK_PrePhysics ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupTimersStats( TICK_PrePhysicsPost ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupTimersStats( TICK_Main ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupTimersStats( TICK_PostPhysics ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupTimersStats( TICK_PostPhysicsPost ).m_statsTime;
		totalTime += world->GetTickManager()->GetGroupTimersStats( TICK_PostUpdateTransform ).m_statsTime;
		totalTime += world->GetTickManager()->GetEntitiesStats().m_statsTime;
		totalTime += world->GetTickManager()->GetEffectStats().m_statsTime;

		m_currentTime = TicksToTime( totalTime );
		m_currentCount = 0;

		SInternalCounter::Update( world );
	}


	//////////////////////////////////////////////////////////////////////////
	//
	CDebugWindowTickManager::SInternalEntitiesCounter::SInternalEntitiesCounter( const String& name )
	: SInternalCounter( name, TICK_Max )
	{
		/* intentionally empty */
	}

	void CDebugWindowTickManager::SInternalEntitiesCounter::Update( CWorld* world )
	{
		const STickGenericStats& stats = world->GetTickManager()->GetEntitiesStats();

		//
		m_currentTime = TicksToTime( stats.m_statsTime );
		m_currentCount = stats.m_statsCount;

		SInternalCounter::Update( world );
	}


	//////////////////////////////////////////////////////////////////////////
	//
	CDebugWindowTickManager::SInternalEffectsCounter::SInternalEffectsCounter( const String& name )
		: SInternalCounter( name, TICK_Max )
	{
		/* intentionally empty */
	}

	void CDebugWindowTickManager::SInternalEffectsCounter::Update( CWorld* world )
	{
		const STickGenericStats& stats = world->GetTickManager()->GetEffectStats();

		//
		m_currentTime = TicksToTime( stats.m_statsTime );
		m_currentCount = stats.m_statsCount;

		SInternalCounter::Update( world );
	}


	//////////////////////////////////////////////////////////////////////////
	//
	CDebugWindowTickManager::CDebugWindowTickManager()
		: RedGui::CRedGuiWindow(200,200,800,500)
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowTickManager::NotifyOnTick );
		SetCaption( TXT("Tick manager") );

		CreateControls();
		CreateCounters();
	}

	CDebugWindowTickManager::~CDebugWindowTickManager()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowTickManager::NotifyOnTick );
	}

	void CDebugWindowTickManager::CreateControls()
	{
		m_resetCouters = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		m_resetCouters->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_resetCouters->SetText( TXT("Reset peek times") );
		m_resetCouters->SetDock( RedGui::DOCK_Top );
		m_resetCouters->EventButtonClicked.Bind( this, &CDebugWindowTickManager::NotifyOnResetClicked );
		AddChild( m_resetCouters );

		m_generalTicks = new RedGui::CRedGuiList( 0, 0, 100, 230 );
		m_generalTicks->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_generalTicks->AppendColumn( TXT("Group name"), 300 );
		m_generalTicks->AppendColumn( TXT("Count"), 120, RedGui::SA_Integer );
		m_generalTicks->AppendColumn( TXT("Time [ms]"), 120, RedGui::SA_Real );
		m_generalTicks->AppendColumn( TXT("Avg time [ms]"), 120, RedGui::SA_Real );
		m_generalTicks->AppendColumn( TXT("Peek time [ms]"), 120, RedGui::SA_Real );
		m_generalTicks->SetSorting( true );
		m_generalTicks->SetDock( RedGui::DOCK_Top );
		AddChild( m_generalTicks );
		CreateDefaultRows();

		m_groups = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_groups->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_groups->AddTab( TXT("PrePhysic") );
		m_groups->AddTab( TXT("PrePhysicPost") );
		m_groups->AddTab( TXT("Main") );
		m_groups->AddTab( TXT("PostPhysic") );
		m_groups->AddTab( TXT("PostPhysicPost") );
		m_groups->AddTab( TXT("PostUT") );
		m_groups->AddTab( TXT("Timers detailed view") );
		m_groups->AddTab( TXT("Animation events") );
		m_groups->SetDock( RedGui::DOCK_Fill );
		m_groups->SetActiveTab( 0 );
		m_groups->EventTabChanged.Bind( this, &CDebugWindowTickManager::NotifyChangeTab );
		AddChild( m_groups );

		for( Uint32 i=0; i<TICK_Max; ++i )
		{
			m_groupsLists[i] = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_groupsLists[i]->SetDock( RedGui::DOCK_Fill );
			m_groupsLists[i]->SetMargin( Box2( 3, 3, 3, 3 ) );
			m_groupsLists[i]->AppendColumn( TXT("Name"), 400 );
			m_groupsLists[i]->AppendColumn( TXT("Count"), 150 );
			m_groupsLists[i]->AppendColumn( TXT("Time [um]"), 150 );
			m_groups->GetTabAt( i )->AddChild( m_groupsLists[i] );
		}

		m_timersGroup = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		m_timersGroup->SetDock( RedGui::DOCK_Fill );
		m_timersGroup->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_timersGroup->AppendColumn( TXT("Name"), 300 );
		m_timersGroup->AppendColumn( TXT("Count"), 80, RedGui::SA_Integer );
		m_timersGroup->AppendColumn( TXT("Time [ms]"), 100, RedGui::SA_Real );
		m_timersGroup->AppendColumn( TXT("Old count"), 80, RedGui::SA_Integer );
		m_timersGroup->AppendColumn( TXT("Old time [ms]"), 100, RedGui::SA_Real );
		m_timersGroup->SetSorting( true );
		m_groups->GetTabAt( GetTimersGroupIndex() )->AddChild( m_timersGroup );

		m_animEventsGroup = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		m_animEventsGroup->SetDock( RedGui::DOCK_Fill );
		m_animEventsGroup->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_animEventsGroup->AppendColumn( TXT("Name"), 300 );
		m_animEventsGroup->AppendColumn( TXT("Count"), 80, RedGui::SA_Integer );
		m_animEventsGroup->AppendColumn( TXT("Time [ms]"), 100, RedGui::SA_Real );
		m_animEventsGroup->AppendColumn( TXT("Old count"), 80, RedGui::SA_Integer );
		m_animEventsGroup->AppendColumn( TXT("Old time [ms]"), 100, RedGui::SA_Real );
		m_animEventsGroup->SetSorting( true );
		m_groups->GetTabAt( GetAnimEventsGroupIndex() )->AddChild( m_animEventsGroup );
	}

	void CDebugWindowTickManager::CreateDefaultRows()
	{
		m_generalTicks->AddItem( TXT("Total") );

		m_generalTicks->AddItem( TXT("PrePhysic") );
		m_generalTicks->AddItem( TXT("Main") );
		m_generalTicks->AddItem( TXT("PostPhysic") );
		m_generalTicks->AddItem( TXT("PostUT") );

		m_generalTicks->AddItem( TXT("PrePhysic timers") );
		m_generalTicks->AddItem( TXT("Main timers") );
		m_generalTicks->AddItem( TXT("PostPhysic timers") );
		m_generalTicks->AddItem( TXT("PostUT timers") );

		m_generalTicks->AddItem( TXT("Entities") );
		m_generalTicks->AddItem( TXT("Effects") );
	}

	void CDebugWindowTickManager::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() == false )
		{
			return;
		}

		UpdateCounters();
	}

	void CDebugWindowTickManager::CreateCounters()
	{
		//! Create total time counter
		m_counters.PushBack( new SInternalTotalCounter( TXT("Total") ) );

		//! Create component tick group counters
		m_counters.PushBack( new SInternalComponentCounter( TXT("PrePhysic"), TICK_PrePhysics ) );
		m_counters.PushBack( new SInternalComponentCounter( TXT("PrePhysicPost"), TICK_PrePhysicsPost ) );
		m_counters.PushBack( new SInternalComponentCounter( TXT("Main"), TICK_Main ) );
		m_counters.PushBack( new SInternalComponentCounter( TXT("PostPhysic"), TICK_PostPhysics ) );
		m_counters.PushBack( new SInternalComponentCounter( TXT("PostPhysicPost"), TICK_PostPhysicsPost ) );
		m_counters.PushBack( new SInternalComponentCounter( TXT("PostUT"), TICK_PostUpdateTransform ) );

		//! Timer counters
		m_counters.PushBack( new SInternalTimerCounter( TXT("PrePhysic timers"), TICK_PrePhysics ) );
		m_counters.PushBack( new SInternalTimerCounter( TXT("PrePhysicPost timers"), TICK_PrePhysicsPost ) );
		m_counters.PushBack( new SInternalTimerCounter( TXT("Main timers"), TICK_Main ) );
		m_counters.PushBack( new SInternalTimerCounter( TXT("PostPhysic timers"), TICK_PostPhysics ) );
		m_counters.PushBack( new SInternalTimerCounter( TXT("PostPhysicPost timers"), TICK_PostPhysicsPost ) );
		m_counters.PushBack( new SInternalTimerCounter( TXT("PostUT timers"), TICK_PostUpdateTransform ) );

		//! Special
		m_counters.PushBack( new SInternalEntitiesCounter( TXT("Entities") ) );
		m_counters.PushBack( new SInternalEffectsCounter( TXT("Effects") ) );
	}

	void CDebugWindowTickManager::UpdateCounters()
	{
		if( GGame != nullptr )
		{
			CWorld* world = GGame->GetActiveWorld();
			if ( world != nullptr )
			{
				// Update counters
				const Uint32 counterCount = m_counters.Size();
				for ( Uint32 i=0; i<counterCount; ++i )
				{
					m_counters[i]->Update( world );

					Int32 index = m_generalTicks->Find( m_counters[i]->m_name );
					if( index != -1 )
					{
						m_generalTicks->SetItemText( String::Printf( TXT("%i"), m_counters[i]->m_currentCount ), index, 1 );
						m_generalTicks->SetItemText( String::Printf( TXT("%1.2f"), m_counters[i]->m_currentTime * 1000.0f ), index, 2 );
						m_generalTicks->SetItemText( String::Printf( TXT("%1.2f"), m_counters[i]->m_averageTime * 1000.0f ), index, 3 );
						m_generalTicks->SetItemText( String::Printf( TXT("%1.2f"), m_counters[i]->m_maxTime * 1000.0f ), index, 4 );
					}
				}

				// Update components
				Int32 activeIndex = m_groups->GetActiveTabIndex();
				if( activeIndex != -1 && (ETickGroup)activeIndex < TICK_Max )
				{
					RedGui::CRedGuiList* list = m_groupsLists[ activeIndex ];
					const CComponentTickManager::TickGroupStats& stats = GGame->GetActiveWorld()->GetTickManager()->GetGroupComponentStats( (ETickGroup)activeIndex );
					for ( Uint32 i=0; i<stats.m_numGroups; ++i )
					{
						String name = stats.m_statGroups[i].m_class->GetName().AsString();
						Int32 index = list->Find( name );

						if( index != -1 )
						{
							list->SetItemText( String::Printf( TXT("%i"), stats.m_statGroups[i].m_count ), index, 1 );
							list->SetItemText( String::Printf( TXT("%1.2f"), TicksToTime( stats.m_statGroups[i].m_time ) * 1000000.0f ), index, 2 );
						}
						else
						{
							Uint32 newIndex = list->AddItem( name );
							list->SetItemText( String::Printf( TXT("%i"), stats.m_statGroups[i].m_count ), newIndex, 1 );
							list->SetItemText( String::Printf( TXT("%1.2f"), TicksToTime( stats.m_statGroups[i].m_time ) * 1000000.0f ), newIndex, 2 );
						}
					}
				}

				// Update animation events

				else if ( activeIndex == GetAnimEventsGroupIndex() )
				{
					// Reset

					const String zeroIntString( TXT("0") );
					const String zeroFloatString( TXT("0.0") );
					for ( Uint32 i = 0; i < m_animEventsGroup->GetItemCount(); ++i )
					{
						m_animEventsGroup->SetItemText( zeroIntString, i, 1 );
						m_animEventsGroup->SetItemText( zeroFloatString, i, 2 );
					}

					// Update

					Uint32 totalCount = 0;
					Float totalTimeMS = 0.0f;

					const CExtAnimEvent::StatsCollector::EventStatsMap& stats = CExtAnimEvent::StatsCollector::GetInstance().GetStats();
					for ( const CExtAnimEvent::StatsCollector::EventStats& stat : stats )
					{
						const String name = stat.m_name.AsString();
						Int32 index = m_animEventsGroup->Find( name );
						if ( index == -1 )
						{
							index = m_animEventsGroup->AddItem( name );
						}

						const String countString = String::Printf( TXT("%i"), stat.m_count );
						const String timeString = String::Printf( TXT("%1.2f"), stat.m_timeMS );
						m_animEventsGroup->SetItemText( countString, index, 1 );
						m_animEventsGroup->SetItemText( timeString, index, 2 );
						m_animEventsGroup->SetItemText( countString, index, 3 );
						m_animEventsGroup->SetItemText( timeString, index, 4 );

						totalCount += stat.m_count;
						totalTimeMS += stat.m_timeMS;
					}

					// Update total

					const String name = TXT("total");
					Int32 index = m_animEventsGroup->Find( name );
					if ( index == -1 )
					{
						index = m_animEventsGroup->AddItem( name );
					}

					const String countString = String::Printf( TXT("%i"), totalCount );
					const String timeString = String::Printf( TXT("%1.2f"), totalTimeMS );
					m_animEventsGroup->SetItemText( countString, index, 1 );
					m_animEventsGroup->SetItemText( timeString, index, 2 );
					m_animEventsGroup->SetItemText( countString, index, 3 );
					m_animEventsGroup->SetItemText( timeString, index, 4 );
				}

				// Update timers

				else if ( activeIndex == GetTimersGroupIndex() )
				{
					// Reset

					const String zeroIntString( TXT("0") );
					const String zeroFloatString( TXT("0.0") );
					for ( Uint32 i = 0; i < m_timersGroup->GetItemCount(); ++i )
					{
						m_timersGroup->SetItemText( zeroIntString, i, 1 );
						m_timersGroup->SetItemText( zeroFloatString, i, 2 );
					}

					// Update

					const TDynArray< CTimerManager::FiredTimerStats >& stats = GGame->GetActiveWorld()->GetTickManager()->GetTimerManager().SortAndGetFiredTimerStats();
					for ( const CTimerManager::FiredTimerStats& stat : stats )
					{
						const String name = stat.m_name.AsString();
						Int32 index = m_timersGroup->Find( name );
						if ( index == -1 )
						{
							index = m_timersGroup->AddItem( name );
						}

						const String countString = String::Printf( TXT("%i"), stat.m_count );
						const String timeString = String::Printf( TXT("%1.2f"), stat.m_timeMS );
						m_timersGroup->SetItemText( countString, index, 1 );
						m_timersGroup->SetItemText( timeString, index, 2 );
						m_timersGroup->SetItemText( countString, index, 3 );
						m_timersGroup->SetItemText( timeString, index, 4 );
					}
				}
			}
		}
	}

	void CDebugWindowTickManager::NotifyOnResetClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		const Uint32 counterCount = m_counters.Size();
		for( Uint32 i=0; i<counterCount; ++i )
		{
			m_counters[i]->Reset();
		}
	}

	void CDebugWindowTickManager::NotifyChangeTab( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* tab )
	{
		RED_UNUSED( eventPackage );

		for( Uint32 i=0; i<TICK_Max; ++i )
		{
			m_groupsLists[i]->RemoveAllItems();
		}
	}

	Float CDebugWindowTickManager::TicksToTime( Uint64 ticks )
	{
		// Get tick frequency
		static Uint64 frequency = 0;
		if ( frequency == 0 )
		{
			Red::System::Clock::GetInstance().GetTimer().GetFrequency( frequency );
		}

		// Convert time
		return ( Float )( (Double)ticks / (Double)frequency );
	}

}	// namespace DebugWindows

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
