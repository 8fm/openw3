/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "shortcutsEditor.h"
#include "shapesPreviewItem.h"
#include "propertyDialog.h"
#include "../../common/engine/curveEntitySpawner.h"

class wxMenuWithSeparators;

//! Information about selected curve objects
struct SCurveSelectionInfo
{
	CCurveEntity* m_singleCurve;											// If set indicates that we only have parts of a single curve selected (no parts of 2 or more different curves are selected)
	CCurveControlPointEntity* m_singleControlPoint;							// If set indicates that we only have a single control point selected (no 2 or more control points are selected)

	TDynArray< CCurveEntity* > m_curves;									// All selected curves
	TDynArray< CCurveControlPointEntity* > m_controlPoints;					// All selected curve control points
	TDynArray< CCurveTangentControlPointEntity* > m_tangentControlPoints;	// All selected curve tangent control points

	Bool m_enablePositionInterpolationModes;
	Bool m_enableRotationInterpolationModes;
	Bool m_enableEaseInOutOptions;
	Bool m_enableRotationRecalculation;
	Bool m_enableTimeRecalculation;
	Bool m_enableParentOptions;

	SCurveSelectionInfo()
		: m_enablePositionInterpolationModes( true )
		, m_enableRotationInterpolationModes( true )
		, m_enableEaseInOutOptions( true )
		, m_enableRotationRecalculation( true )
		, m_enableTimeRecalculation( true )
		, m_enableParentOptions( true )
	{}
};

//! Helper class used to extend panel context menu functionality by curve options
class CCurveEditorPanelHelper : public wxEvtHandler, public IEdEventListener
{
public:
	CCurveEditorPanelHelper();
	~CCurveEditorPanelHelper();

	//! To be invoked when generating context menu for a rendering panel
	void HandleContextMenu( CEdRenderingPanel* panel, wxMenuWithSeparators* rootMenu );

	static Bool GetSelectionInfo( CEdRenderingPanel* panel, SCurveSelectionInfo& info );

private:
	void AddCommand( wxMenu* menu, const wxString& caption, wxObjectEventFunction func );

	CEdRenderingPanel*				m_panel;
	wxMenuWithSeparators*			m_rootMenu;
	Int32							m_currentItemId;
	CEdPropertyDialog*				m_dlg;
	CCurveEntity*					m_curEnt;

public:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	CCurveControlPointEntity*		GetSelectedCurveControlPointEntity();
	CCurveEntity*					GetSelectedCurveEntity();

private:
	void OnAddControlPointAfter( wxCommandEvent& event );
	void OnAddControlPointBefore( wxCommandEvent& event );
	void OnToggleCurvePositionRendering( wxCommandEvent& event );
	void OnToggleCurveRotationRendering( wxCommandEvent& event );
	void OnToggleDebugCurveRun( wxCommandEvent& event );
	void OnHideCurve( wxCommandEvent& event );
	void OnToggleCurveTimeVisualization( wxCommandEvent& event );
	void OnToggleAutomaticCurveTimeByDistanceCalculation( wxCommandEvent& event );
	void OnToggleAutomaticCurveTimeCalculation( wxCommandEvent& event );
	void OnToggleAutomaticCurveRotationFromDirectionCalculation( wxCommandEvent& event );
	void OnRecalculateCurveTimeByDistance( wxCommandEvent& event );
	void OnRecalculateCurveTimeByDistance_SelectedControlPointsOnly( wxCommandEvent& event );
	void OnRecalculateCurveTimeByIndices( wxCommandEvent& event );
	void OnRecalculateCurveRotationFromDirection( wxCommandEvent& event );
	void OnToggleCurveLooping( wxCommandEvent& event );
	void OnSelectAllCurveControlPoints( wxCommandEvent& event );
	void OnEnablePositionLinearInterpolationMode( wxCommandEvent& event );
	void OnEnablePositionAutomaticInterpolationMode( wxCommandEvent& event );
	void OnEnablePositionManualInterpolationMode( wxCommandEvent& event );
	void OnEnableBezierManualMode( wxCommandEvent& event );
	void OnEnableBezierSymmetricManualMode( wxCommandEvent& event );
	void OnEnableBezierSymmetricDirectionManualMode( wxCommandEvent& event );
	void OnRecalculateBezierTangents( wxCommandEvent& event );
	void OnEnableRotationLinearInterpolationMode( wxCommandEvent& event );
	void OnEnableRotationAutomaticInterpolationMode( wxCommandEvent& event );
	void OnActivateEntityLayer( wxCommandEvent& event );
	void OnToggleDrawOnTop( wxCommandEvent& event );
	void OnToggleEaseParams( wxCommandEvent& event );
	void OnSelectCurveOwner( wxCommandEvent& event );
	void OnSpawnEntitiesTool( wxCommandEvent& event );
};