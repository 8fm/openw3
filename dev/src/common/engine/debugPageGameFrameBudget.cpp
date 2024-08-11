/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "debugStat.h"
#include "debugWindowsManager.h"
#include "debugCheckBox.h"
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "inputBufferedInputEvent.h"
#include "renderFrame.h"

#define BUDGET_ELEM_SIZE 10

class CDebugPageGameFrameBudget : public IDebugPage
{
	enum EFrameBudgetElem
	{
		FBE_Gameplay,
		FBE_Scripts,
		FBE_Movement,
		FBE_Animation,
		FBE_Physics,
		FBE_UpdateTransform,
		FBE_Sounds,
		FBE_Renderer,
		FBE_Hud,
		FBE_RenderFence,
		FBE_Last
	};

	struct SFrameBudgetElem
	{
		struct SPerfCounterStat
		{
			CPerfCounter*	m_counter;
			Uint64			m_lastTime;
			Uint64			m_lastIncr;

			SPerfCounterStat() : m_counter( NULL ), m_lastTime( 0 ), m_lastIncr( 0 ) {}
		};

		struct SPerfCounterStatArray
		{
			TDynArray< SPerfCounterStat >	m_counters;
			String							m_name;

			Bool HasCounter( const CPerfCounter* c ) const
			{
				const Uint32 size = m_counters.Size();
				for ( Uint32 i=0; i<size; ++i )
				{
					if ( m_counters[ i ].m_counter == c )
					{
						return true;
					}
				}

				return false;
			}

			void AddCounter( CPerfCounter* c )
			{
				Int32 index = static_cast< Int32 >( m_counters.Grow( 1 ) );
				m_counters[ index ].m_counter  = c;
				m_counters[ index ].m_lastTime = 0;
			}

			void Update( Uint64& accTime, Bool acc )
			{
				const Uint32 sizeAdd = m_counters.Size();
				for ( Uint32 i=0; i<sizeAdd; ++i )
				{
					SPerfCounterStat& counter = m_counters[ i ];
					if ( counter.m_counter )
					{
						counter.m_lastIncr = counter.m_counter->GetTotalTime() - counter.m_lastTime;
						counter.m_lastTime = counter.m_counter->GetTotalTime();

						accTime = acc ? accTime + counter.m_lastIncr : accTime - counter.m_lastIncr;
					}
				}
			}

			void Collect()
			{
				TDynArray< CPerfCounter* > counters;
				CProfiler::GetCounters( UNICODE_TO_ANSI( m_name.AsChar() ), counters, 0 );

				const Uint32 size = counters.Size();
				for ( Uint32 i=0; i<size; ++i )
				{
					CPerfCounter* c = counters[ i ];

					if ( !HasCounter( c ) )
					{
						AddCounter( c );
					}
				}
			}

			Float GetAverageTime( Double freq ) const
			{
				Uint64 time = 0;

				const Uint32 size = m_counters.Size();
				for ( Uint32 i=0; i<size; ++i )
				{
					time += m_counters[ i ].m_lastIncr;
				}

				return (Float)(time / freq);
			}

			Uint32 CounterSize() const
			{
				return m_counters.Size();
			}
		};

	private:
		TDynArray< SPerfCounterStatArray >	m_countersAdd;
		TDynArray< SPerfCounterStatArray >	m_countersRemove;

		Uint64							m_times[ BUDGET_ELEM_SIZE ];
		Int32								m_curr;
		Double							m_freq;

		Double							m_avg;
		Double							m_percent;

		Float							m_timeBudget;

		Double CalcAverageTime() const
		{
			Uint64 incr = 0;

			for ( int i=0; i<BUDGET_ELEM_SIZE; ++i )
			{
				incr += m_times[ i ];
			}

			return incr / ( m_freq * BUDGET_ELEM_SIZE );
		}

	public:
		SFrameBudgetElem()
			: m_timeBudget( 1.f )
		{
			Reset();
		}

