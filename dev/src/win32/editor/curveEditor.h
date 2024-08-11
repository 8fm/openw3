/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CCurve;

wxDECLARE_EVENT( wxEVT_COMMAND_CURVE_CHANGING, wxCommandEvent );
wxDECLARE_EVENT( wxEVT_COMMAND_CURVE_CHANGED, wxCommandEvent );


class CEdGradientEditor;
class CEdCurveEditor : public wxPanel, public IEdCurveEditorCanvasHook
{
	DECLARE_EVENT_TABLE()

protected:
	CEdGradientEditor*		m_gradientEditor;
	CEdCurveEditorCanvas*	m_curveEditorCanvas;
	Bool					m_absoluteMove;

public:
	CEdCurveEditor()
		: m_gradientEditor( NULL )
	{}
	CEdCurveEditor( wxWindow* parent );
	~CEdCurveEditor();

	Bool IsCurveAdded( SCurveData* curve ) const;
	virtual void AddCurve( CurveParameter* curve, Bool setZoom = true, const Vector &curveScale = Vector(0,1.0f,0), const Color& color = Color( 255, 255, 255 ) );
	virtual void AddCurve( SCurveData* curve, const String& curveName, Bool setZoom = true, const Vector &curveScale = Vector(0,1.0f,0), CObject* curveContainer = NULL, const Color& color = Color( 255, 255, 255 ) );
	virtual void AddCurve( CCurve* curve, const String& curveName, Bool setZoom = true, const Vector &curveScale = Vector(0,1.0f,0), const Color& color = Color( 255, 255, 255 ) );
	Bool UpdateCurveParam( SCurveData* curve, const Vector &curveScale );
	void RemoveCurve( SCurveData* curve );
	void RemoveCurveGroup( const CName& curveGroupName );
	void RemoveAllCurveGroups();
	Bool IsCurveGroupAdded( const CName& curveGroupName ) const;

	// Force repaint of CurveEditorCanvas
	void CanvasRepaint() { ASSERT( GetCanvas() ); GetCanvas()->Repaint(); }

	void SetActiveRegion( Float start, Float end );

protected:
	void SetTime( Float time );
	void SetValue( Float value );
	void ClearTimeValue();
	void SetTimeValue( Float time, Float value );

	CEdCurveEditorCanvas* GetCanvas() { return m_curveEditorCanvas; }
	const CEdCurveEditorCanvas* GetCanvas() const { return m_curveEditorCanvas; }

public:
	// Event sent when some wx controls changed in curve editor
	void OnTimeValueChanged( wxCommandEvent& event );
	void OnAbsoluteChanged( wxCommandEvent& event );
	void OnShowControlPoints( wxCommandEvent& event );

public:
	void						SetGradientEditor( CEdGradientEditor* gradientEditor ) { m_gradientEditor = gradientEditor; }
	CEdGradientEditor*			GetGradientEditor() { return m_gradientEditor; }
	const CEdGradientEditor*	GetGradientEditor() const { return m_gradientEditor; }

	// Update UI when canvas selection changed (called by CurveEditorCanvas)
	void OnSelectionChanged();
	virtual void OnControlPointsChanged();
	virtual void OnControlPointsChangedComplete();

	virtual void OnCanvasHookSelectionChanged() { return OnSelectionChanged(); }
	virtual void OnCanvasHookControlPointsChanged() { return OnControlPointsChanged(); }
	virtual void OnCanvasHookControlPointsChangedComplete() { OnControlPointsChangedComplete(); }
};
