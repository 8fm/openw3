/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiAreaChart.h"

namespace RedGui
{
	CRedGuiAreaChart::CRedGuiAreaChart( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl( left, top, width, height )
		, m_endIndex( 0 )
		, m_count( 0 )
		, m_maxValue( 1 )
		, m_peak( 0 )
		, m_trough( 0xffffffff )
	{
		/* intentionally empty */
	}

	CRedGuiAreaChart::~CRedGuiAreaChart()
	{
		/* intentionally empty */
	}

	void CRedGuiAreaChart::Update( Int64 value, Int64 max )
	{
		m_values[ m_endIndex ] = value;
		++m_endIndex;
		++m_count;
		if( m_endIndex >= c_maxValueHistory )
		{
			m_endIndex = 0;
		}
		if( m_count >= c_maxValueHistory )
		{
			m_count = c_maxValueHistory;
		}
		m_maxValue = max;
		m_peak = Red::Math::NumericalUtils::Max( m_peak, value );
		m_trough = Red::Math::NumericalUtils::Min( m_trough, value );
	}

	void CRedGuiAreaChart::Reset()
	{
		m_endIndex = 0;
		m_count = 0;
		m_maxValue = 1;
		m_peak = 0;
		m_trough = 0xffffffff;
	}

	void CRedGuiAreaChart::Draw()
	{
		GetTheme()->DrawPanel( this );

		GetTheme()->SetCroppedParent( this );

		Color barColour( 255, 255, 0, 128 );
		Color peakColour = Color::LIGHT_RED;
		Color troughColour = Color::LIGHT_GREEN;

		Float barWidthPx = Red::Math::MCeil( (Float)GetWidth() / (Float)c_maxValueHistory );
		Float barHeightMultiplier = (Float)GetHeight() / (Float)m_maxValue;
		Float barX = GetAbsoluteLeft() + GetWidth() - barWidthPx;
		Int32 barIndex = m_endIndex - 1;

		for( Int32 i=0; i < m_count; ++i )
		{
			if( barIndex < 0 )
			{
				barIndex = c_maxValueHistory - 1; 
			}
			if( barX < GetAbsoluteLeft() )
			{
				break;
			}

			Float heightPx =  barHeightMultiplier * (Float)m_values[barIndex];
			GetTheme()->DrawRawFilledRectangle( Vector2( barX, (Float)( GetAbsoluteTop() + GetHeight() ) - (Int32)heightPx ), Vector2( barWidthPx, heightPx ), barColour );
			barX -= barWidthPx;
			--barIndex;
		}
		
		// draw statistic line
		GetTheme()->DrawRawFilledRectangle( Vector2( (Float)GetAbsoluteLeft(), (Float)( (Float)( GetAbsoluteTop() + GetHeight() ) - ( (Float)m_peak * barHeightMultiplier ) ) ), Vector2( (Float)GetWidth(), 1), peakColour );
		GetTheme()->DrawRawFilledRectangle( Vector2( (Float)GetAbsoluteLeft(), (Float)( (Float)( GetAbsoluteTop() + GetHeight() ) - ( (Float)m_trough * barHeightMultiplier ) ) ), Vector2( (Float)GetWidth(), 1), troughColour );

		GetTheme()->DrawRawText( Vector2( (Float)( GetAbsoluteLeft() ), (Float)( (Float)( GetAbsoluteTop() + GetHeight() ) - ( (Float)m_peak * barHeightMultiplier ) ) ), TXT("Peak"), peakColour );
		GetTheme()->DrawRawText( Vector2( (Float)( GetAbsoluteLeft() + GetWidth() - 35 ), (Float)( (Float)( GetAbsoluteTop() + GetHeight() ) - ( (Float)m_trough * barHeightMultiplier ) ) ), TXT("Trough"), troughColour );

		GetTheme()->ResetCroppedParent();
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
