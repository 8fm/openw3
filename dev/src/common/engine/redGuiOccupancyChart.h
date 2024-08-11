/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiOccupancyChart : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
		struct SRedGuiOccupancySeries
		{
			String m_name;
			Float m_percent;
			Color m_color;
		};

	public:
		CRedGuiOccupancyChart(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiOccupancyChart();

		void SetLegendVisible(Bool value);
		Bool GetLegendVisible() const;

		void SetCheckCorrectPercent(Bool value);
		Bool GetCheckCorrectPercent() const;

		void AddData(const String& name, const Color& color, Float percent);
		Bool UpdateData(const String& name, Float percent);	// percent must be from 0.0f to 1.0f. Returns true if data was added
		void ClearData();

		void Draw();

	private:
		void CheckPercentCorrectness();

		TDynArray<SRedGuiOccupancySeries, MC_RedGuiControls, MemoryPool_RedGui> m_data;

		CRedGuiButton*	m_errorInfo;
		CRedGuiPanel*	m_chartPanel;
		CRedGuiPanel*	m_legendPanel;
		CRedGuiList*	m_legendList;

		Bool			m_correctPercent;
		Bool			m_checkCorrentPercent;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
