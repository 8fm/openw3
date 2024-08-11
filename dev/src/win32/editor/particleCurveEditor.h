/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CEdParticleEditor;

class CEdParticleCurveEditor : public CEdCurveEditor
{
private:
	CEdParticleEditor*	m_editor;

public:
	CEdParticleCurveEditor( wxWindow* parent, CEdParticleEditor* particleEditor );
	~CEdParticleCurveEditor();
	virtual void AddCurve( CurveParameter* curve, const String& moduleName, Bool pinned = false, Bool setZoom = true, const Vector &curveScale = Vector(0,1.0,0) );
	virtual void AddCurve( SCurveData* curve, const String& curveName, Bool setZoom = true, const Vector &curveScale = Vector(0,1.0,0), CObject* container = NULL );
	virtual void AddCurve( CCurve* curve, const String& curveName, Bool setZoom = true, const Vector &curveScale = Vector(0,1.0,0) );

public:
	virtual void OnControlPointsChanged();
};