		void AddPerf( const String& name )
		{
			Int32 index = static_cast< Int32 >( m_countersAdd.Grow( 1 ) );
			m_countersAdd[ index ].m_name = name;
		}

		void RemovePerf( const String& name )
		{
			Int32 index = static_cast< Int32 >( m_countersRemove.Grow( 1 ) );
			m_countersRemove[ index ].m_name = name;
		}

		Float GetTimeBudget() const
		{
			return m_timeBudget;
		}

		void SetTimeBudget( Float t )
		{
			m_timeBudget = t;
		}

		void Reset()
		{
			m_curr = 0;

			m_avg = 0.0;
			m_percent = 0.0;

			Red::System::MemorySet( m_times, 0, BUDGET_ELEM_SIZE * sizeof( Uint64 ) );

			Uint64 freq;
			Red::System::Clock::GetInstance().GetTimer().GetFrequency( freq );
			m_freq = freq/1000.0;
		}

		void Update()
		{
			m_times[ m_curr ] = 0;

			const Uint32 sizeAdd = m_countersAdd.Size();
			for ( Uint32 i=0; i<sizeAdd; ++i )
			{
				m_countersAdd[ i ].Update( m_times[ m_curr ], true );
			}
			
			const Uint32 sizeRemove = m_countersRemove.Size();
			for ( Uint32 i=0; i<sizeRemove; ++i )
			{
				m_countersRemove[ i ].Update( m_times[ m_curr ], false );
			}

			m_curr = ( m_curr+1 ) % BUDGET_ELEM_SIZE;

			m_avg = CalcAverageTime();
		}

		void UpdateAndCalc( Double totalTime, Double& accTime )
		{
			Update();

			accTime += m_avg;

			m_percent = m_avg / totalTime;
		}

		Double GetAverageTime() const
		{
			return m_avg;
		}

		Double GetPercent() const
		{
			return m_percent;
		}

		void Draw( CRenderFrame *frame, Int32 w, Int32 h, Int32& currX, Int32 y, Color color )
		{
			Int32 barWidth = Clamp< Int32 >( (Int32)( m_percent*w ), 0, w );

			frame->AddDebugRect( currX, y, barWidth, h, color );

			currX += barWidth;
		}

		void CollectPerfs()
		{
			const Uint32 sizeAdd = m_countersAdd.Size();
			const Uint32 sizeRemove = m_countersRemove.Size();

			for ( Uint32 i=0; i<sizeAdd; ++i )
			{
				m_countersAdd[ i ].Collect();
			}

			for ( Uint32 i=0; i<sizeRemove; ++i )
			{
				m_countersRemove[ i ].Collect();
			}
		}

	public:
		const TDynArray< SPerfCounterStatArray >& GetCountersAdd()		{ return m_countersAdd; }
		const TDynArray< SPerfCounterStatArray >& GetCountersRemove()	{ return m_countersRemove; }
	};

