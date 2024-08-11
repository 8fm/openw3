/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "renderFrame.h"

namespace DebugWindows
{
	#define BUDGET_ELEM_SIZE 10

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
		FBE_Engine,
		FBE_RenderFence,
		FBE_Editor,
		FBE_Last
	};

	struct SFrameBudgetElem
	{
		struct SPerfCounterStat
		{
			CPerfCounter*	m_counter;
			Uint64			m_lastTime;
			Uint64			m_lastIncr;

			SPerfCounterStat() : m_counter( nullptr ), m_lastTime( 0 ), m_lastIncr( 0 ) {}
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

			Float GetAverageTime( Float freq ) const
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
		Int32							m_curr;
		Float							m_freq;

		Float							m_avg;
		Float							m_percent;

		Float							m_timeBudget;

		Float CalcAverageTime() const
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
			m_freq = freq/1000.0f;
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

		void UpdateAndCalc( Float totalTime, Float& accTime )
		{
			Update();

			accTime += m_avg;

			m_percent = m_avg / totalTime;
		}

		Float GetAverageTime() const
		{
			return m_avg;
		}

		Float GetPercent() const
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

	class CDebugWindowGameFrame : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowGameFrame();
		~CDebugWindowGameFrame();;

	protected:
		void NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta);
		void NotifyEventNodeDoubleClicked( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiTreeNode* node); 

	private:
		void CollectCouters();

		void AddNodeCounter(CPerfCounter* rootCounter, RedGui::CRedGuiTreeNode* rootNode);

		void CollectPerfs();
		String GetName( Uint32 i );
		Color GetColor( Uint32 i );

	private:
		RedGui::CRedGuiTab*					m_modes;
		RedGui::CRedGuiList*				m_generalList;
		RedGui::CRedGuiTreeView*			m_functionsTree;
		RedGui::CRedGuiHistogram*			m_chart;

		TDynArray< SFrameBudgetElem >		m_elems;
		SFrameBudgetElem					m_allBudget;
		Float								m_restPercent;
		Float								m_restTime;
		Bool								m_showTree;
		Float								m_timeForSearch;
		Bool								m_showBudgetsValue;

	};
}

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
