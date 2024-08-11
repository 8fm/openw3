/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

class CDebugWindowPhysicsArea : public RedGui::CRedGuiWindow
{
public:
	CDebugWindowPhysicsArea();
	~CDebugWindowPhysicsArea();

private:
	void OnWindowOpened( CRedGuiControl* control );
	void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );

	void Draw();
};

#endif
#endif
