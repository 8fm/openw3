/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiAreaChart : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiAreaChart( Uint32 left, Uint32 top, Uint32 width, Uint32 height );
		virtual ~CRedGuiAreaChart();

		void Update( Int64 value, Int64 max );
		void Reset();

		void Draw();

	private:
		static const Int32 c_maxValueHistory = 1024;
		Int64 m_values[ c_maxValueHistory ];
		Int64 m_maxValue;
		Int32 m_endIndex;
		Int32 m_count;
		Int64 m_peak;
		Int64 m_trough;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