	TDynArray< SFrameBudgetElem >		m_elems;
	SFrameBudgetElem					m_allBudget;
	Double								m_restPercent;
	Double								m_restTime;
	Bool								m_showTree;
	CDebugOptionsTree*					m_tree;
	Float								m_timeForSearch;
	Bool								m_showBudgets;
RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4512 )
RED_MESSAGE( "CDebugFrameBudgetElemBox needs to address warning 4512 ( No assignment operator can be generated )" )
	class CDebugFrameBudgetElemBox : public IDebugCheckBox
	{
		typedef StatChart< 64 > DebugFrameBudgetStatClass;

		class DebugFrameBudgetStat : public DebugFrameBudgetStatClass
		{
		public:
			DebugFrameBudgetStat( const Char* name, Float limit ) : StatChart< 64 >( name, limit ) {}
			virtual void Update( CWorld* world ) {}
			virtual Uint32 GetBarHeight() const { return 13; }
		};

		const SFrameBudgetElem::SPerfCounterStatArray&	m_stats;
		Double											m_freq;
		DebugFrameBudgetStat							m_statChar;

	public:
		CDebugFrameBudgetElemBox( IDebugCheckBox* parent, const SFrameBudgetElem::SPerfCounterStatArray& stat ) 
			: IDebugCheckBox( parent, stat.m_name, false, false )
			, m_stats( stat ) 
			, m_statChar( TXT(""), 0.001f )
		{
			Uint64 freq;
			Red::System::Clock::GetInstance().GetTimer().GetFrequency( freq );
			m_freq = (Double)(freq/1000.0);
		}

		virtual void OnTick( Float timeDelta )
		{
			IDebugCheckBox::OnTick( timeDelta );

			m_statChar.DebugFrameBudgetStatClass::Update( m_stats.GetAverageTime( m_freq ) / 1000.f , 0 );
			m_statChar.FitLimit();
		}

		virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
		{
			Uint32 tempY = y;

			IDebugCheckBox::OnRender( frame, x, y, counter, options );

			String str = String::Printf( TXT("%1.2f (%u)"), m_stats.GetAverageTime( m_freq ), m_stats.CounterSize() );

			frame->AddDebugScreenText( x + 200 + 1, tempY + 1, str, Color::BLACK );
			frame->AddDebugScreenText( x + 200, tempY, str, Color::GREEN );

			tempY -= 10;
			m_statChar.DrawBar( frame, x + 250, tempY, 200 );
		}
	};
