/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiSeparator : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiSeparator(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
		virtual  ~CRedGuiSeparator();

		void SetSize(Int32 width, Int32 height);
		void SetSize( const Vector2& size);
		void SetCoord(Int32 left, Int32 top, Int32 width, Int32 height);
		void SetCoord(const Box2& coord);

		// Set vertical orientation
		void SetVerticalOrientation(Bool value);
		// Get vertical orientation
		Bool GetVerticalOrientation() const;

		void Draw();

	private:
		void UpdateView();

		Bool	m_verticalOrientation;	//!<
		Vector2 m_preferSize;			//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
