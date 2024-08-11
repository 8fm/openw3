/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiSeparator.h"

namespace RedGui
{
	CRedGuiSeparator::CRedGuiSeparator( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
		: CRedGuiControl(x, y, width, height)
		, m_verticalOrientation(false)
		, m_preferSize((Float)width, (Float)height)
	{
		UpdateView();
	}

	CRedGuiSeparator::~CRedGuiSeparator()
	{
		/*intentionally empty*/
	}

	void CRedGuiSeparator::SetVerticalOrientation( Bool value )
	{
		m_verticalOrientation = value;
		UpdateView();
	}

	Bool CRedGuiSeparator::GetVerticalOrientation() const
	{
		return m_verticalOrientation;
	}

	void CRedGuiSeparator::Draw()
	{
		GetTheme()->DrawPanel(this);
	}

	void CRedGuiSeparator::UpdateView()
	{
		if(m_verticalOrientation == true)
		{
			CRedGuiControl::SetSize(1, (Int32)m_preferSize.Y);
		}
		else
		{
			CRedGuiControl::SetSize((Int32)m_preferSize.X, 1);
		}
	}

	void CRedGuiSeparator::SetSize( Int32 width, Int32 height )
	{
		m_preferSize = Vector2((Float)width, (Float)height);
		UpdateView();
	}

	void CRedGuiSeparator::SetSize( const Vector2& size )
	{
		m_preferSize = size;
		UpdateView();
	}

	void CRedGuiSeparator::SetCoord( Int32 left, Int32 top, Int32 width, Int32 height )
	{
		CRedGuiControl::SetCoord(left, top, width, height);
		m_preferSize = Vector2((Float)width, (Float)height);
		UpdateView();
	}

	void CRedGuiSeparator::SetCoord( const Box2& coord )
	{
		CRedGuiControl::SetCoord(coord);
		m_preferSize = coord.Max;
		UpdateView();
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