RED_WARNING_POP()
public:
	CDebugPageGameFrameBudget()
		: IDebugPage( TXT("Game Frame") )
		, m_restPercent( 0.0 )
		, m_restTime( 0.0 )
		, m_tree( NULL )
		, m_showTree( false )
		, m_timeForSearch( 0.f )
		, m_showBudgets( false )
	{
		m_allBudget.AddPerf( TXT("BaseEngineTick") );

		m_elems.Resize( FBE_Last );

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Gameplay ];
			elem.SetTimeBudget( 6.f );
			elem.AddPerf( TXT("ActorsStorageConsistencyCheck") );
			elem.AddPerf( TXT("LightsStorageConsistencyCheck") );
			elem.AddPerf( TXT("CommunityTick") );
			elem.AddPerf( TXT("QuestSystemTick") );
			elem.AddPerf( TXT("AgentsBudgetingStrategy") );
			elem.AddPerf( TXT("AgentsWorldTick") );
			elem.AddPerf( TXT("TickBehTreeMachines") );
			elem.AddPerf( TXT("NpcCameraCollision") );
			elem.AddPerf( TXT("InteractionsMgr") );
			elem.AddPerf( TXT("ProcessInput") );
			elem.AddPerf( TXT("ProcessScheduledPartitionChanges") );
			elem.AddPerf( TXT("UpdateInputManager") );
			elem.AddPerf( TXT("UpdateTimeManager") );
			elem.AddPerf( TXT("ProcessCameraMovement") );
			elem.AddPerf( TXT("UpdateLoadingState") );
			elem.AddPerf( TXT("TickManagerAdvanceTime") );
			elem.AddPerf( TXT("TickEffects") );
			elem.AddPerf( TXT("FireTickEvents") );
			elem.AddPerf( TXT("DelayedStuff") );
			elem.AddPerf( TXT("StorySceneSystemTick") );
			elem.AddPerf( TXT("ReactionsManageriTck") );
			elem.AddPerf( TXT("ItemManagerTick") );	
			elem.AddPerf( TXT("ProjectileTick") );
			elem.AddPerf( TXT("PlayerTick") );
			elem.AddPerf( TXT("ActorTick") );
			elem.AddPerf( TXT("TickNPC1") );
			elem.AddPerf( TXT("TickNPC2") );
			elem.AddPerf( TXT("ExtEventProcess") );
			elem.AddPerf( TXT("StaticCameraTick") );
			elem.AddPerf( TXT("WindowComponentTick") );
			elem.AddPerf( TXT("DaycycleGraphicsEntityTick") );
			elem.AddPerf( TXT("SkyTransformEntityTick") );
			elem.AddPerf( TXT("ArchimedesComponentTick") );
			elem.AddPerf( TXT("GameplayDeviceTick") );
			elem.AddPerf( TXT("SceneAreaComponentTick") );
			//
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Scripts ];
			elem.SetTimeBudget( 3.f );
			elem.AddPerf( TXT("AdvanceThreads") );
			elem.AddPerf( TXT("EntityCallAnimEvent") );
			elem.AddPerf( TXT("FireTimers") );
			//
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Animation ];
			elem.SetTimeBudget( 5.f );
			elem.AddPerf( TXT("SkeletalAnimCompUpdate") );
			elem.RemovePerf( TXT("ProcessOutputEvents") );
			elem.AddPerf( TXT("AnimationManager_Pre") );
			elem.AddPerf( TXT("AnimationManager_Post") );
			// animated component updates
			elem.AddPerf( TXT("UpdateAndSampleAnimationJobImmediate") );
			elem.AddPerf( TXT("UpdateAndSampleAnimationJobAsync") );
			elem.AddPerf( TXT("AnimatePrePhysicsRagdoll") );
			elem.AddPerf( TXT("UpdateAndSampleAnimationSyncPart") );
			elem.AddPerf( TXT("UpdatePoseConstraintsJobImmediate") );
			elem.AddPerf( TXT("PostPoseConstraintsSyncPart") );
			elem.AddPerf( TXT("RagdollToAnimation") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Movement ];
			elem.SetTimeBudget( 3.f );
			elem.AddPerf( TXT("PathPlanerTick") );
			elem.AddPerf( TXT("FormationsManagerTick") );
			elem.AddPerf( TXT("ActorsStorageColComp") );
			elem.AddPerf( TXT("MovingPhysicalAgentComp_Post") );
			elem.AddPerf( TXT("MAC_UpdateSyncMovement") );
			elem.AddPerf( TXT("MAC_UpdateParallelMovement") );
			elem.AddPerf( TXT("FinalizeMovement_PreSeparation") );
			elem.AddPerf( TXT("FinalizeMovement_PostSeparation") );
			//
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Physics ];
			elem.SetTimeBudget( 2.5f );
			elem.AddPerf( TXT("HavokSimulate") );
			elem.AddPerf( TXT("RagdollMeshComponentTick") );
			elem.AddPerf( TXT("PhantomComponentTick") );
			elem.AddPerf( TXT("FXSphericalForcePreTick") );
			elem.AddPerf( TXT("FXCylindricalForcePreTick") );
			elem.AddPerf( TXT("AnimCompPrePhysicsRagdoll") );
			elem.AddPerf( TXT("DestrSysCompPostTick") );
			elem.AddPerf( TXT("PhysicsSystemComp") );
			elem.AddPerf( TXT("ACUpdateAndSampleSkeletonRagdoll") );
			elem.AddPerf( TXT("ACUpdateAndSampleSkeletonRagdoll2") );
			elem.AddPerf( TXT("FXCylindricalForcePhantomTick") );
			elem.AddPerf( TXT("FXSphericalForcePhantomTick") );
			
			//
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_UpdateTransform ];
			elem.SetTimeBudget( 7.f );
			elem.AddPerf( TXT("UpdateTransforms_Pre") );
			elem.AddPerf( TXT("UpdateTransforms_Post") );
			elem.AddPerf( TXT("UpdateTransforms_PostUpdate") );
			//
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Sounds ];
			elem.SetTimeBudget( 2.5f );
			elem.AddPerf( TXT("SoundTick") );
			elem.AddPerf( TXT("UpdateSoundListener") );
			elem.AddPerf( TXT("SoundUpdateActorCache") );
			//
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Renderer ];
			elem.SetTimeBudget( 3.f );
			elem.AddPerf( TXT("RenderTick") );
			elem.AddPerf( TXT("GenerateFrame") );
			elem.AddPerf( TXT("WorldRender") );
			elem.AddPerf( TXT("EnvironmentTick") );
			elem.AddPerf( TXT("ParticlePostUpdate") );
			elem.AddPerf( TXT("GameplayFXCatEffect") );
			//
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Hud ];
			elem.SetTimeBudget( 1.f );
			elem.AddPerf( TXT("UpdateHUDTime") );
			elem.AddPerf( TXT("TickHUDAll") );
			elem.AddPerf( TXT("VideoPlayerTick") );
			//
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_RenderFence ];
			elem.SetTimeBudget( 0.5f );
			elem.AddPerf( TXT("RenderFence") );
			//
		}
	};

	~CDebugPageGameFrameBudget()
	{
		delete m_tree;
		m_tree = NULL;
	}

	virtual void OnTick( Float timeDelta )
	{
		m_timeForSearch -= timeDelta;

		if ( m_timeForSearch < 0.f )
		{
			CollectPerfs();
			m_timeForSearch = 10.f;
		}

		m_allBudget.Update();

		Double totalTime = Max( m_allBudget.GetAverageTime(), 0.0001 );
		Double accTime = 0.0;

		const Uint32 size = m_elems.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			m_elems[ i ].UpdateAndCalc( totalTime, accTime );
		}

		m_restTime = totalTime - accTime;
		m_restPercent = m_restTime / totalTime;

		if ( m_tree )
		{
			m_tree->OnTick( timeDelta );
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

		const Uint32 width = 600;
		const Uint32 height = 150;
		const Uint32 y = 100;
		const Uint32 x = 100;

		static Color bgColor( 0, 0, 0 );

		// BG
		Int32 barWidth = width - 2;
		frame->AddDebugRect( x + 1, y + 1, barWidth, height - 1, bgColor );

		Int32 currX = x + 1;

		const Uint32 size = m_elems.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			m_elems[ i ].Draw( frame, width - 2, height, currX, y, GetColor( i ) );
		}

		{
			// Rest
			Int32 barWidth = Clamp< Int32 >( (Int32)( m_restPercent* ( width - 2 ) ), 0, width - 2 );
			frame->AddDebugRect( currX, y,  barWidth, height, Color( 255, 0, 0 ) );
		}

		// Legend
		Int32 legendY = y;
		Int32 legendLine = 15;
		Int32 legendX = x + width + 50;

		frame->AddDebugRect( legendX, legendY, 300, legendLine * ( size + 3 ), bgColor );
		legendX += 10;

		for ( Uint32 i=0; i<size; ++i )
		{
			legendY += 15;

			Color color = GetColor( i );
			String text = GetName( i );

			Float time = (Float)(m_elems[ i ].GetAverageTime());
			Float percent = (Float)(m_elems[ i ].GetPercent());
			Float timeBudget = m_elems[ i ].GetTimeBudget();

			String fullText = !m_showBudgets ? 
				String::Printf( TXT("%s | %1.3f ms | %1.1f"), text.AsChar(), time, 100.0 * percent ) :
				String::Printf( TXT("%s | %1.3f ms | %1.1f | %1.1f | %1.1f"), text.AsChar(), time, 100.0 * percent, timeBudget, timeBudget - time );

			frame->AddDebugScreenText( legendX, legendY, fullText.AsChar(), color );
		}

		{
			legendY += 15;

			String fullText;

			if ( m_restTime >= 0.0 )
			{
				fullText = String::Printf( TXT("Unknown | %1.3f ms | %1.1f"), m_restTime, 100.0 * m_restPercent );
			}
			else
			{
				fullText = String::Printf( TXT("Unknown | %1.3f ms | %1.1f ??? :)"), m_restTime, 100.0 * m_restPercent );
			}

			frame->AddDebugScreenText( legendX, legendY, fullText.AsChar(), Color( 255, 0, 0 ) );
		}

		{
			legendY += 15;
			frame->AddDebugScreenText( legendX, legendY, TXT("(Press T/DigitRight for details)"), Color( 128, 128, 128 ) );
		}

		// Create tree
		if ( !m_tree )
		{
			const Uint32 width = frame->GetFrameOverlayInfo().m_width - 100;
			const Uint32 height = frame->GetFrameOverlayInfo().m_height - legendY - 80;

			legendY += 15;

			m_tree = new CDebugOptionsTree( 50, legendY, width, height, this );

			for ( Uint32 i=0; i<m_elems.Size(); ++i )
			{
				IDebugCheckBox* group = new IDebugCheckBox( NULL, GetName( i ), true, false );

				{
					const TDynArray< SFrameBudgetElem::SPerfCounterStatArray >& arrayAdd = m_elems[ i ].GetCountersAdd();

					for ( Uint32 j=0; j<arrayAdd.Size(); ++j )
					{
						new CDebugFrameBudgetElemBox( group, arrayAdd[ j ] );
					}
				}

				m_tree->AddRoot( group );
			}
		}

		// Render tree
		if ( m_showTree )
		{
			m_tree->OnRender( frame );
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_GameFrame );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		if ( m_tree && m_tree->OnInput( key, action, data ) )
		{
			return true;
		}
		else if ( ( key == IK_T && action == IACT_Press ) || ( key == IK_Pad_DigitRight && action == IACT_Press ) )
		{
			m_showTree = !m_showTree;
		}
		else if ( ( key == IK_Y && action == IACT_Press ) || ( key == IK_Pad_DigitLeft && action == IACT_Press ) )
		{
			m_showBudgets = !m_showBudgets;
		}

		// Not processed
		return false;
	}

