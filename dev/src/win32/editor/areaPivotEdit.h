/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CPivotEditorEntity;

/// Editor tool for editing area pivot
class CEdPivotEdit : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdPivotEdit, IEditorTool, 0 );

public:
	CWorld*					m_world;						//!< World shortcut

	CAreaComponent*			m_editedComponent;
	CPivotEditorEntity*		m_pivotEntity;

public:
	CEdPivotEdit();
	virtual String GetCaption() const;
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );

	virtual Bool UsableInActiveWorldOnly() const { return false; }
};

BEGIN_CLASS_RTTI( CEdPivotEdit );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
