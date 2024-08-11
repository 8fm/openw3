/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class CRedGuiDock
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiDock( EDock value = DOCK_None )
			: m_value(value)
		{
			/* intentionally empty */
		}

		CRedGuiDock( const CRedGuiDock& dock )
			: m_value(dock.m_value)
		{
			/* intentionally empty */
		}

		RED_INLINE EDock GetValue() const
		{
			return m_value;
		}

		RED_INLINE Bool IsLeft() const
		{
			return DOCK_Left == m_value;
		}

		RED_INLINE Bool IsRight() const
		{
			return DOCK_Right == m_value;
		}

		RED_INLINE Bool IsTop() const
		{
			return DOCK_Top == m_value;
		}

		RED_INLINE Bool IsBottom() const
		{
			return DOCK_Bottom == m_value;
		}

		RED_INLINE Bool IsNone() const
		{
			return DOCK_None == m_value;
		}

		RED_INLINE Bool IsFill() const
		{
			return DOCK_Fill == m_value;
		}

		friend Bool operator== (CRedGuiDock const& a, CRedGuiDock const& b)
		{
			return a.m_value == b.m_value;
		}

		friend Bool operator!= (CRedGuiDock const& a, CRedGuiDock const& b)
		{
			return a.m_value != b.m_value;
		}

	private:
		EDock m_value;
	};
}	// namespace RedGui

#endif	// NO_RED_GUI
