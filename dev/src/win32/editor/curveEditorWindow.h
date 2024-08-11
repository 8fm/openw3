#pragma  once

class CEdCurveEditorFrame: public wxFrame
{
	DECLARE_EVENT_TABLE();

	void Init( wxWindow* parent );
public:
	CEdCurveEditorFrame( wxWindow* parent, SCurveData* curve );
	CEdCurveEditorFrame( wxWindow* parent, TDynArray< SCurveData* >& curves );
	~CEdCurveEditorFrame();

	void SetActiveRegion( Float start, Float end );

protected:
	void OnOk( wxCommandEvent& event );

protected:
	CEdCurveEditor* m_curveEditor;
};