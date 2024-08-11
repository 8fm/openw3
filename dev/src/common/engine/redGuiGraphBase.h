/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	// This is a class that can act as a base for making various graph controls (bar graphs, lines, histograms, etc, etc)
	// It handles the drawing of axes and a key
	class CRedGuiGraphBase : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiGraphBase( Uint32 left, Uint32 top, Uint32 width, Uint32 height );
		virtual ~CRedGuiGraphBase();
		virtual void Draw();

		void SetXAxisPadding( Uint32 pad );
		void SetYAxisPadding( Uint32 pad );

		void SetXAxisMaximum( Float maximum );
		void SetYAxisMaximum( Float maximum );

		void AddXAxisLabel( const String& text, Float axisValue );
		void AddYAxisLabel( const String& text, Float axisValue );

		void AddKey( const String& key, Color colour );
		void SetKeyWidth( Uint32 width );

	protected:

		virtual void DrawGraph( const Vector2& origin, const Vector2& dimensions ) = 0;
		void DrawAxes( const Vector2& origin, const Vector2& dimensions );
		void DrawKeys( const Vector2& origin, const Vector2& dimensions );
		Vector2 CalculateGraphScreenOffset( Vector2 inputData, const Vector2& graphScreenDimensions ) const;		// Uses axis limits to scale graph input data to screen offset

		// Keys
		struct SGraphKey
		{
			Color m_colour;
			String m_keyText;
		};
		TDynArray< SGraphKey > m_graphKeys;
		Uint32 m_keyPanelWidth;

		// Labels can be added to either axis
		struct SAxisLabel
		{
			String m_labelText;
			Float m_value;
		};
		TDynArray< SAxisLabel > m_xAxisLabels;
		TDynArray< SAxisLabel > m_yAxisLabels;

		// Max values used to calculate label positions
		Float m_xAxisMaxValue;
		Float m_yAxisMaxValue;

		// Width / height of 'free space' to draw labels
		Uint32 m_xAxisPadding;
		Uint32 m_yAxisPadding;

		// Colour of axis data
		Color m_axisLineColour;
		Color m_axisLabelColour;
	};
}

#endif	// NO_RED_GUI