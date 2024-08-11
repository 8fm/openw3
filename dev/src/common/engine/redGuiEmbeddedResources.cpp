/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "redGuiEmbeddedResources.h"
#include "..\core\gatheredResource.h"

#ifndef NO_RED_GUI 

namespace RedGui
{
	namespace Resources
	{
		// cursors
		CGatheredResource GArrowCursor( String( REDGUI_PATH + TXT("cursors\\arrowcursor.xbm") ).AsChar(), 0 );
		CGatheredResource GHandCursor( String( REDGUI_PATH + TXT("cursors\\handcursor.xbm") ).AsChar(), 0 );
		CGatheredResource GWaitCursor( String( REDGUI_PATH + TXT("cursors\\waitcursor.xbm") ).AsChar(), 0 );
		CGatheredResource GTextCursor( String( REDGUI_PATH + TXT("cursors\\textcursor.xbm") ).AsChar(), 0 );
		CGatheredResource GMoveCursor( String( REDGUI_PATH + TXT("cursors\\movecursor.xbm") ).AsChar(), 0 );
		CGatheredResource GHorizontalResizeCursor( String( REDGUI_PATH + TXT("cursors\\horizontalresizecursor.xbm") ).AsChar(), 0 );
		CGatheredResource GVerticalResizeCursor( String( REDGUI_PATH + TXT("cursors\\verticalresizecursor.xbm") ).AsChar(), 0 );
		CGatheredResource GSlashResizeCursor( String( REDGUI_PATH + TXT("cursors\\slashresizecursor.xbm") ).AsChar(), 0 );
		CGatheredResource GBackslashResizeCursor( String( REDGUI_PATH + TXT("cursors\\backslashresizecursor.xbm") ).AsChar(), 0 );

		// icons
		CGatheredResource GTestIcon( String( REDGUI_PATH + TXT("imagetest.xbm") ).AsChar(), 0 );
		CGatheredResource GCheckIcon( String( REDGUI_PATH + TXT("checkicon.xbm") ).AsChar(), 0 );
		CGatheredResource GHourglassIcon( String( REDGUI_PATH + TXT("hourglassicon.xbm") ).AsChar(), 0 );

		CGatheredResource GExitIcon( String( REDGUI_PATH + TXT("exiticon.xbm") ).AsChar(), 0 );
		CGatheredResource GMaximizeIcon( String( REDGUI_PATH + TXT("maximizeicon.xbm") ).AsChar(), 0 );
		CGatheredResource GMinimizeIcon( String( REDGUI_PATH + TXT("minimizeicon.xbm") ).AsChar(), 0 );
		CGatheredResource GHelpIcon( String( REDGUI_PATH + TXT("helpicon.xbm") ).AsChar(), 0 );

		CGatheredResource GCollapseIcon( String( REDGUI_PATH + TXT("collapseicon.xbm") ).AsChar(), 0 );
		CGatheredResource GExpandIcon( String( REDGUI_PATH + TXT("expandicon.xbm") ).AsChar(), 0 );

		CGatheredResource GMinusIcon( String( REDGUI_PATH + TXT("minusicon.xbm") ).AsChar(), 0 );
		CGatheredResource GPlusIcon( String( REDGUI_PATH + TXT("plusicon.xbm") ).AsChar(), 0 );

		CGatheredResource GInformationIcon( String( REDGUI_PATH + TXT("informationicon.xbm") ).AsChar(), 0 );
		CGatheredResource GWarningIcon( String( REDGUI_PATH + TXT("warningicon.xbm") ).AsChar(), 0 );
		CGatheredResource GErrorIcon( String( REDGUI_PATH + TXT("erroricon.xbm") ).AsChar(), 0 );

		CGatheredResource GStopIcon( String( REDGUI_PATH + TXT("stopicon.xbm") ).AsChar(), 0 );
		CGatheredResource GPlayIcon( String( REDGUI_PATH + TXT("playicon.xbm") ).AsChar(), 0 );

		// arrows
		CGatheredResource GUpArrowIcon( String( REDGUI_PATH + TXT("uparrow.xbm") ).AsChar(), 0 );
		CGatheredResource GDownArrowIcon( String( REDGUI_PATH + TXT("downarrow.xbm") ).AsChar(), 0 );
		CGatheredResource GRightArrowIcon( String( REDGUI_PATH + TXT("rightarrow.xbm") ).AsChar(), 0 );
		CGatheredResource GLeftArrowIcon( String( REDGUI_PATH + TXT("leftarrow.xbm") ).AsChar(), 0 );

		// review system
		CGatheredResource GReviewEyeIcon( String( REDGUI_PATH + TXT("reviewmarkerdebugwindow\\eye.xbm") ).AsChar(), 0 );
		CGatheredResource GReviewAddIcon( String( REDGUI_PATH + TXT("reviewmarkerdebugwindow\\add.xbm") ).AsChar(), 0 );
		CGatheredResource GReviewPropertiesIcon( String( REDGUI_PATH + TXT("reviewmarkerdebugwindow\\properties.xbm") ).AsChar(), 0 );
		CGatheredResource GReviewRefreshIcon( String( REDGUI_PATH + TXT("reviewmarkerdebugwindow\\refresh.xbm") ).AsChar(), 0 );
		CGatheredResource GReviewFiltersIcon( String( REDGUI_PATH + TXT("reviewmarkerdebugwindow\\filter.xbm") ).AsChar(), 0 );
		CGatheredResource GReviewResetFiltersIcon( String( REDGUI_PATH + TXT("reviewmarkerdebugwindow\\reset_filters.xbm") ).AsChar(), 0 );
		CGatheredResource GReviewMoveIcon( String( REDGUI_PATH + TXT("reviewmarkerdebugwindow\\move.xbm") ).AsChar(), 0 );
		CGatheredResource GReviewFreeCam( String( REDGUI_PATH + TXT("reviewmarkerdebugwindow\\free_camera.xbm") ).AsChar(), 0 );
		
		// backgrounds
		CGatheredResource GWindowBackground( String( REDGUI_PATH + TXT("windowbackground.xbm") ).AsChar(), 0 );

		// fonts
		CGatheredResource GParachuteFont( TXT("engine\\fonts\\parachute.w2fnt"), 0 );

	}	//namespace Resources
}	// namespace RedGui

#endif	//NO_RED_GUI
