/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiModalWindow.h"

namespace RedGui
{
	class CRedGuiProgressBox : public CRedGuiModalWindow
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiProgressBox();
		virtual ~CRedGuiProgressBox();

		void Show(const String& text, Bool autoProgress = false);
		void Hide();

		void SetText(const String& text);
		void UpdateProgress( Float percent);

	private:
		CRedGuiLabel*		m_text;
		CRedGuiImage*		m_image;
		CRedGuiProgressBar*	m_progressBar;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
