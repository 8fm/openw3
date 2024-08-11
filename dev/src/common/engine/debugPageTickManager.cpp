/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugStat.h"
#include "debugPage.h"
#ifndef NO_DEBUG_PAGES

#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#include "debugPageManagerBase.h"
#include "inputKeys.h"
#include "game.h"
#include "renderFrame.h"
#include "updateTransformManager.h"
#include "world.h"
#include "tickManager.h"
#include "inputBufferedInputEvent.h"

class CDebugPageTickManager : public IDebugPage
{
private:
	//! Default stat counter with history
	typedef StatChart< 256 > DefualtStatChar;

	//! Total tick stats
	class TickGroupTotals : public DefualtStatChar
	{
	public:
		TickGroupTotals()
			: DefualtStatChar( TXT("Total"), 0.030f )
		{};

		//! Update
		virtual void Update( CWorld* world )
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

			// convert totime
			DefualtStatChar::Update( TicksToTime( totalTime ), 0 );
		}
	};

	//! Tick group stats
	class TickGroupComponentStats : public DefualtStatChar
	{
	private:
		ETickGroup	m_tickGroup;

	public:
		TickGroupComponentStats( const Char* name, Float limit, ETickGroup group )
			: DefualtStatChar( name, limit )
			, m_tickGroup( group )
		{};

		//! Update
		virtual void Update( CWorld* world )
		{
			const CComponentTickManager::TickGroupStats& stats = world->GetTickManager()->GetGroupComponentStats( m_tickGroup );
			DefualtStatChar::Update( TicksToTime( stats.m_statsTime ), stats.m_statsCount );
		}
	};

	//! Tick group stats for timers
	class TickGroupTimersStats : public DefualtStatChar
	{
	private:
		ETickGroup	m_tickGroup;

	public:
		TickGroupTimersStats( const Char* name, Float limit, ETickGroup group )
			: DefualtStatChar( name, limit )
			, m_tickGroup( group )
		{};

		//! Update
		virtual void Update( CWorld* world )
		{
			const STickGenericStats& stats = world->GetTickManager()->GetGroupTimersStats( m_tickGroup );
			DefualtStatChar::Update( TicksToTime( stats.m_statsTime ), stats.m_statsCount );
		}
	};

	//! Tick group stats for entity tick
	class TickGroupEntities : public DefualtStatChar
	{
	public:
		TickGroupEntities( const Char* name, Float limit )
			: DefualtStatChar( name, limit )
		{};

		//! Update
		virtual void Update( CWorld* world )
		{
			const STickGenericStats& stats = world->GetTickManager()->GetEntitiesStats();
			DefualtStatChar::Update( TicksToTime( stats.m_statsTime ), stats.m_statsCount );
		}
	};

	//! Tick group stats for effects tick
	class TickGroupEffects : public DefualtStatChar
	{
	public:
		TickGroupEffects( const Char* name, Float limit )
			: DefualtStatChar( name, limit )
		{};

		//! Update
		virtual void Update( CWorld* world )
		{
			const STickGenericStats& stats = world->GetTickManager()->GetEffectStats();
			DefualtStatChar::Update( TicksToTime( stats.m_statsTime ), stats.m_statsCount );
		}
	};
		
	//! Per component crap
	struct DisplayItem
	{
		CClass*		m_class;
		Float		m_totalTime;
		Float		m_avgTime;
		Uint32		m_count;

		DisplayItem( const CComponentTickManager::TickGroupClassStats& stats )
			: m_class( stats.m_class )
			, m_count( stats.m_count )
		{
			m_totalTime = BaseStatChart::TicksToTime( stats.m_time );
			m_avgTime = m_totalTime / (Float)stats.m_count;
		};

		static int Compare( const void* a, const void* b )
		{
			const DisplayItem* aa = ( const DisplayItem* ) a;
			const DisplayItem* bb = ( const DisplayItem* ) b;
			if ( aa->m_totalTime > bb->m_totalTime ) return -1;
			if ( aa->m_totalTime < bb->m_totalTime ) return 1;
			return 0;
		}
	};

private:
	ETickGroup						m_showGroup;			//!< Active preview group
	TDynArray< BaseStatChart* >		m_counters;				//!< Stat counters
	TDynArray< DisplayItem >		m_components;			//!< Per components stats

