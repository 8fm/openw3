/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CEdGradientEditor : public wxFrame
{
	DECLARE_EVENT_TABLE()

protected:
	CEdCurveEditor*			m_curveEditor;
	CEdGradientPicker*		m_gradientColor;
	CEdGradientPicker*		m_gradientAlpha;

public:
	CEdGradientEditor( wxWindow* parent, Bool hidden = false );
	~CEdGradientEditor();

	void					SetCurveEditor( CEdCurveEditor* curveEditor ) { m_curveEditor = curveEditor; }
	CEdCurveEditor*			GetCurveEditor() { return m_curveEditor; }
	const CEdCurveEditor*	GetCurveEditor() const { return m_curveEditor; }

	void SetCurves( CCurve* curveR, CCurve* curveG, CCurve* curveB, CCurve* curveA );
	void SetLinked( Bool linked );

	void OnLinkedChanged( wxCommandEvent& event );
	void OnShowTransparencyChanged( wxCommandEvent& event );
	void OnCloseWindow( wxCloseEvent& event );

	void OnControlPointsChanged();
	void OnClearSelection();
	void RefreshGradients();
};