/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiHistogram : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );

		static const Int32 kHistoryLength = 600;

		struct SRedGuiOccupancySeries
		{
			String m_name;
			Float m_percent[kHistoryLength];
			Color m_color;
		};

	public:
		CRedGuiHistogram(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiHistogram();

		void SetLegendVisible(Bool value);
		Bool GetLegendVisible() const;

		void SetCheckCorrectPercent(Bool value);
		Bool GetCheckCorrectPercent() const;

		void AddData(const String& name, const Color& color, Float percent);
		void UpdateDate(const String& name, Float percent);	// percent must be from 0.0f to 1.0f
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
