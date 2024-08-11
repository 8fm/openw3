/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoStep.h"
#include "curveEditorPanelHelper.h"

class CUndoCurveControlPointsAddOrDelete : public IUndoStep
{
	CUndoCurveControlPointsAddOrDelete() {}
	DECLARE_ENGINE_CLASS( CUndoCurveControlPointsAddOrDelete, IUndoStep, 0 );

private:
	CEdRenderingPanel*	m_panel;
	SMultiCurve* m_curve;
	struct SControlPoint
	{
		Uint32 m_index;
		EngineTransform m_transform;
	};
	TDynArray< SControlPoint >	m_controlPoints;
	Bool m_isAdd;

public:
	CUndoCurveControlPointsAddOrDelete( CEdRenderingPanel* panel, Bool isAdd );

	virtual String GetName();

	static void CreateStep( CEdRenderingPanel* panel, CCurveControlPointEntity* controlPointEntity, Bool add );
	static void FinishStep( CEdRenderingPanel* panel );

	virtual void DoUndo() override;
	virtual void DoRedo() override;

private:
	void ProcessControlPoint( CCurveEntity* curveEntity, const SControlPoint& controlPoint, Bool undo );
};

BEGIN_CLASS_RTTI( CUndoCurveControlPointsAddOrDelete )
	PARENT_CLASS( IUndoStep );
END_CLASS_RTTI();

//! Editor tool for editing curves
class CCurveEditorTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CCurveEditorTool, IEditorTool, 0 );

public:
	CEdRenderingPanel*			m_panel;
	CCurveControlPointEntity*	m_hoveredControlPoint;
	CCurveEditorPanelHelper		m_curveEditorHelper;

public:
	CCurveEditorTool();
	virtual String GetCaption() const override;
	virtual Bool StartPersistent( CEdRenderingPanel* panel ) override;
	virtual void End() override;	
	virtual Bool HandleActionClick( Int32 x, Int32 y ) override;
	virtual Bool OnDelete() override;
	virtual Bool OnCopy() override;
	virtual Bool OnCut() override;
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) override;
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y ) override;
	virtual Bool OnViewportTrack( const CMousePacket& packet ) override;
	virtual Bool HandleContextMenu( Int32 x, Int32 y, const Vector& collision ) override;
	virtual Bool UsableInActiveWorldOnly() const override { return false; }
	virtual Bool IsPersistent() const override { return true; }

private:
	Bool GetWorldClickPos( Int32 clickX, Int32 clickY, Vector& clickPos, Vector& clickNormal );
};

BEGIN_CLASS_RTTI( CCurveEditorTool );
PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();