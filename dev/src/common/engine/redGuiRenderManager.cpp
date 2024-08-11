/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiGraphicContext.h"
#include "redGuiRenderManager.h"

namespace RedGui
{
	CRedGuiRenderManager::CRedGuiRenderManager()
		: m_graphicContext(nullptr)
		, m_currentViewSize( 0.0f, 0.0f )
	{
		m_graphicContext = new CRedGuiGraphicContext();
	}

	CRedGuiRenderManager::~CRedGuiRenderManager()
	{
		if(m_graphicContext != nullptr)
		{
			delete m_graphicContext;
			m_graphicContext = nullptr;
		}
	}

	Vector2 CRedGuiRenderManager::GetViewSize() const
	{
		return m_currentViewSize;
	}

	void CRedGuiRenderManager::SetViewSize( const Vector2& value )
	{
		m_currentViewSize = value;
	}

	CRedGuiGraphicContext* CRedGuiRenderManager::GetGraphicContext() const
	{
		return m_graphicContext;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
