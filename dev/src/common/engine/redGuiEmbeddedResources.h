/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

#define REDGUI_PATH String( TXT("engine\\icons\\redgui\\") )

namespace RedGui
{
	namespace Resources
	{
		// cursors
		extern CGatheredResource GArrowCursor;
		extern CGatheredResource GHandCursor;
		extern CGatheredResource GWaitCursor;
		extern CGatheredResource GTextCursor;
		extern CGatheredResource GMoveCursor;
		extern CGatheredResource GHorizontalResizeCursor;
		extern CGatheredResource GVerticalResizeCursor;
		extern CGatheredResource GSlashResizeCursor;
		extern CGatheredResource GBackslashResizeCursor;

		// icons
		extern CGatheredResource GTestIcon;
		extern CGatheredResource GCheckIcon;
		extern CGatheredResource GHourglassIcon;

		extern CGatheredResource GExitIcon;
		extern CGatheredResource GMaximizeIcon;
		extern CGatheredResource GMinimizeIcon;
		extern CGatheredResource GHelpIcon;

		extern CGatheredResource GCollapseIcon;
		extern CGatheredResource GExpandIcon;

		extern CGatheredResource GMinusIcon;
		extern CGatheredResource GPlusIcon;

		extern CGatheredResource GInformationIcon;
		extern CGatheredResource GWarningIcon;
		extern CGatheredResource GErrorIcon;

		extern CGatheredResource GStopIcon;
		extern CGatheredResource GPlayIcon;

		// arrows
		extern CGatheredResource GUpArrowIcon;
		extern CGatheredResource GDownArrowIcon;
		extern CGatheredResource GRightArrowIcon;
		extern CGatheredResource GLeftArrowIcon;

		// review system
		extern CGatheredResource GReviewEyeIcon;
		extern CGatheredResource GReviewAddIcon;
		extern CGatheredResource GReviewPropertiesIcon;
		extern CGatheredResource GReviewRefreshIcon;
		extern CGatheredResource GReviewFiltersIcon;
		extern CGatheredResource GReviewResetFiltersIcon;
		extern CGatheredResource GReviewMoveIcon;
		extern CGatheredResource GReviewFreeCam;

		// backgrounds
		extern CGatheredResource GWindowBackground;

		// fonts
		extern CGatheredResource GParachuteFont;

	}	// namespace Resources
}	// namespace RedGui

#endif	// NO_RED_GUI
