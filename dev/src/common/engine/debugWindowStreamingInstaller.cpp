/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowTaskManager.h"
#include "redGuiList.h"
#include "redGuiLabel.h"
#include "redGuiCheckBox.h"
#include "redGuiManager.h"
#include "redGuiTimelineChart.h"
#include "redGuiGridLayout.h"
#include "redGuiAdvancedSlider.h"
#include "redGuiTimeline.h"

#include "debugWindowStreamingInstaller.h"

namespace DebugWindows
{
	CDebugWindowStreamingInstaller::CDebugWindowStreamingInstaller()
		: RedGui::CRedGuiWindow( 200, 200, 650, 600 )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowStreamingInstaller::NotifyOnTick );

		SetCaption( TXT("Streaming installer") );
		CreateControls();
	}

	CDebugWindowStreamingInstaller::~CDebugWindowStreamingInstaller()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowStreamingInstaller::NotifyOnTick );
	}

	void CDebugWindowStreamingInstaller::OnWindowOpened( CRedGuiControl* control )
	{
		
	}

	void CDebugWindowStreamingInstaller::OnWindowClosed( CRedGuiControl* control )
	{
	
	}

	void CDebugWindowStreamingInstaller::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		RED_UNUSED( eventPackage );

		if( GetVisible() == false )
		{
			return;
		}

	}

	void CDebugWindowStreamingInstaller::CreateControls()
	{
	}

}	// namespace DebugWindows

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
