/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redGuiInputManager.h"
#include "redGuiModalWindow.h"
#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#endif	// NO_DEBUG_WINDOWS

namespace RedGui
{
	CRedGuiModalWindow::CRedGuiModalWindow( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiWindow(left, top, width, height)
	{
		/* intentionally empty */
	}

	CRedGuiModalWindow::~CRedGuiModalWindow()
	{
		/* intentionally empty */
	}

	void CRedGuiModalWindow::SetVisible( Bool value )
	{
		if( value == true )
		{
#ifndef NO_DEBUG_WINDOWS
			GDebugWin::GetInstance().LockHiding();
#endif	// NO_DEBUG_WINDOWS
			GRedGui::GetInstance().GetInputManager()->SetActiveModalWindow( this );
			MoveToTop();
		}
		else
		{
			GRedGui::GetInstance().GetInputManager()->ResetModalWindow( this );
#ifndef NO_DEBUG_WINDOWS
			GDebugWin::GetInstance().UnlockHiding();
#endif	// NO_DEBUG_WINDOWS
		}

		CRedGuiWindow::SetVisible( value );
	}

}	// namespace RedGui


#endif	// NO_RED_GUI
