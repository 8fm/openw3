/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Editor tool to reset all DestructionSystemComponents in the current world. Probably only really useful in editors that allow interaction with
/// destructibles, like the Entity Editor.
class CEdDestructionResetTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdDestructionResetTool, IEditorTool, 0 );

public:
	CEdDestructionResetTool();
	virtual ~CEdDestructionResetTool();

	virtual String GetCaption() const;

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();

	virtual Bool UsableInActiveWorldOnly() const { return false; }
};


BEGIN_CLASS_RTTI( CEdDestructionResetTool );
PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
