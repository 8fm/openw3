/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class CRedGuiAnchor
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
		CRedGuiControl* m_relatedControl;
		Int32			m_spacing;
	public:
		CRedGuiAnchor( EAnchor value = ANCHOR_Default )
			: m_value(value)
		{
			/* intentionally empty */
		}

		CRedGuiAnchor( const CRedGuiAnchor& align )
			: m_value(align.m_value)
		{
			/* intentionally empty */
		}

		RED_INLINE Bool IsLeft() const
		{
			return ANCHOR_Left == (m_value & ((Int32)ANCHOR_Left));
		}

		RED_INLINE Bool IsRight() const
		{
			return ANCHOR_Right == (m_value & ((Int32)ANCHOR_Right));
		}

		RED_INLINE Bool IsHStretch() const
		{
			return ANCHOR_HStretch == (m_value & ((Int32)ANCHOR_HStretch));
		}

		RED_INLINE Bool IsTop() const
		{
			return ANCHOR_Top == (m_value & ((Int32)ANCHOR_Top));
		}

		RED_INLINE Bool IsBottom() const
		{
			return ANCHOR_Bottom == (m_value & ((Int32)ANCHOR_Bottom));
		}

		RED_INLINE Bool IsVStretch() const
		{
			return ANCHOR_VStretch == (m_value & ((Int32)ANCHOR_VStretch));
		}

		RED_INLINE Bool IsStretch() const
		{
			return ANCHOR_Stretch == (m_value & ((Int32)ANCHOR_Stretch));
		}

		RED_INLINE Bool IsNone() const
		{
			return ANCHOR_None == m_value;
		}

		RED_INLINE Bool IsDefaul() const
		{
			return ANCHOR_Default == m_value;
		}

		CRedGuiAnchor& operator |= (CRedGuiAnchor const& option)
		{
			m_value = EAnchor( Int32(m_value) | Int32(option.m_value) );
			return (*this);
		}

		friend CRedGuiAnchor operator| (EAnchor const& a, EAnchor const& b)
		{
			return CRedGuiAnchor(EAnchor(int(a) | int(b)));
		}

		friend CRedGuiAnchor operator| (CRedGuiAnchor const& a, CRedGuiAnchor const& b)
		{
			return CRedGuiAnchor(EAnchor(int(a.m_value) | int(b.m_value)));
		}

		friend Bool operator== (CRedGuiAnchor const& a, CRedGuiAnchor const& b)
		{
			return a.m_value == b.m_value;
		}

		friend Bool operator!= (CRedGuiAnchor const& a, CRedGuiAnchor const& b)
		{
			return a.m_value != b.m_value;
		}

	private:
		EAnchor m_value;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
