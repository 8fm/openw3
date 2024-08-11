#pragma once

class CRenderFrame;

#ifndef NO_DEBUG_PAGES

#include "debugPage.h"
#include "renderFrame.h"

// Base stats
class BaseStatChart
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );
public:
	virtual ~BaseStatChart() {};

	//! Reset stats
	virtual void Reset()=0;

	//! Update
	virtual void Update( CWorld* world )=0;

	//! Draw
	virtual void DrawBar( CRenderFrame* frame, Uint32 baseX, Uint32 &baseY, Uint32 barWidth )=0;

public:
	//! Convert ticks to time
	static Float TicksToTime( Uint64 ticks )
	{
		// Get tick frequency
		static Uint64 frequency = 0;
		if ( !frequency )
		{
			Red::System::Clock::GetInstance().GetTimer().GetFrequency( frequency );
		}

		// Convert time
		return ( Float )( (Double)ticks / (Double)frequency );
	}
};

// Crap stats 
template< Uint32 HistorySize >
class StatChart : public BaseStatChart
{
protected:
	String			m_title;
	Float			m_currentTime;
	Uint32			m_currentCount;
	Float			m_history[ HistorySize ];
	Uint32			m_historySize;
	Float			m_historyAverage;
	Float			m_historyMin;
	Float			m_historyMax;
	Float			m_limit;

public:
	StatChart( const Char* name, Float limit )
		: m_title( name )
		, m_currentTime( 0.0f )
		, m_currentCount( 0 )
		, m_historyMin( 0.0f )
		, m_historyMax( 0.0f )
		, m_historySize( 0 )
		, m_limit( limit )
	{
		Reset();
	}

	//! Update value
	using BaseStatChart::Update;
	void Update( Float currentValue, Uint32 currentCount )
	{
		// Update value
		m_currentTime = currentValue;
		m_currentCount = currentCount;

		// Update history
		if ( m_historySize < HistorySize )
		{
			// Add to history
			m_history[ m_historySize ] = currentValue;
			++m_historySize;
		}
		else
		{
			// TODO: this can be optimized
			for ( Uint32 i=1; i<HistorySize; ++i )
			{
				m_history[i-1] = m_history[i];
			}

			m_history[ HistorySize-1 ] = currentValue;
		}

		// Calculate average value based on history
		Float min = m_history[0];
		Float max = m_history[0];
		Float avg = 0.0f;
		for ( Uint32 i=0; i<m_historySize; ++i )
		{
			avg += m_history[i];
			min = Min< Float >( min, m_history[i] );
			max = Max< Float >( max, m_history[i] );
		}

		// Remember
		m_historyAverage = avg / ( Float ) m_historySize;
		m_historyMax = max;
		m_historyMin = min;
	}

	//! Reset
	virtual void Reset()
	{
		// Reset values
		m_historyMin = 0.0f;
		m_historyMax = 0.0;
		m_historySize = 0;
		m_historyAverage = 0.0f;
		m_currentTime = 0;
		m_currentCount = 0;

		// Cleanup history
		Red::System::MemoryZero( m_history, sizeof(m_history) );
	}

	//! Get bar height
	virtual Uint32 GetBarHeight() const
	{
		return 17;
	}

	//! Draw bar
	void DrawBar( CRenderFrame* frame, Uint32 baseX, Uint32 &baseY, Uint32 barWidth )
	{
		// Draw the background bar - local maximum
		const Uint32 barHeight = GetBarHeight();
		const Float maxPrc = m_historyMax / m_limit;
		IDebugPage::DrawProgressBar( frame, baseX, baseY, barWidth, barHeight, maxPrc, Color::GRAY );

		// Determine color
		Color barColor = Color( 0, 128, 0 );
		const Float prc = m_currentTime / m_limit;
		if ( prc > 0.90f ) barColor = Color( 128, 0, 0 );
		else if ( prc > 0.75f ) barColor = Color( 128, 128, 0 );
		else if ( prc > 0.5f ) barColor = Color( 64, 128, 0 );

		// Draw real value bar
		IDebugPage::DrawProgressBar( frame, baseX, baseY, barWidth, barHeight, prc, barColor );

		// Draw average value marker
		const Float peekPrc = m_historyAverage / m_limit;
		if ( peekPrc > 0.0f && peekPrc < 1.0f )
		{
			Int32 peekWidth = Clamp< Int32 >( (Int32)( peekPrc * ( 400 - 2 ) ), 0, 400 - 2 );
			frame->AddDebugFrame( baseX + peekWidth, baseY, 1, 17, Color::WHITE );
		}

		// Draw text
		frame->AddDebugScreenFormatedText( baseX+5, baseY+12, TXT("%s (%1.2fms)"), m_title.AsChar(), m_currentTime * 1000.0f );
		frame->AddDebugScreenFormatedText( baseX+barWidth+20, baseY+12, TXT("%i"), m_currentCount );
		frame->AddDebugScreenFormatedText( baseX+barWidth+80, baseY+12, TXT("%1.2fms"), m_historyAverage * 1000.0f );
		frame->AddDebugScreenFormatedText( baseX+barWidth+140, baseY+12, TXT("%1.2fms"), m_historyMax * 1000.0f );

		// Move down
		baseY += barHeight + 5;
	}

	//! Fit limit
	void FitLimit()
	{
		if ( m_historyMax >= m_limit )
		{
			m_limit = m_historyMax;
		}
		else
		{
			Float diff = m_limit - m_historyMax;

			if ( diff / m_limit > 0.1f )
			{
				m_limit = m_historyMax;
			}
		}

		m_limit = Max( m_limit, 0.01f );
	}
};

#endif // ! NO_DEBUG_PAGES