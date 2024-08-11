/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace DLCTool
{
	// Show progress window
	void ShowProgressWindow();

	// Hide progress window
	void HideProgressWindow();

	// Update task title
	void UpdateTaskInfo( StringID id );

	// Update task progress
	void UpdateTaskProgress( INT cur, INT max );
}