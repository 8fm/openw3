/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiToolTipManager
	{
	public:
		CRedGuiToolTipManager( CRedGuiManager* redGuiManager );
		virtual ~CRedGuiToolTipManager();

		Float GetDelayVisible() const;
		void SetDelayVisible(Float val);

		void HideToolTip(CRedGuiControl* control);
		void ShowToolTip(CRedGuiControl* control, const Vector2& position);

	protected:
		void NotifyEventFrameTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime);
		void NotifyEventChangeMouseFocus( RedGui::CRedGuiEventPackage& eventPackage );

	private:
		CRedGuiControl* m_oldFocusControl;		//!<
		CRedGuiControl* m_currentFocusControl;	//!<
		Float			m_delayVisible;			//!<
		Bool			m_toolTipVisible;		//!<
		Float			m_currentTime;			//!<
		Bool			m_needToolTip;			//!<
		Vector2			m_oldMousePosition;		//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
