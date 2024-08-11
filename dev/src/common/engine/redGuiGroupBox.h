/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiGroupBox : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiGroupBox(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiGroupBox();

		String GetText() const;
		void SetText(const String& text);

		void Collapse();
		void Expand();

		void Draw();

		void SetBackgroundColor(const Color& color);

	protected:
		void NotifyEventCheckedChanged( CRedGuiEventPackage& eventPackage, Bool value );
		
		virtual void OnPendingDestruction() override final;

	private:
		CRedGuiButton*	m_expandButton;
		CRedGuiButton*	m_titleButton;
		CRedGuiPanel*	m_captionPanel;
		CRedGuiPanel*	m_contentPanel;

		Box2			m_minPreviousCoord;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