private:
	Color GetColor( Uint32 i )
	{
		switch( i )
		{
		case FBE_Gameplay:
			return Color( 200, 200, 200 );
		case FBE_Scripts:
			return Color( 200, 64, 200 );
		case FBE_Animation:
			return Color( 128, 255, 128 );
		case FBE_Movement:
			return Color( 255, 255, 128 );
		case FBE_Physics:
			return Color( 128, 255, 255 );
		case FBE_UpdateTransform:
			return Color( 255, 128, 255 );	
		case FBE_Sounds:
			return Color( 0, 128, 255 );
		case FBE_Renderer:
			return Color( 180, 100, 120 );
		case FBE_Hud:
			return Color( 128, 128, 255 );
		case FBE_RenderFence:
			return Color( 100, 100, 100 );
		default:
			return Color( 255, 128, 128 );
		};
	}

	String GetName( Uint32 i )
	{
		switch( i )
		{
		case FBE_Gameplay:
			return TXT("Gameplay");
		case FBE_Scripts:
			return TXT("Scripts");
		case FBE_Animation:
			return TXT("Animation");
		case FBE_Movement:
			return TXT("Movement");
		case FBE_Physics:
			return TXT("Physics");
		case FBE_UpdateTransform:
			return TXT("UpdateTransform");
		case FBE_Sounds:
			return TXT("Sound");
		case FBE_Renderer:
			return TXT("Renderer");
		case FBE_Hud:
			return TXT("Hud");
		case FBE_RenderFence:
			return TXT("RenderFence");
		default:
			return TXT("Error");
		};
	}

	void CollectPerfs()
	{
		m_allBudget.CollectPerfs();

		const Uint32 size = m_elems.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			m_elems[ i ].CollectPerfs();
		}
	}
};

void CreateDebugPageGameFrameBudget()
{
	IDebugPage* page = new CDebugPageGameFrameBudget();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif