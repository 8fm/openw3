#pragma once
#include "dialogEditor.h"

class CEdSceneEditor_Hacks
{
public:
	/*
	*	When editor is in "edit" or "free move" camera mode its using CEdPreviewPanel : public CEdRenderingPanel for handling camera data 
	*	Those classes does not provide support for changing dof parameters it can be only done trough CEnvironmentManager in CWorld in CEdPreviewPanel
	*   This hack allows to change ( and edit ) dof effects while in "edit" and "free move" camera 
	*/
	static void OnPreview_ViewportTick_ApplyDof( CEdSceneEditor* editor );
};