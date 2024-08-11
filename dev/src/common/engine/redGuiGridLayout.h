/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiGridLayout : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiGridLayout(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
		virtual ~CRedGuiGridLayout();

		void SetDimensions( Int32 columnCount, Int32 rowCount );
		Vector2 GetDimensions() const;

	public:
		virtual void UpdateControl();
		virtual void Draw();

	private:
		Vector2 m_dimensions;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
