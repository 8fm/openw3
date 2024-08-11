/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "redGuiGraphBase.h"

#ifndef NO_RED_GUI 

namespace RedGui
{
	CRedGuiGraphBase::CRedGuiGraphBase( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl( left, top, width, height )
		, m_xAxisMaxValue( 100.0f )
		, m_yAxisMaxValue( 100.0f )
		, m_xAxisPadding( 10 )
		, m_yAxisPadding( 10 )
		, m_axisLineColour( Color::WHITE )
		, m_axisLabelColour( Color::WHITE )
		, m_keyPanelWidth( 0 )
	{
	}

	CRedGuiGraphBase::~CRedGuiGraphBase()
	{

	}

	void CRedGuiGraphBase::SetXAxisMaximum( Float maximum )
	{
		m_xAxisMaxValue = maximum;
	}

	void CRedGuiGraphBase::SetYAxisMaximum( Float maximum )
	{
		m_yAxisMaxValue = maximum;
	}

	void CRedGuiGraphBase::AddXAxisLabel( const String& text, Float axisValue )
	{
		SAxisLabel newLabel;
		newLabel.m_labelText = text;
		newLabel.m_value = axisValue;
		m_xAxisLabels.PushBack( newLabel );
	}

	void CRedGuiGraphBase::AddYAxisLabel( const String& text, Float axisValue )
	{
		SAxisLabel newLabel;
		newLabel.m_labelText = text;
		newLabel.m_value = axisValue;
		m_yAxisLabels.PushBack( newLabel );
	}

	void CRedGuiGraphBase::SetXAxisPadding( Uint32 pad )
	{
		m_xAxisPadding = pad;
	}

	void CRedGuiGraphBase::SetYAxisPadding( Uint32 pad )
	{
		m_yAxisPadding = pad;
	}

	void CRedGuiGraphBase::AddKey( const String& key, Color colour )
	{
		SGraphKey newKey;
		newKey.m_keyText = key;
		newKey.m_colour = colour;
		m_graphKeys.PushBack( newKey );
	}

	void CRedGuiGraphBase::SetKeyWidth( Uint32 width )
	{
		m_keyPanelWidth = width;
	}

	void CRedGuiGraphBase::DrawAxes( const Vector2& origin, const Vector2& dimensions )
	{
		const Float s_textCharacterWidth = 7.0f;
		const Float s_textCharacterHeight = 12.0f;
		const Float s_textXAxisOffset = 2.0f;
		const Float s_textYAxisOffset = 2.0f;

		Vector2 graphOrigin = origin + Vector2( (Float)m_yAxisPadding, -(Float)m_xAxisPadding );
		Vector2 graphDimensions( dimensions.X - (Float)m_yAxisPadding, dimensions.Y - (Float)m_xAxisPadding );

		// X Axis line
		GetTheme()->DrawRawLine( graphOrigin, graphOrigin + Vector2( graphDimensions.X, 0.0f ), m_axisLineColour );

		// Y Axis line
		GetTheme()->DrawRawLine( graphOrigin, graphOrigin + Vector2( 0.0f, -graphDimensions.Y ), m_axisLineColour );

		// X Axis Labels
		Float xAxisScaling = ( dimensions.X - m_yAxisPadding ) / m_xAxisMaxValue;
		for( auto axisLabel = m_xAxisLabels.Begin(); axisLabel != m_xAxisLabels.End(); ++ axisLabel )
		{
			Float valueXOffset = (*axisLabel).m_value * xAxisScaling;
			Float labelXOffset = valueXOffset - ( ( (*axisLabel).m_labelText.GetLength() * s_textCharacterWidth ) * 0.5f );

			GetTheme()->DrawRawText( graphOrigin + Vector2( labelXOffset, s_textXAxisOffset ), (*axisLabel).m_labelText, m_axisLabelColour );
			GetTheme()->DrawRawLine( graphOrigin + Vector2( valueXOffset, 0.0f ), graphOrigin + Vector2( valueXOffset, 2.0f ), m_axisLabelColour );
		}

		// Y Axis Labels
		Float yAxisScaling = ( dimensions.Y - m_xAxisPadding ) / m_yAxisMaxValue;
		for( auto axisLabel = m_yAxisLabels.Begin(); axisLabel != m_yAxisLabels.End(); ++ axisLabel )
		{
			Float valueYOffset = (*axisLabel).m_value * yAxisScaling;
			Float labelXOffset = -( s_textCharacterWidth * (*axisLabel).m_labelText.GetLength() ) - s_textYAxisOffset;
			Float labelYOffset = valueYOffset + ( s_textCharacterHeight * 0.5f );

			GetTheme()->DrawRawText( graphOrigin + Vector2( labelXOffset, -labelYOffset ), (*axisLabel).m_labelText, m_axisLabelColour );
			GetTheme()->DrawRawLine( graphOrigin + Vector2( -2.0f, -valueYOffset ), graphOrigin + Vector2( 0.0f, -valueYOffset ), m_axisLabelColour );
		}
	}

	void CRedGuiGraphBase::DrawKeys( const Vector2& origin, const Vector2& dimensions )
	{
		const Float k_keyBoxSize = 14.0f;
		const Float k_keyTextOffset = 4.0f;
		Float keyTextOffsetY = 2.0f;
		for( auto it : m_graphKeys )
		{
			Vector2 keyBoxOrigin( 10.0f, keyTextOffsetY );
			GetTheme()->DrawRawFilledRectangle( origin + keyBoxOrigin, Vector2( k_keyBoxSize, k_keyBoxSize ), it.m_colour );
			GetTheme()->DrawRawText( origin + keyBoxOrigin + Vector2( k_keyBoxSize + k_keyTextOffset, 2.0f ), it.m_keyText, Color::WHITE );
			keyTextOffsetY += k_keyBoxSize + 2.0f;
		}
	}

	void CRedGuiGraphBase::Draw()
	{
		GetTheme()->DrawPanel( this );
		GetTheme()->SetCroppedParent( this );

		Vector2 origin = GetAbsolutePosition() + Vector2( 0.0f, (Float)GetHeight() );
		Vector2 dimensions( (Float)GetWidth() - m_keyPanelWidth, (Float)GetHeight() );
		DrawAxes( origin, dimensions );

		Vector2 keyOrigin( origin.X + dimensions.X, GetAbsolutePosition().Y );
		Vector2 keyDimensions( (Float)m_keyPanelWidth, (Float)GetHeight() );
		DrawKeys( keyOrigin, keyDimensions );

		Vector2 graphOrigin = GetAbsolutePosition() + Vector2( (Float)m_yAxisPadding, -(Float)m_xAxisPadding );
		Vector2 graphDimensions( (Float)GetWidth() - m_keyPanelWidth - m_yAxisPadding, (Float)GetHeight() - m_xAxisPadding );
		DrawGraph( graphOrigin, graphDimensions );

		GetTheme()->ResetCroppedParent();
	}
	
	Vector2 CRedGuiGraphBase::CalculateGraphScreenOffset( Vector2 inputData, const Vector2& graphScreenDimensions ) const
	{
		return Vector2( ( inputData.X / m_xAxisMaxValue ) * graphScreenDimensions.X, ( inputData.Y / m_yAxisMaxValue ) * graphScreenDimensions.Y );
	}
}

#endif