public:
	CDebugPageTickManager()
		: IDebugPage( TXT("Tick Manager") )
		, m_showGroup( TICK_PrePhysics )
	{
		//! Create total time counter
		m_counters.PushBack( new TickGroupTotals() );

		//! Create component tick group counters
		m_counters.PushBack( new TickGroupComponentStats( TXT("PrePhysics"), 0.015f, TICK_PrePhysics ) );
		m_counters.PushBack( new TickGroupComponentStats( TXT("PrePhysicsPost"), 0.015f, TICK_PrePhysicsPost ) );
		m_counters.PushBack( new TickGroupComponentStats( TXT("Main"), 0.010f, TICK_Main ) );
		m_counters.PushBack( new TickGroupComponentStats( TXT("PostPhysics"), 0.010f, TICK_PostPhysics ) );
		m_counters.PushBack( new TickGroupComponentStats( TXT("PostPhysicsPost"), 0.010f, TICK_PostPhysicsPost ) );
		m_counters.PushBack( new TickGroupComponentStats( TXT("PostUT"), 0.010f, TICK_PostUpdateTransform ) );

		//! Timer counters
		m_counters.PushBack( new TickGroupTimersStats( TXT("PrePhysics Timers"), 0.002f, TICK_PrePhysics ) );
		m_counters.PushBack( new TickGroupTimersStats( TXT("PrePhysicsPost Timers"), 0.002f, TICK_PrePhysicsPost ) );
		m_counters.PushBack( new TickGroupTimersStats( TXT("Main Timers"), 0.002f, TICK_Main ) );
		m_counters.PushBack( new TickGroupTimersStats( TXT("PostPhysics Timers"), 0.002f, TICK_PostPhysics ) );
		m_counters.PushBack( new TickGroupTimersStats( TXT("PostPhysicsPost Timers"), 0.002f, TICK_PostPhysicsPost ) );
		m_counters.PushBack( new TickGroupTimersStats( TXT("PostUT Timers"), 0.002f, TICK_PostUpdateTransform ) );

		//! Special
		m_counters.PushBack( new TickGroupEntities( TXT("Entities"), 0.003f ) );
		m_counters.PushBack( new TickGroupEffects( TXT("Effects"), 0.003f ) );
	};

	//! Page shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();
	}

	~CDebugPageTickManager()
	{
		m_counters.ClearPtr();
	}

	void ResetPeek()
	{
		for ( Uint32 i=0; i<m_counters.Size(); ++i )
		{
			m_counters[i]->Reset();
		}
	}

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

		// Print crap
		Uint32 x = 50;
		Uint32 y = 65;

		// Header
		frame->AddDebugScreenText( x, y, TXT("Group"), Color::YELLOW );
		frame->AddDebugScreenText( x + 420, y, TXT("Count"), Color::YELLOW );
		frame->AddDebugScreenText( x + 480, y, TXT("Avg"), Color::YELLOW );
		frame->AddDebugScreenText( x + 540, y, TXT("Peek"), Color::YELLOW );
		y += 5;

		// Draw counters
		for ( Uint32 i=0; i<m_counters.Size(); ++i )
		{
			const Uint32 barWidth = 400;
			m_counters[i]->DrawBar( frame, x, y, barWidth );
		}

		// Type
		const Char* groupName = TXT("unknown");
		switch ( m_showGroup )
		{
			case TICK_PrePhysics: groupName = TXT("PrePhysics"); break;
			case TICK_PrePhysicsPost: groupName = TXT("PrePhysicsPost"); break;
			case TICK_Main: groupName = TXT("Main"); break;
			case TICK_PostPhysics: groupName = TXT("PostPhysics"); break;
			case TICK_PostPhysicsPost: groupName = TXT("PostPhysicsPost"); break;
			case TICK_PostUpdateTransform: groupName = TXT("PostUpdateTransform"); break;
		}

		// Header
		y += 20;
		frame->AddDebugScreenFormatedText( x, y, Color::YELLOW, TXT("PerComponents stats in group '%ls'"), groupName );
		y += 15;

		// Display header
		frame->AddDebugScreenText( x, y, TXT("Component class"), Color::YELLOW );
		frame->AddDebugScreenText( x+400, y, TXT("Count"), Color::YELLOW );
		frame->AddDebugScreenText( x+450, y, TXT("Time [us]"), Color::YELLOW );
		frame->AddDebugScreenText( x+540, y, TXT("AvgTime [us]"), Color::YELLOW );
		y += 15;

		// Display per class info
		for ( Uint32 i=0; i<m_components.Size(); ++i )
		{
			const DisplayItem& item = m_components[i];
			frame->AddDebugScreenFormatedText( x, y, TXT("%s"), item.m_class->GetName().AsString().AsChar() );
			frame->AddDebugScreenFormatedText( x+400, y, TXT("%i"), item.m_count );
			frame->AddDebugScreenFormatedText( x+450, y, TXT("%1.0f"), item.m_totalTime * 1000000.0f );
			frame->AddDebugScreenFormatedText( x+540, y, TXT("%1.2f"), item.m_avgTime * 1000000.0f );

			// Move down
			y += 15;
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible( true );
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_TickManager );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// Reset peek
		if ( (key == IK_R || key == IK_Pad_X_SQUARE) && action == IACT_Press )
		{
			ResetPeek();
			return true;
		}

		// Change group
		if ( (key == IK_C || key == IK_Pad_Y_TRIANGLE) && action == IACT_Press )
		{
			m_showGroup = (ETickGroup)( (Uint32)(m_showGroup+1) % TICK_Max );
			return true;
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		// Cleanup
		m_components.ClearFast();

		// Only if have world
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			// Update counters
			for ( Uint32 i=0; i<m_counters.Size(); ++i )
			{
				m_counters[i]->Update( world );
			}

			// Update components
			const CComponentTickManager::TickGroupStats& stats = GGame->GetActiveWorld()->GetTickManager()->GetGroupComponentStats( m_showGroup );
			for ( Uint32 i=0; i<stats.m_numGroups; ++i )
			{
				new ( m_components ) DisplayItem( stats.m_statGroups[i] );
			}

			// Sort the crap
			qsort( m_components.TypedData(), m_components.Size(), sizeof( DisplayItem ), DisplayItem::Compare );
		}
	}
};

void CreateDebugPageTickManager()
{
	IDebugPage* page = new CDebugPageTickManager();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}
#endif

#endif