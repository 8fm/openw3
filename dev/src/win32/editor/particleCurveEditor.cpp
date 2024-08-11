/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "particleCurveEditor.h"
#include "particleEditor.h"
#include "../../common/engine/curve.h"

CEdParticleCurveEditor::CEdParticleCurveEditor( wxWindow* parent, CEdParticleEditor* particleEditor )
	: CEdCurveEditor( parent )
	, m_editor( particleEditor )
{

}

CEdParticleCurveEditor::~CEdParticleCurveEditor()
{

}
void CEdParticleCurveEditor::AddCurve( CCurve* curve, const String& curveName, Bool setZoom /* = true */, const Vector &curveScale /*= Vector(0,1.0,0)*/ )
{
	if ( curve )
	{
		ASSERT( GetCanvas() );
		CCurveEditionWrapper* curves = GetCanvas()->AddCurve( curve, curveName, curveScale );
		if ( curves )
		{
			curves->SetUserData( curve );
		}
		if ( setZoom ) GetCanvas()->SetZoomedRegionToFit();
		GetCanvas()->Repaint();
	}
}
void CEdParticleCurveEditor::AddCurve( SCurveData* curve, const String& curveName, Bool setZoom /* = true */, const Vector &curveScale /*= Vector(0,1.0,0)*/, CObject* container /*= NULL*/ )
{
	HALT( "Add curve with SCurveData can't be used on ParticleCurveEditor." );
}

void CEdParticleCurveEditor::AddCurve( CurveParameter* curve, const String& moduleName, Bool pinned /* = false */, Bool setZoom /* = true */, const Vector &curveScale /*= Vector(0,1.0,0)*/ )
{
	if ( curve )
	{
		const Color bckgColor = curve->GetColor();

		ASSERT( GetCanvas() );
		TDynArray< CCurveEditionWrapper* > curves = GetCanvas()->AddCurveGroup( curve, moduleName, bckgColor, pinned, curveScale );
		for ( Uint32 i = 0; i < curves.Size(); ++i )
		{
			curves[i]->SetUserData( curve->GetCurve( i ) );
		}
		if ( setZoom ) GetCanvas()->SetZoomedRegionToFit();
		GetCanvas()->Repaint();
	}
}
void CEdParticleCurveEditor::OnControlPointsChanged()
{
	CEdCurveEditor::OnControlPointsChanged();

	TDynArray< CCurveEditionWrapper* > curves;
	m_curveEditorCanvas->GetSelectedCurves( curves );

	TDynArray< CCurve* > ccurves;

	for ( Uint32 i = 0; i < curves.Size(); ++i )
	{
		ccurves.PushBack( ( CCurve* )curves[i]->GetUserData() );
	}
	m_editor->OnCurvesChanged( ccurves );
}