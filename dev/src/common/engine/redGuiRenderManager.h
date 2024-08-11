/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class CRedGuiRenderManager
	{
	public:
		CRedGuiRenderManager();
		virtual ~CRedGuiRenderManager();

		// Get actually viewport size
		Vector2 GetViewSize() const;
		void SetViewSize( const Vector2& value );

		// Get graphic context
		CRedGuiGraphicContext* GetGraphicContext() const;

	private:
		CRedGuiGraphicContext*	m_graphicContext;
		Vector2					m_currentViewSize;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
