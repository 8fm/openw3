/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiModalWindow.h"

namespace RedGui
{
	class CRedGuiMessageBox : public CRedGuiModalWindow
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiMessageBox();
		virtual ~CRedGuiMessageBox();

		void Show(const String& text, const String& title, EMessabeBoxIcon icon = MESSAGEBOX_Info);
		void Show(const String& text, EMessabeBoxIcon icon = MESSAGEBOX_Info);

	private:
		void NotifyEventButtonClicked( CRedGuiEventPackage& eventPackage );
		void NotifyEventKeyButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, enum EInputKey key, Char text);

		void SetIcon(EMessabeBoxIcon icon);

		virtual void OnPendingDestruction() override final;

		CRedGuiButton*	m_ok;		//!<
		CRedGuiLabel*	m_text;		//!<
		CRedGuiImage*	m_image;	//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
