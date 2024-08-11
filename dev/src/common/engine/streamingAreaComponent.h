/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "areaComponent.h"

/// Specialized component used to mark stuff for streaming
class CStreamingAreaComponent : public CAreaComponent
{
	DECLARE_ENGINE_CLASS( CStreamingAreaComponent, CAreaComponent, 0 );

public:
	CStreamingAreaComponent();

	// Get contour rendering color
	virtual Color CalcLineColor() const override;

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

private:
	Bool	m_testInEditor;

	// in editor testing stuff
	virtual void OnPropertyPostChange( CProperty* prop );
	virtual void OnDetached( CWorld* world );
};

BEGIN_CLASS_RTTI( CStreamingAreaComponent );
	PARENT_CLASS( CAreaComponent );
	PROPERTY_EDIT_NOT_COOKED( m_testInEditor, TXT("Enable this streaming area to see how it will work in game") );
END_CLASS_RTTI();
